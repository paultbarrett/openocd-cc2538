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

#ifndef OPENOCD_FLASH_NOR_CC2538_H
#define OPENOCD_FLASH_NOR_CC2538_H

#define CC2538_FLASH_BASE_ADDR   0x00200000
#define CC2538_FLASH_MAX_SIZE    0x00080000
#define CC2538_ALGO_BASE_ADDRESS 0x20000000

#define CC2538_FLASH_CTRL_DIECFG0 0x400D3014

/* Flash algorithm parameters */
#define CC2538_MAX_SECTORS   256
#define CC2538_SECTOR_LENGTH 0x800
#define CC2538_ALGO_BUFFER_0 0x20001c00
#define CC2538_ALGO_BUFFER_1 0x20002c00
#define CC2538_ALGO_PARAMS_0 0x20001bd8
#define CC2538_ALGO_PARAMS_1 0x20001bec
#define CC2538_WORKING_SIZE  (CC2538_ALGO_BUFFER_1 + CC2538_SECTOR_LENGTH - \
                              CC2538_ALGO_BASE_ADDRESS)
/* Flash helper algorithm buffer flags */
#define CC2538_BUFFER_EMPTY 0x00000000
#define CC2538_BUFFER_FULL  0xffffffff

/* Flash helper algorithm commands */
#define CC2538_CMD_NO_ACTION                     0
#define CC2538_CMD_ERASE_ALL                     1
#define CC2538_CMD_PROGRAM                       2
#define CC2538_CMD_ERASE_SECTORS                 3

/* Flash helper algorithm parameter block struct */
#define CC2538_STATUS_OFFSET 0x0c
struct cc2538_algo_params {
	uint8_t address[4];
	uint8_t length[4];
	uint8_t command[4];
	uint8_t status[4];
};

/* Flash helper algorithm */
const uint8_t cc2538_algo[] = {
#include "../../../contrib/loaders/flash/cc2538/cc2538_algo.inc"
};


#endif /* OPENOCD_FLASH_NOR_CC2538_H */
