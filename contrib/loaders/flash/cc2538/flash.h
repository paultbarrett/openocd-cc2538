/******************************************************************************
*
* Copyright (C) 2020 Loci Controls Inc
* Copyright (C) 2016-2018 Texas Instruments Incorporated - http://www.ti.com/
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
*  Redistributions of source code must retain the above copyright
*  notice, this list of conditions and the following disclaimer.
*
*  Redistributions in binary form must reproduce the above copyright
*  notice, this list of conditions and the following disclaimer in the
*  documentation and/or other materials provided with the
*  distribution.
*
*  Neither the name of Texas Instruments Incorporated nor the names of
*  its contributors may be used to endorse or promote products derived
*  from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

#ifndef OPENOCD_LOADERS_FLASH_CC2538_FLASH_H
#define OPENOCD_LOADERS_FLASH_CC2538_FLASH_H

#include <stdint.h>
#include <stdbool.h>

/* Location of flash in memory map */
#define FLASHMEM_BASE 0x00200000

#define FLASH_ERASE_SIZE       2048
#define FLASH_MAX_SECTOR_COUNT 256
#define FLASH_MAX_BYTES        (FLASH_ERASE_SIZE * FLASH_MAX_SECTOR_COUNT)

extern uint32_t flash_sector_erase(uint32_t sector);
extern uint32_t flash_bank_erase(void);
extern uint32_t flash_program(uint8_t *data_buffer, uint32_t address,
	uint32_t count);

static inline uint32_t flash_address_to_sector(uint32_t address) {
        return ((address - FLASHMEM_BASE) / FLASH_ERASE_SIZE);
};
static inline uint32_t flash_sector_to_address(uint32_t sector) {
        return (FLASHMEM_BASE + (sector * FLASH_ERASE_SIZE));
};

#endif /* #ifndef OPENOCD_LOADERS_FLASH_CC2538_FLASH_H */