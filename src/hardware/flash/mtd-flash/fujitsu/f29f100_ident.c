/*
 * $QNXLicenseC:
 * Copyright 2008, QNX Software Systems. 
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"). You 
 * may not reproduce, modify or distribute this software except in 
 * compliance with the License. You may obtain a copy of the License 
 * at: http://www.apache.org/licenses/LICENSE-2.0 
 * 
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" basis, 
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as 
 * contributors under the License or as licensors under other terms.  
 * Please review this entire file for other proprietary rights or license 
 * notices, as well as the QNX Development Suite License Guide at 
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */





#include <sys/f3s_mtd.h>

/*
 * Summary
 *
 * Method:      "JEDEC" ID only
 * MTD Version: 1 and 2
 * Bus Width:   8-bit and 16-bit
 * Boot-Block?: Yes
 *
 * Description
 *
 * This ident callout uses the "JEDEC" ID code to recognize chips with
 * non-uniform block sizes (aka boot-block flash).
 *
 * Use this for really old legacy flash chips. If the ID for your chip is not
 * listed, copy this file into your driver's source directory and add an entry
 * for your chip.
 */

int32_t f3s_f29f100_ident(f3s_dbase_t *dbase,
                          f3s_access_t *access,
                          uint32_t flags,
                          uint32_t offset)
{
	volatile uint8_t *	memory;
	uintptr_t			amd_cmd1, amd_cmd2;
	F3S_BASETYPE		jedec_hi, jedec_lo;
	int32_t				geo, size;

	static f3s_dbase_t *probe;
	static f3s_dbase_t virtual[] =
	{
		{sizeof(f3s_dbase_t), 0xffff, 0x04, 0xc4, "MBM29LV160-T",
			F3S_ERASE_FOR_ALL, 1, 1, 10000, 1000000000, 3000, 3000,
		1, 1000000, 0, 0, 0, 4, {{31, 16}, {1, 15}, {2, 13}, {1, 14}}},

		{sizeof(f3s_dbase_t), 0xffff, 0x04, 0x22c4, "MBM29LV160-T",
			F3S_ERASE_FOR_ALL, 1, 1, 10000, 1000000000, 3000, 3000,
		1, 1000000, 0, 0, 0, 4, {{31, 16}, {1, 15}, {2, 13}, {1, 14}}}
	};

	if (flashcfg.device_width == 1) {
		amd_cmd1 = AMD_CMD_ADDR1_W8;
		amd_cmd2 = AMD_CMD_ADDR2_W8;
	} else {
		amd_cmd1 = AMD_CMD_ADDR1_W16;
		amd_cmd2 = AMD_CMD_ADDR2_W16;
	}
	amd_cmd1 *= flashcfg.bus_width;
	amd_cmd2 *= flashcfg.bus_width;

	/* Check listing flag */
	if (flags & F3S_LIST_ALL)
	{
		/* If first pass */
		if (!probe) probe = virtual;

		/* If dbase is valid */
		if (!probe->struct_size) return (ENOENT);

		*dbase = *probe;
		probe++;

		return (EOK);
	}

	/* Set necessary ident size */
	size = amd_cmd1 + flashcfg.bus_width;

	/* Set proper page on socket */
	memory = access->service->page(&access->socket, F3S_POWER_ALL, offset & amd_command_mask, &size);
	if (memory == NULL) return (ERANGE);

	/* If size is ok */
	if (size < (amd_cmd1 + flashcfg.bus_width)) return (ENOTSUP);

	/* Issue unlock cycles */
	send_command(memory + amd_cmd1, AMD_UNLOCK_CMD1);
	send_command(memory + amd_cmd2, AMD_UNLOCK_CMD2);

	/* Issue read ident command */
	send_command(memory + amd_cmd1, AMD_AUTOSELECT);

	probe = virtual;

	/* While there are dbase entries */
	while (probe->struct_size) {
		/* Read jedecs */
		jedec_hi = probe->jedec_hi * flashcfg.device_mult;
		jedec_lo = probe->jedec_lo * flashcfg.device_mult;

		/* Compare jedecs */
		if ((jedec_hi == readmem(memory)) &&
		    (jedec_lo == readmem(memory + flashcfg.bus_width)))
		{
			*dbase = *probe;
			dbase->chip_inter = flashcfg.chip_inter;

			/* for all geometries */
			for (geo = 0; geo < dbase->geo_num; geo++) {
				dbase->geo_vect[geo].unit_pow2 += (flashcfg.chip_inter >> 1);
			}

			return (EOK);
		}

		/* Go to next dbase entry */
		probe++;
	}

	/* Flash type is not amd */
	return (ENOTSUP);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/flash/mtd-flash/fujitsu/f29f100_ident.c $ $Rev: 718635 $")
#endif
