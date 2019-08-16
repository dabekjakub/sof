/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2019 Intel Corporation. All rights reserved.
 *
 * Author: Jakub Dabek <jakub.dabek@intel.com>
 */

#include <sof/lib/memory_pm.h>
#include <sof/lib/shim.h>
#include <sof/trace/trace.h>
#include <user/trace.h>
#include <sof/lib/wait.h>
#include <stdint.h>
#include <errno.h>

#define trace_mem_pm(__e, ...) \
	trace_event(TRACE_CLASS_MEM, __e, ##__VA_ARGS__)
/**
 * \brief Set power gateing of memory banks in address range
 *
 * Power gates address range only on full banks if given address form mid block
 * it will try to narrow down power gate to nearest full banks
 *
 * \param[in] ptr Ptr to address from which to start gating.
 * \param[in] size Size of memory to manage.
 * \param[in] bit Boolean deciding banks desired state (1 powered 0 gated).
 */
void set_power_gate_for_memory_address_range(void *ptr,
	uint32_t size, uint32_t bit)
{
	uint32_t start_bank_id;
	uint32_t ending_bank_id;
	void *end_ptr = ptr + size;

	/* if ptr is not aligned to bank size change it to
	 * closest possible memory address at the start of bank
	 * or end for end address
	 */
	if ((uintptr_t)ptr % SRAM_BANK_SIZE)
		ptr = ptr + (uintptr_t)(SRAM_BANK_SIZE
		- (uintptr_t)ptr % SRAM_BANK_SIZE);
	if ((uintptr_t)end_ptr % SRAM_BANK_SIZE)
		end_ptr = end_ptr
		- (uintptr_t)((uintptr_t)end_ptr % SRAM_BANK_SIZE);
	/* return if no full bank could be found for power gate control */
	if (ptr - end_ptr < SRAM_BANK_SIZE)
		trace_mem_pm("Could not find full bank "
			"to perform gating operation");
		return;


	start_bank_id = (HP_SRAM_BASE - (uintptr_t)ptr) / SRAM_BANK_SIZE;
	ending_bank_id = (HP_SRAM_BASE - (uintptr_t)end_ptr) / SRAM_BANK_SIZE;

	set_banks_gating(start_bank_id, ending_bank_id, bit);
}

/**
 * \brief Set memory power gating hw bit mask for memory banks
 * \param[in] start_bank_id Id of first bank to be managed (inclusive) 0 based.
 * \param[in] ending_bank_id Id of last bank to be managed (inclusive) 0 based.
 * \param[in] bit Boolean deciding banks desired state (1 powered 0 gated).
 */
void set_banks_gating(uint32_t start_bank_id, uint32_t ending_bank_id,
		      uint32_t bit)
{
	struct ebb_data *operation_data;

	operation_data->start_bank_id = start_bank_id;
	operation_data->ending_bank_id = ending_bank_id;
	operation_data->start_bank_id_high = start_bank_id - EBB_SEGMENT_SIZE;
	operation_data->ending_bank_id_high = ending_bank_id - EBB_SEGMENT_SIZE;

	/* Take note that if there are more banks than EBB_SEGMENT_SIZE
	 * then those over have to be controlled by second mask
	 * hence start_bank_id and start_bank_id_high
	 * for calculating the banks in second segment (currently two
	 * segments is all HW supports)
	 */
	shim_write(SHIM_LDOCTL, SHIM_LDOCTL_HPSRAM_LDO_ON);

	set_bank_masks(operation_data);

	calculate_new_masks(operation_data, bit);

	write_new_masks_and_check_status(operation_data);

	shim_write(SHIM_LDOCTL, SHIM_LDOCTL_HPSRAM_LDO_BYPASS);
}

/**
 * \brief Read masks from hw registers and prepare new masks from data
 * \param[in] ebb_data All masks data required for calculations.
 */
void set_bank_masks(struct ebb_data *operation_data)
{
	/* Bit is set for gated banks in  HSPGISTS register so
	 * we negate it to have consistent 1 = powered 0 = gated
	 * convention
	 */
	operation_data->ebb_current_mask0 = ~io_reg_read(HSPGISTS0);
	operation_data->ebb_current_mask1 = ~io_reg_read(HSPGISTS1);

	/* Calculate mask of bits (banks) to set to powered */
	operation_data->change_mask0 = SET_BITS(operation_data->ending_bank_id,
		operation_data->start_bank_id,
		MASK(operation_data->ending_bank_id, 0));
	operation_data->change_mask1 = SET_BITS(
		operation_data->ending_bank_id_high,
		operation_data->start_bank_id_high,
		MASK(operation_data->ending_bank_id_high, 0));
}


/**
 * \brief Calculates new mask to write to hw register
 * \param[in] ebb_data All masks data required for calculations.
 * \param[in] bit Boolean deciding banks desired state (1 powered 0 gated).
 */
void calculate_new_masks(struct ebb_data *operation_data, uint32_t bit)
{
	/* check if either start or end bank is in different segment
	 * and treat it on case by case basis first only banks
	 * the segments above EBB_SEGMENT_SIZE are managed
	 * (separate hw register has to be used for ebb
	 * numbered over 32 different ebb segment)
	 */
	if (operation_data->start_bank_id > EBB_SEGMENT_SIZE_ZERO_BASE
		&& operation_data->ending_bank_id
		> EBB_SEGMENT_SIZE_ZERO_BASE) {
		/* Set the bits for all powered banks operations
		 * depend whether we turn on or off the memory
		 */
		operation_data->ebb_new_mask0 =
			operation_data->ebb_current_mask0;
		operation_data->ebb_new_mask1 = bit ?
			operation_data->ebb_current_mask1
			| operation_data->change_mask1 :
			operation_data->ebb_current_mask1
			& ~operation_data->change_mask1;

	/* second check if managing spans above and bellow EBB_SEGMENT_SIZE
	 * meaning we have to handle both operation_data for both segments
	 */
	} else if (operation_data->start_bank_id > EBB_SEGMENT_SIZE_ZERO_BASE
		&& operation_data->ending_bank_id
		< EBB_SEGMENT_SIZE_ZERO_BASE) {
		operation_data->ebb_new_mask0 = bit ?
			operation_data->ebb_current_mask0
			| operation_data->change_mask0 :
			operation_data->ebb_current_mask0
			& ~operation_data->change_mask0;
		operation_data->ebb_new_mask1 = bit ?
			operation_data->ebb_current_mask1
			| operation_data->change_mask1 :
			operation_data->ebb_current_mask1
			& ~operation_data->change_mask1;

	/* if only first segment needs changes */
	} else {
		operation_data->ebb_new_mask0 = bit ?
			operation_data->ebb_current_mask0
			| operation_data->change_mask0 :
			operation_data->ebb_current_mask0
			& ~operation_data->change_mask0;
		operation_data->ebb_new_mask1 =
			operation_data->ebb_current_mask1;
	}

}

/**
 * \brief Write calculated masks to HW registers
 * \param[in] ebb_data All masks data with masks to write.
 */
void write_new_masks_and_check_status(struct ebb_data *operation_data)
{
	uint32_t status, delay_count = 256;

	/* HSPGCTL, HSRMCTL use reverse logic - 0 means EBB is power gated */
	io_reg_write(HSPGCTL0,  operation_data->ebb_new_mask0);
	io_reg_write(HSRMCTL0,  operation_data->ebb_new_mask0);
	io_reg_write(HSPGCTL1,  operation_data->ebb_new_mask1);
	io_reg_write(HSRMCTL1,  operation_data->ebb_new_mask1);

	/* Query the power status of first part of HP memory
	 * to check whether it has been powered up. A few
	 * cycles are needed for it to be powered up
	 */
	status = io_reg_read(HSPGISTS0);
	while (status != operation_data->ebb_new_mask0) {
		idelay(delay_count);
		status = io_reg_read(HSPGISTS0);
	}
	/* Query the power status of second part of HP memory */

	status = io_reg_read(HSPGISTS1);
	while (status != operation_data->ebb_new_mask1) {
		idelay(delay_count);
		status = io_reg_read(HSPGISTS1);
	}
	/* add some delay before touch power register */
	idelay(delay_count);
}
