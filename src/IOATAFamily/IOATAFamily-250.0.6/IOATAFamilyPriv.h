/*
 * Copyright (c) 2009 Apple, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 * 
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */


#ifndef	_IO_ATA_FAMILY_PRIV_H
#define	_IO_ATA_FAMILY_PRIV_H


/*!
 
 @header IOATAFamilyPriv.h
 @discussion contains various private definitions and constants for use in the IOATAFamily and clients. Header Doc is incomplete at this point, but file is heavily commented.
 
 */


/* The ATA private Event codes */
/* sent when calling the device driver's event handler */

enum ataPrivateEventCode
{
	
	kATANewMediaEvent			= 0x80		/* Someone has added new media to the drive */
	
};


#endif	_IO_ATA_FAMILY_PRIV_H

