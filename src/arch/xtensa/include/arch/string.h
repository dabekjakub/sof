/*
 * Copyright (c) 2018, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Intel Corporation nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 *
 */

#ifndef __INCLUDE_ARCH_STRING_SOF__
#define __INCLUDE_ARCH_STRING_SOF__

#include <errno.h>

void *memcpy(void *dest, const void *src, size_t n);
int memcpy_s(void *dest, size_t dest_size, const void *src, size_t src_size);
void *xthal_memcpy(void *dst, const void *src, size_t len);
void *__vec_memcpy(void *dst, const void *src, size_t len);
void *__vec_memset(void *dest, int data, size_t src_size);

#define arch_memcpy(dest, src, size) xthal_memcpy(dest, src, size)

static inline int arch_memcpy_s(void *dest, size_t dest_size, const void *src,
								size_t src_size)
{
	if (!dest || !src)
		return -EINVAL;

	if ((dest + dest_size >= src && dest + dest_size <= src + src_size) ||
		(src + src_size >= dest && src + src_size <= dest + dest_size))
		return -EINVAL;

	if (src_size > dest_size)
		return -EINVAL;
#if __XCC__
	__vec_memcpy(dest, src, src_size);
#else
	memcpy(dest, src, src_size);
#endif
	return 0;
}

static inline int arch_memset_s(void *dest, size_t dest_size,
								int data, size_t count)
{
	if (!dest)
		return -EINVAL;

	if (count > dest_size)
		return -EINVAL;

#if __XCC__ && !CONFIG_HOST
	if (!__vec_memset(dest, data, count))
		return -ENOMEM;
#else
	if (!memset(dest, data, count))
		return -ENOMEM;
#endif
	return 0;
}

#endif
