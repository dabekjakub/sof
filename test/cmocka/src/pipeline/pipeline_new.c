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
 * Author: Jakub Dabek <jakub.dabek@linux.intel.com>
 */

#include <cmocka.h>
#include <stddef.h>
#include "pipeline_mocks.c"

static void test_pipeline_new(void **state)
{
	/*Initialize structs for arguments*/
	struct sof_ipc_pipe_new *pipe_desc;
	struct comp_dev *cd;

	/*Memmory allocation values check. Pipeline can have those changed
	 *in future so expect errors here if any change to pipeline memmory
	 *capabilities or memmory space was made
	 */
	expect_value(__wrap_rzalloc, zone, RZONE_RUNTIME);
	expect_value(__wrap_rzalloc, caps, SOF_MEM_CAPS_RAM);
	expect_value(__wrap_rzalloc, bytes, sizeof(struct pipeline));

	/*Testing component*/
	struct pipeline *result = pipeline_new(pipe_desc, cd);

	/*Pipeline should have been created so pointer can't be null*/
	assert_non_null(result);

	/*Pipeline should end in pre init state untli sheduler runs
	 *task that initializes it
	 */
	assert_int_equal(COMP_STATE_INIT, result->status);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_pipeline_new)
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
