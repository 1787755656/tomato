/*
 * Write-once support for otp.
 *
 * Copyright 2006, Broadcom Corporation
 * All Rights Reserved.                
 *                                     
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;   
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior      
 * written permission of Broadcom Corporation.                            
 *
 * $Id$
 */


extern void *otp_init(sb_t *sbh);
extern uint16 otpr(void *oh, chipcregs_t *cc, uint wn);
extern int otp_status(void *oh);
extern int otp_size(void *oh);

#ifdef BCMNVRAMW
extern int otp_write_region(void *oh, int region, uint16 *data, uint wlen);

extern int otp_nvwrite(void *oh, uint16 *data, uint wlen);
#endif

#if	defined(WLTEST)
extern int otp_dump(void *oh, int arg, char *buf, uint size);
#endif
