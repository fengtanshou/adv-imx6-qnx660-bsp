/*
 * $QNXLicenseC: 
 * Copyright 2007, 2008, QNX Software Systems.  
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




#include "proto.h"

int
_spi_iomsg_cmdread(resmgr_context_t *ctp, io_msg_t *msg, spi_ocb_t *ocb)
{
	spi_msg_t	*spimsg = (spi_msg_t *)msg;
    uint8_t		*buf;
    int			nbytes, cbytes, msglen, status;
	SPIDEV		*drvhdl = (SPIDEV *)ocb->hdr.attr;
	spi_dev_t	*dev = drvhdl->hdl;
	uint16_t	chip = spimsg->device & SPI_DEV_ID_MASK;

	if (chip == SPI_DEV_ID_NONE)
		chip = ocb->chip;

	cbytes = spimsg->msg_hdr.i.combine_len - sizeof(spi_msg_t);
	msglen = spimsg->msg_hdr.i.combine_len + spimsg->xlen;

	if (msglen > ctp->msg_max_size) {
		if (dev->buflen < msglen) {
			dev->buflen = msglen;
			if (dev->buf)
				free(dev->buf);
			if ((dev->buf = malloc(dev->buflen)) == NULL) {
				dev->buflen = 0;
				return ENOMEM;
			}
		}

		status = resmgr_msgread(ctp, dev->buf, spimsg->msg_hdr.i.combine_len, 0);
		if (status < 0)
			return errno;
		if (status < spimsg->msg_hdr.i.combine_len)
			return EFAULT;

		buf = dev->buf;
	}
	else
		buf = (uint8_t *)msg;

	buf += sizeof(spi_msg_t);

	nbytes = cbytes + spimsg->xlen;
	buf = dev->funcs->xfer(drvhdl, _SPI_DEV_EXCHANGE(chip), buf, &nbytes);

	if (nbytes == 0)
		return EAGAIN;

	if (nbytes > cbytes) {
		_IO_SET_READ_NBYTES(ctp, nbytes - cbytes);
		return _RESMGR_PTR(ctp, buf + cbytes, nbytes - cbytes);
	}

	return EIO;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/spi/master/_spi_iomsg_cmdread.c $ $Rev: 680332 $")
#endif
