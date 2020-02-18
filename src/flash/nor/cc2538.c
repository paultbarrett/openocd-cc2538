/***************************************************************************
 *   Copyright (C) 2020 Loci Controls Inc                                  *
 *   Copyright (C) 2017 by Texas Instruments, Inc.                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "imp.h"
#include "cc2538.h"
#include <helper/binarybuffer.h>
#include <helper/time_support.h>
#include <target/algorithm.h>
#include <target/armv7m.h>
#include <target/image.h>

#define FLASH_TIMEOUT 10000

struct cc2538_bank {
	const char *family_name;
        uint16_t chip_id;
	bool probed;
	struct working_area *working_area;
	struct armv7m_algorithm armv7m_info;
	const uint8_t *algo_code;
	uint32_t algo_size;
	uint32_t algo_working_size;
	uint32_t buffer_addr[2];
	uint32_t params_addr[2];
};

static int cc2538_auto_probe(struct flash_bank *bank);

static int cc2538_wait_algo_done(struct flash_bank *bank, uint32_t params_addr)
{
	struct target *target = bank->target;

	uint32_t status_addr = params_addr + CC2538_STATUS_OFFSET;
	uint32_t status = CC2538_BUFFER_FULL;
	long long start_ms;
	long long elapsed_ms;

	int retval = ERROR_OK;

	start_ms = timeval_ms();
	while (CC2538_BUFFER_FULL == status) {
		retval = target_read_u32(target, status_addr, &status);
		if (ERROR_OK != retval)
			return retval;

		elapsed_ms = timeval_ms() - start_ms;
		if (elapsed_ms > 500)
			keep_alive();
		if (elapsed_ms > FLASH_TIMEOUT)
			break;
	};

	if (CC2538_BUFFER_EMPTY != status) {
		LOG_ERROR("cc2538: Flash operation failed (0x%08x)", status);
		return ERROR_FAIL;
	}

	return ERROR_OK;
}

static int cc2538_init(struct flash_bank *bank)
{
	struct target *target = bank->target;
	struct cc2538_bank *cc2538_bank = bank->driver_priv;

	int retval;

	/* Make sure we've probed the flash to get the size */
	retval = cc2538_auto_probe(bank);
	if (ERROR_OK != retval)
		return retval;

	/* Check for working area to use for flash helper algorithm */
	if (NULL != cc2538_bank->working_area)
		target_free_working_area(target, cc2538_bank->working_area);
	retval = target_alloc_working_area(target, cc2538_bank->algo_working_size,
				&cc2538_bank->working_area);
	if (ERROR_OK != retval)
		return retval;

	/* Confirm the defined working address is the area we need to use */
	if (CC2538_ALGO_BASE_ADDRESS != cc2538_bank->working_area->address)
		return ERROR_TARGET_RESOURCE_NOT_AVAILABLE;

	/* Write flash helper algorithm into target memory */
	retval = target_write_buffer(target, CC2538_ALGO_BASE_ADDRESS,
				cc2538_bank->algo_size, cc2538_bank->algo_code);
	if (ERROR_OK != retval) {
		LOG_ERROR("cc2538: Failed to load flash helper algorithm");
		target_free_working_area(target, cc2538_bank->working_area);
		return retval;
	}

	/* Initialize the ARMv7 specific info to run the algorithm */
	cc2538_bank->armv7m_info.common_magic = ARMV7M_COMMON_MAGIC;
	cc2538_bank->armv7m_info.core_mode = ARM_MODE_THREAD;

	/* Begin executing the flash helper algorithm */
	retval = target_start_algorithm(target, 0, NULL, 0, NULL,
				CC2538_ALGO_BASE_ADDRESS, 0, &cc2538_bank->armv7m_info);
	if (ERROR_OK != retval) {
		LOG_ERROR("cc2538: Failed to start flash helper algorithm");
		target_free_working_area(target, cc2538_bank->working_area);
		return retval;
	}

	/*
	 * At this point, the algorithm is running on the target and
	 * ready to receive commands and data to flash the target
	 */

	return retval;
}

static int cc2538_quit(struct flash_bank *bank)
{
	struct target *target = bank->target;
	struct cc2538_bank *cc2538_bank = bank->driver_priv;

	int retval;

	/* Regardless of the algo's status, attempt to halt the target */
	(void)target_halt(target);

	/* Now confirm target halted and clean up from flash helper algorithm */
	retval = target_wait_algorithm(target, 0, NULL, 0, NULL, 0, FLASH_TIMEOUT,
				&cc2538_bank->armv7m_info);

	target_free_working_area(target, cc2538_bank->working_area);
	cc2538_bank->working_area = NULL;

	return retval;
}

