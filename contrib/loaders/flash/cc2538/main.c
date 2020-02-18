/******************************************************************************
*
* Copyright (C) 2020 Loci Controls Inc
* Copyright (C) 2017-2018 Texas Instruments Incorporated - http://www.ti.com/
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
#include "flashloader.h"

/* Data buffers used by host to communicate with flashloader */

/* Flashloader parameter structure. */
__attribute__ ((section(".buffers.g_cfg")))
volatile struct flash_params g_cfg[2];
/* Data buffer 1. */
__attribute__ ((section(".buffers.g_buf1")))
uint8_t g_buf1[BUFFER_LEN];
/* Data buffer 2. */
__attribute__ ((section(".buffers.g_buf2")))
uint8_t g_buf2[BUFFER_LEN];

/* Buffer used for program with retain feature */
__attribute__ ((section(".buffers.g_retain_buf")))
uint8_t g_retain_buf[BUFFER_LEN];

uint32_t g_curr_buf; /* Current buffer used. */
uint32_t g_vims_ctl; /* Saved flash cache state. */

/******************************************************************************
*
* CC2538 flashloader main function.
*
******************************************************************************/
int main(void)
{
	flashloader_init((struct flash_params *)g_cfg, g_buf1, g_buf2);

	g_curr_buf = 0; /* start with the first buffer */
	uint32_t status;

	while (1) {
		/* Wait for host to signal buffer is ready */
		while (g_cfg[g_curr_buf].full == BUFFER_EMPTY)
			;

		/* Perform requested task */
		switch (g_cfg[g_curr_buf].cmd) {
			case CMD_ERASE_ALL:
				status = flashloader_erase_all();
				break;
			case CMD_PROGRAM:
				status =
					flashloader_program(
						(uint8_t *)g_cfg[g_curr_buf].buf_addr,
						g_cfg[g_curr_buf].dest, g_cfg[g_curr_buf].len);
				break;
			case CMD_ERASE_SECTORS:
				status =
					flashloader_erase_sectors(g_cfg[g_curr_buf].dest,
						g_cfg[g_curr_buf].len);
				break;
			default:
				status = STATUS_FAILED_UNKNOWN_COMMAND;
				break;
		}

		/* Enter infinite loop on error condition */
		if (status != STATUS_OK) {
			g_cfg[g_curr_buf].full = status;
			while (1)
				;
		}

		/* Mark current task complete, and begin looking at next buffer */
		g_cfg[g_curr_buf].full = BUFFER_EMPTY;
		g_curr_buf ^= 1;
	}
}

void _exit(int status)
{
	/* Enter infinite loop on hitting an exit condition */
	(void)status; /* Unused parameter */
	while (1)
		;
}
