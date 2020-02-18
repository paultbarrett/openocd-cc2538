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

#include <stdint.h>
#include <stdbool.h>
#include "flash.h"

struct rom_api {
        unsigned long (*Crc32)(unsigned char *pucData,
                               unsigned long ulByteCount);
        unsigned long (*GetFlashSize)(void);
        unsigned long (*GetChipId)(void);
        long (*PageErase)(unsigned long ulStartAddress,
                          unsigned long ulEraseSize);
        long (*ProgramFlash)(unsigned long *pulData,
                             unsigned long ulAddress,
                             unsigned long ulCount);
} *ROM = (struct rom_api *)0x00000048;

extern uint32_t flash_sector_erase(uint32_t sector_address)
{
        long ret;
        bool already_erased = true;
        int i;

        for (i = 0; i < FLASH_ERASE_SIZE; i += 4) {
                if ((*(uint32_t *)(sector_address + i)) != 0xffffffff) {
                        already_erased = false;
                        break;
                }
        }
        if (already_erased)
                return 0;

        ret = ROM->PageErase(sector_address, FLASH_ERASE_SIZE);
        if (ret == -1)
                return 0x101;
        if (ret == -2)
                return 0x102;
        if (ret != 0)
                return 0x103;
        return 0;
}

extern uint32_t flash_bank_erase(void)
{
        int i;
        long ret;
        /* Some pages may be locked by bits in the CCA.  Erasing the CCA
           first will ensure that those pages will become unlocked.
           So, since the CCA is the last page in flash, let's just erase
           sectors backwards. */
        int num_sectors = ROM->GetFlashSize() / FLASH_ERASE_SIZE;
        for (i = num_sectors - 1; i >= 0; i--)
        {
                ret = flash_sector_erase(flash_sector_to_address(i));
                if (ret != 0)
                        return ret;
        }
        return 0;
}

extern uint32_t flash_program(uint8_t *data_buffer, uint32_t address,
                              uint32_t count)
{
        long ret;
        ret = ROM->ProgramFlash((unsigned long *)data_buffer, address, count);
        if (ret == -1)
                return 0x104;
        if (ret == -2)
                return 0x105;
        if (ret != 0)
                return 0x106;
        return 0;
}