static int cc2538_mass_erase(struct flash_bank *bank)
{
	struct target *target = bank->target;
	struct cc2538_bank *cc2538_bank = bank->driver_priv;
	struct cc2538_algo_params algo_params;

	int retval;

	if (TARGET_HALTED != target->state) {
		LOG_ERROR("Target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	retval = cc2538_init(bank);
	if (ERROR_OK != retval)
		return retval;

	/* Initialize algorithm parameters */
	buf_set_u32(algo_params.address, 0, 32, 0);
	buf_set_u32(algo_params.length,  0, 32, 4);
	buf_set_u32(algo_params.command, 0, 32, CC2538_CMD_ERASE_ALL);
	buf_set_u32(algo_params.status,  0, 32, CC2538_BUFFER_FULL);

	/* Issue flash helper algorithm parameters for mass erase */
	retval = target_write_buffer(target, cc2538_bank->params_addr[0],
				sizeof(algo_params), (uint8_t *)&algo_params);

	/* Wait for command to complete */
	if (ERROR_OK == retval)
		retval = cc2538_wait_algo_done(bank, cc2538_bank->params_addr[0]);

	/* Regardless of errors, try to close down algo */
	(void)cc2538_quit(bank);

	return retval;
}

FLASH_BANK_COMMAND_HANDLER(cc2538_flash_bank_command)
{
	struct cc2538_bank *cc2538_bank;

	if (CMD_ARGC < 6)
		return ERROR_COMMAND_SYNTAX_ERROR;

	cc2538_bank = malloc(sizeof(struct cc2538_bank));
	if (NULL == cc2538_bank)
		return ERROR_FAIL;

	/* Initialize private flash information */
	memset((void *)cc2538_bank, 0x00, sizeof(struct cc2538_bank));

	/* Finish initialization of bank */
	bank->driver_priv = cc2538_bank;
	bank->next = NULL;

	return ERROR_OK;
}

static int cc2538_erase(struct flash_bank *bank, int first, int last)
{
	struct target *target = bank->target;
	struct cc2538_bank *cc2538_bank = bank->driver_priv;
	struct cc2538_algo_params algo_params;

	uint32_t address;
	uint32_t length;
	int retval;

	if (TARGET_HALTED != target->state) {
		LOG_ERROR("Target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	/* Do a mass erase if user requested all sectors of flash */
	if ((first == 0) && (last == (bank->num_sectors - 1))) {
		/* Request mass erase of flash */
		return cc2538_mass_erase(bank);
	}

	address = first * CC2538_SECTOR_LENGTH;
	length = (last - first + 1) * CC2538_SECTOR_LENGTH;

	retval = cc2538_init(bank);
	if (ERROR_OK != retval)
		return retval;

	/* Set up algorithm parameters for erase command */
	buf_set_u32(algo_params.address, 0, 32, address);
	buf_set_u32(algo_params.length,  0, 32, length);
	buf_set_u32(algo_params.command, 0, 32, CC2538_CMD_ERASE_SECTORS);
	buf_set_u32(algo_params.status,  0, 32, CC2538_BUFFER_FULL);

	/* Issue flash helper algorithm parameters for erase */
	retval = target_write_buffer(target, cc2538_bank->params_addr[0],
				sizeof(algo_params), (uint8_t *)&algo_params);

	/* If no error, wait for erase to finish */
	if (ERROR_OK == retval)
		retval = cc2538_wait_algo_done(bank, cc2538_bank->params_addr[0]);

	/* Regardless of errors, try to close down algo */
	(void)cc2538_quit(bank);

	return retval;
}

static int cc2538_write(struct flash_bank *bank, const uint8_t *buffer,
	uint32_t offset, uint32_t count)
{
	struct target *target = bank->target;
	struct cc2538_bank *cc2538_bank = bank->driver_priv;
	struct cc2538_algo_params algo_params[2];
	uint32_t size = 0;
	long long start_ms;
	long long elapsed_ms;
	uint32_t address;

	uint32_t index;
	int retval;

	if (TARGET_HALTED != target->state) {
		LOG_ERROR("Target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	retval = cc2538_init(bank);
	if (ERROR_OK != retval)
		return retval;

	/* Initialize algorithm parameters to default values */
	buf_set_u32(algo_params[0].command, 0, 32, CC2538_CMD_PROGRAM);
	buf_set_u32(algo_params[1].command, 0, 32, CC2538_CMD_PROGRAM);

	/* Write requested data, ping-ponging between two buffers */
	index = 0;
	start_ms = timeval_ms();
	address = bank->base + offset;
	while (count > 0) {

		if (count > CC2538_SECTOR_LENGTH)
			size = CC2538_SECTOR_LENGTH;
		else
			size = count;

		/* Put next block of data to flash into buffer */
		retval = target_write_buffer(target, cc2538_bank->buffer_addr[index],
					size, buffer);
		if (ERROR_OK != retval) {
			LOG_ERROR("Unable to write data to target memory");
			break;
		}

		/* Update algo parameters for next block */
		buf_set_u32(algo_params[index].address, 0, 32, address);
		buf_set_u32(algo_params[index].length,  0, 32, size);
		buf_set_u32(algo_params[index].status,  0, 32, CC2538_BUFFER_FULL);

		/* Issue flash helper algorithm parameters for block write */
		retval = target_write_buffer(target, cc2538_bank->params_addr[index],
					sizeof(algo_params[index]), (uint8_t *)&algo_params[index]);
		if (ERROR_OK != retval)
			break;

		/* Wait for next ping pong buffer to be ready */
		index ^= 1;
		retval = cc2538_wait_algo_done(bank, cc2538_bank->params_addr[index]);
		if (ERROR_OK != retval)
			break;

		count -= size;
		buffer += size;
		address += size;

		elapsed_ms = timeval_ms() - start_ms;
		if (elapsed_ms > 500)
			keep_alive();
	}

	/* If no error yet, wait for last buffer to finish */
	if (ERROR_OK == retval) {
		index ^= 1;
		retval = cc2538_wait_algo_done(bank, cc2538_bank->params_addr[index]);
	}

	/* Regardless of errors, try to close down algo */
	(void)cc2538_quit(bank);

	return retval;
}

static int cc2538_probe(struct flash_bank *bank)
{
	struct target *target = bank->target;
	struct cc2538_bank *cc2538_bank = bank->driver_priv;

	uint32_t value;
	int num_sectors;

	int retval;

	retval = target_read_u32(target, CC2538_FLASH_CTRL_DIECFG0, &value);
	if (ERROR_OK != retval)
		return retval;

        cc2538_bank->chip_id = value >> 16;

        num_sectors = ((value & 0x70) >> 4) * 0x20000 / CC2538_SECTOR_LENGTH;
	if (num_sectors > CC2538_MAX_SECTORS)
		num_sectors = CC2538_MAX_SECTORS;

	bank->sectors = malloc(sizeof(struct flash_sector) * num_sectors);
	if (NULL == bank->sectors)
		return ERROR_FAIL;

	/* Set up flash helper algorithm */
        cc2538_bank->algo_code = cc2538_algo;
        cc2538_bank->algo_size = sizeof(cc2538_algo);
        cc2538_bank->algo_working_size = CC2538_WORKING_SIZE;
        cc2538_bank->buffer_addr[0] = CC2538_ALGO_BUFFER_0;
        cc2538_bank->buffer_addr[1] = CC2538_ALGO_BUFFER_1;
        cc2538_bank->params_addr[0] = CC2538_ALGO_PARAMS_0;
        cc2538_bank->params_addr[1] = CC2538_ALGO_PARAMS_1;

	bank->base = CC2538_FLASH_BASE_ADDR;
	bank->num_sectors = num_sectors;
	bank->size = num_sectors * CC2538_SECTOR_LENGTH;
	bank->write_start_alignment = 0;
	bank->write_end_alignment = 0;

	for (int i = 0; i < num_sectors; i++) {
		bank->sectors[i].offset = i * CC2538_SECTOR_LENGTH;
		bank->sectors[i].size = CC2538_SECTOR_LENGTH;
		bank->sectors[i].is_erased = -1;
		bank->sectors[i].is_protected = 0;
	}

	/* We've successfully determined the stats on the flash bank */
	cc2538_bank->probed = true;

	/* If we fall through to here, then all went well */

	return ERROR_OK;
}

static int cc2538_auto_probe(struct flash_bank *bank)
{
	struct cc2538_bank *cc2538_bank = bank->driver_priv;

	int retval = ERROR_OK;

	if (!cc2538_bank->probed)
		retval = cc2538_probe(bank);

	return retval;
}

static int cc2538_info(struct flash_bank *bank, char *buf, int buf_size)
{
	struct cc2538_bank *cc2538_bank = bank->driver_priv;
	int printed = 0;

	printed = snprintf(buf, buf_size,
                "cc2538 device: chip ID 0x%04x\n", cc2538_bank->chip_id);

	if (printed >= buf_size)
		return ERROR_BUF_TOO_SMALL;

	return ERROR_OK;
}

const struct flash_driver cc2538_flash = {
	.name = "cc2538",
	.flash_bank_command = cc2538_flash_bank_command,
	.erase = cc2538_erase,
	.write = cc2538_write,
	.read = default_flash_read,
	.probe = cc2538_probe,
	.auto_probe = cc2538_auto_probe,
	.erase_check = default_flash_blank_check,
	.info = cc2538_info,
	.free_driver_priv = default_flash_free_driver_priv,
};
