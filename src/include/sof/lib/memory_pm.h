/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2019 Intel Corporation. All rights reserved.
 *
 * Author: Jakub Dabek <jakub.dabek@intel.com>
 */

#ifndef __SOF_LIB_MEMORY_PM_
#define __SOF_LIB_MEMORY_PM_

#include <stdint.h>

#define EBB_SEGMENT_SIZE_ZERO_BASE (EBB_SEGMENT_SIZE - 1)

struct ebb_data {
	uint32_t ebb_current_mask0;
	uint32_t ebb_current_mask1;
	uint32_t ebb_new_mask0;
	uint32_t ebb_new_mask1;
	uint32_t change_mask0;
	uint32_t change_mask1;
	uint32_t start_bank_id;
	uint32_t ending_bank_id;
	uint32_t start_bank_id_high;
	uint32_t ending_bank_id_high;
};

void set_power_gate_for_memory_address_range(void *ptr,
	uint32_t size, uint32_t bit);
void set_banks_gating(uint32_t start_bank_id,
	uint32_t ending_bank_id, uint32_t bit);
void set_bank_masks(struct ebb_data *operation_data);
void calculate_new_masks(struct ebb_data *operation_data,
	uint32_t bit);
void write_new_masks_and_check_status(
	struct ebb_data *operation_data);
#endif
