/*
 * Copyright (c) 1998-2009 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 2.0 (the
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

#include <i386/cpuid.h>
#include <IOKit/IOPlatformExpert.h>
#include <IOKit/IOMemoryDescriptor.h>
#include <IOKit/IODeviceTreeSupport.h>
#include <IOKit/pci/IOPCIBridge.h>   // IOPCIAddressSpace
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <string.h>
#include <libkern/version.h>
#include "AppleSMBIOS.h"

#if     DEBUG
#define DEBUG_LOG(args...)  IOLog(args)
#else
#define DEBUG_LOG(args...)
#endif

//---------------------------------------------------------------------------
// SMBPackedStrings class

class SMBPackedStrings 
{
protected:
    const char * _start;
    const char * _limit;

public:
    SMBPackedStrings( const SMBStructHeader * header, const void * limit );
    SMBPackedStrings( const SMBStructHeader * header );

    const char * stringAtIndex( UInt8 index, UInt8 * length = 0 ) const;

    void setDataProperty( IORegistryEntry * entry,
                          const char *      key,
                          UInt8             index ) const;

    void setStringProperty( IORegistryEntry * entry,
                            const char *      key,
                            UInt8             index ) const;
};

SMBPackedStrings::SMBPackedStrings( const SMBStructHeader * header,
                                    const void *            limit )
{
    _start = (const char *) header + header->length;
    _limit = (const char *) limit;
}

SMBPackedStrings::SMBPackedStrings( const SMBStructHeader * header )
{
    _start = (const char *) header + header->length;
	
	const char * cp = _start;
	// Find the double null at the end of the record
	while( cp[0] || cp[1]) cp++;
	
	_limit = &cp[1];
}
    
const char * SMBPackedStrings::stringAtIndex( UInt8 index, UInt8 * length ) const
{
    const char * last = 0;
    const char * next = _start;

    if ( length ) *length = 0;

    while ( index-- )
    {
        last = 0;
        for ( const char * cp = next; cp < _limit; cp++ )
        {
            if ( *cp == '\0' )
            {
                last = next;
                next = cp + 1;
                break;
            }
        }
        if ( last == 0 ) break;
    }

    if ( last )
    {
        while (*last == ' ') last++;
        if (length)
        {
            UInt8 len;
            for ( len = next - last - 1; len && last[len - 1] == ' '; len-- )
                ;
            *length = len; // number of chars not counting the terminating NULL
        }
    }

    return last ? last : "";
}

void SMBPackedStrings::setDataProperty( IORegistryEntry * entry,
                                        const char *      key,
                                        UInt8             index ) const
{
    UInt8        length;
    const char * string = SMBPackedStrings::stringAtIndex(index, &length);
    
    if (length)
    {
        OSData * data = OSData::withCapacity(length + 1);
        if (data)
        {
            data->appendBytes(string, length);
            data->appendByte('\0', 1);
            entry->setProperty(key, data);
            data->release();
        }
    }
}

void SMBPackedStrings::setStringProperty( IORegistryEntry * entry,
                                          const char *      key,
                                          UInt8             index ) const
{
    UInt8        length;
    const char * string = SMBPackedStrings::stringAtIndex(index, &length);

    if (length)
    {
        OSString * strObj = OSString::withCString(string);
        if (strObj)
        {
            entry->setProperty(key, strObj);
            strObj->release();
        }
    }
}


#define super IOService

OSDefineMetaClassAndStructors( AppleSMBIOS, IOService )

//---------------------------------------------------------------------------

bool AppleSMBIOS::start( IOService * provider )
{
    OSSerializer * serializer;

    if (super::start(provider) != true       ||
        IOService::getResourceService() == 0 ||
        IOService::getResourceService()->getProperty("SMBIOS"))
    {
        return false;
    }
	
	SMBIOSTable = NULL;
	SMBIOSTableLength = 0;

    fSlotQueueHead = IONew(queue_head_t, 1);
    if (!fSlotQueueHead)
        return false;

    queue_init(fSlotQueueHead);

    // Get the IOPlatformExpertDevice

    fRoot = getServiceRoot();
    if (!provider || !fRoot)
        return false;

    // Serialize SMBIOS structures to user-space on demand

    serializer = OSSerializer::forTarget((void *) this, &serializeSMBIOS);
    if (!serializer)
        return false;

    setProperty("SMBIOS", serializer);

    memInfoSource = kNoMemoryInfo;
    memSlotsData  = OSData::withCapacity(kMemDataSize);
    memTypesData  = OSData::withCapacity(kMemDataSize);
    memSizesData  = OSData::withCapacity(kMemDataSize);
    memSpeedData  = OSData::withCapacity(kMemDataSize);
    memInfoData   = OSData::withCapacity(kMemDataSize);
    memManufData  = OSData::withCapacity(kMemDataSize);
    memSerialData = OSData::withCapacity(kMemDataSize);
    memPartData   = OSData::withCapacity(kMemDataSize);
    memSizeTotal  = 0;

    if (!memSlotsData || !memTypesData || !memSizesData || !memSpeedData ||
        !memInfoData || !memManufData || !memSerialData || !memPartData)
        return false;

	if (!findSMBIOSTableEFI())
	{
		return false;
	}

    // Update device tree
    updateDeviceTree();

    publishResource("SMBIOS");
    registerService();
	
    return true;
}

//---------------------------------------------------------------------------

#define RELEASE(x) do { if (x) { (x)->release(); (x) = 0; } } while(0)

void AppleSMBIOS::free( void )
{
    RELEASE( memSlotsData  );
    RELEASE( memTypesData  );
    RELEASE( memSizesData  );
    RELEASE( memSpeedData  );
    RELEASE( memInfoData   );
    RELEASE( memManufData  );
    RELEASE( memSerialData );
    RELEASE( memPartData   );
    RELEASE( fDMIMemoryMap );

    if (fSlotQueueHead)
    {
        SystemSlotEntry * slotEntry;

        while (!queue_empty(fSlotQueueHead))
        {
            queue_remove_first(fSlotQueueHead, slotEntry, SystemSlotEntry *,
                chain);
            IODelete(slotEntry, SystemSlotEntry, 1);
        }
        IODelete(fSlotQueueHead, queue_head_t, 1);
        fSlotQueueHead = 0;
    }

    super::free();
}

//---------------------------------------------------------------------------

bool AppleSMBIOS::
serializeSMBIOS( void * target, void * refcon, OSSerialize * s )
{
    AppleSMBIOS *   me = (AppleSMBIOS *) target;
    OSData *        data;
    IOMemoryMap *   map;
    bool            ok = false;

    map = me->fDMIMemoryMap;
    if (map)
    {
        data = OSData::withBytesNoCopy(
                (void *) map->getVirtualAddress(), map->getLength());

        if (data)
        {
            ok = data->serialize(s);
            data->release();
        }
    }

    return ok;
}

//---------------------------------------------------------------------------

static UInt8 checksum8( void * start, UInt length )
{
    UInt8   csum = 0;
    UInt8 * cp = (UInt8 *) start;

    for (UInt i = 0; i < length; i++)
        csum += *cp++;

    return csum;
}

#define kGenericPCISlotName     "PCI Slot"

//---------------------------------------------------------------------------

OSData * AppleSMBIOS::
getSlotNameWithSlotId( int slotId )
{
    char                name[80];
    SystemSlotEntry *   slot = 0;
    SystemSlotEntry *   iter;

	queue_iterate(fSlotQueueHead, iter, SystemSlotEntry *, chain)
	{
		if ((iter->slotID & 0xff) == slotId)
		{
			slot = iter;
			break;
		}
	}

	if (slot && slot->slotName)
	{
		strncpy(name, slot->slotName, sizeof(name));
	}
	else
	{
		// No matching SlotId, return a generic PCI slot name
		snprintf(name, sizeof(name), "%s %u", kGenericPCISlotName, slotId);
	}

	name[sizeof(name) - 1] = '\0';
    return OSData::withBytes(name, strlen(name) + 1);
}

#define EFI_SMBIOS_TABLE \
"/efi/configuration-table/EB9D2D31-2D88-11D3-9A16-0090273FC14D"

//---------------------------------------------------------------------------

bool AppleSMBIOS::findSMBIOSTableEFI( void )
{
	IORegistryEntry *		tableEntry;
	OSData *				tableData;
	UInt64					tableAddr;
	IOMemoryDescriptor *    epsMemory;
	SMBEntryPoint			eps;
	IOMemoryDescriptor *    dmiMemory = 0;
	IOItemCount             dmiStructureCount = 0;

	tableEntry = fromPath(EFI_SMBIOS_TABLE, gIODTPlane);	
	if (tableEntry)
	{		
		tableAddr = 0;
		tableData = OSDynamicCast(OSData, tableEntry->getProperty("table"));
		if (tableData && (tableData->getLength() <= sizeof(tableAddr)))
		{
			bcopy(tableData->getBytesNoCopy(), &tableAddr, tableData->getLength());

			// For SnowLeopard and beyond include the kIOMemoryMapperNone option.
#if VERSION_MAJOR >= 10
			IOOptionBits options = kIODirectionOutIn | kIOMemoryMapperNone;
#else
			IOOptionBits options = kIODirectionOutIn;
#endif
			epsMemory = IOMemoryDescriptor::withAddressRange(
						(mach_vm_address_t) tableAddr,
						(mach_vm_size_t) sizeof(eps),
						options,
						NULL );

			if (epsMemory)
			{
				bzero(&eps, sizeof(eps));
				epsMemory->readBytes(0, &eps, sizeof(eps));
				setProperty("SMBIOS-EPS", (void *) &eps, sizeof(eps));

				if (memcmp(eps.anchor, "_SM_", 4) == 0)
				{
					UInt8 csum;

					csum = checksum8(&eps, sizeof(eps));

					DEBUG_LOG("DMI checksum       = 0x%x\n", csum);
					DEBUG_LOG("DMI tableLength    = %d\n",
						eps.dmi.tableLength);
					DEBUG_LOG("DMI tableAddress   = 0x%x\n",
						(uint32_t) eps.dmi.tableAddress);
					DEBUG_LOG("DMI structureCount = %d\n",
						eps.dmi.structureCount);
					DEBUG_LOG("DMI bcdRevision    = %x\n",
						eps.dmi.bcdRevision);

					if (csum == 0 && eps.dmi.tableLength &&
						eps.dmi.structureCount)
					{
						dmiStructureCount = eps.dmi.structureCount;
						dmiMemory = IOMemoryDescriptor::withPhysicalAddress(
									eps.dmi.tableAddress, eps.dmi.tableLength,
									kIODirectionOutIn );
					}
					else
					{
						DEBUG_LOG("No DMI structure found\n");
					}
				}

				epsMemory->release();
				epsMemory = 0;
			}
		}

		tableEntry->release();
	}

    if ( dmiMemory )
    {
        fDMIMemoryMap = dmiMemory->map();
        if (fDMIMemoryMap)
        {
			SMBIOSTable = (void *) fDMIMemoryMap->getVirtualAddress();
			SMBIOSTableLength =  fDMIMemoryMap->getLength();
			
            decodeSMBIOSTable((void *) fDMIMemoryMap->getVirtualAddress(),
                fDMIMemoryMap->getLength(), dmiStructureCount );
        }
        dmiMemory->release();
        dmiMemory = 0;
    }

    return (fDMIMemoryMap != 0);
}

//---------------------------------------------------------------------------

void AppleSMBIOS::
adjustPCIDeviceEFI( IOService * pciDevice )
{
	IOACPIPlatformDevice *	acpiDevice;
	UInt32					slotNum;
	OSString *				acpiPath;
	IORegistryEntry *		acpiEntry = NULL;

	// Does PCI device have an ACPI alias?
	// N : not built-in, no slot name, exit
	// Y : goto next
	//
	// Does ACPI device have a _SUN object?
	// N : it's a built in PCI device
	// Y : indicates a slot (not built-in), goto next
	//
	// Match _SUN value against SlotId in SMBIOS PCI slot structures

	do {
		acpiPath = OSDynamicCast(OSString, pciDevice->getProperty("acpi-path"));
		if (!acpiPath)
			break;

		acpiEntry = fromPath(acpiPath->getCStringNoCopy());
		if (!acpiEntry)
			break;

		if (!acpiEntry->metaCast("IOACPIPlatformDevice"))
			break;
		
		acpiDevice = (IOACPIPlatformDevice *) acpiEntry;

		if (acpiDevice->evaluateInteger("_SUN", &slotNum) == kIOReturnSuccess)
		{
			OSObject * name = getSlotNameWithSlotId(slotNum);
			if (name)
			{
				pciDevice->setProperty("AAPL,slot-name", name);
				name->release();
			}
		}
		else if (acpiDevice->validateObject("_RMV") == kIOReturnSuccess)
		{
			// no slot name?
		}
		else
		{
			char dummy = '\0';
			pciDevice->setProperty("built-in", &dummy, 1);
		}
	} while (false);

	if (acpiEntry)
		acpiEntry->release();
}

//---------------------------------------------------------------------------

IOReturn AppleSMBIOS::
callPlatformFunction( const char * functionName,
                      bool waitForFunction,
                      void * param1, void * param2,
                      void * param3, void * param4 )
{
    if (!functionName)
        return kIOReturnBadArgument;

	// AdjustPCIBridge function is called by the ACPI
	// platform driver, but is not useful on EFI systems.

    if (!strcmp(functionName, "AdjustPCIDevice"))
    {
        IOService * device = (IOService *) param1;

        if (device)
        {
			adjustPCIDeviceEFI(device);
			return kIOReturnSuccess;
        }
    }

    return kIOReturnUnsupported;
}

//---------------------------------------------------------------------------

void AppleSMBIOS::decodeSMBIOSTable( const void * tableData,
                                     UInt16       tableLength,
                                     UInt16       structureCount )
{
    const SMBStructHeader * header;
    const UInt8 *           next = (const UInt8 *) tableData;
    const UInt8 *           end  = next + tableLength;

    while ( structureCount-- && (end > next + sizeof(SMBStructHeader)) )
    {
        header = (const SMBStructHeader *) next;
        if (header->length > end - next) break;

        decodeSMBIOSStructure( header, end );

        // Skip the formatted area of the structure.

        next += header->length;

        // Skip the unformatted structure area at the end (strings).
        // Look for a terminating double NULL.

        for ( ; end > next + sizeof(SMBStructHeader); next++ )
        {
            if ( next[0] == 0 && next[1] == 0 )
            {
                next += 2; break;
            }
        }
    }
}

//---------------------------------------------------------------------------

void AppleSMBIOS::
decodeSMBIOSStructure( const SMBStructHeader * structureHeader,
                       const void *            tableBoundary )
{
    const union SMBStructUnion {
        SMBBIOSInformation      bios;
        SMBSystemInformation    system;
		SMBBaseBoard			baseBoard;
        SMBMemoryModule         memoryModule;
        SMBSystemSlot           slot;
        SMBPhysicalMemoryArray  memoryArray;
        SMBMemoryDevice         memoryDevice;
		SMBFirmwareVolume       fv;
		SMBMemorySPD            spd;
    } * u = (const SMBStructUnion *) structureHeader;

    SMBPackedStrings strings = SMBPackedStrings( structureHeader,
                                                 tableBoundary );

    switch ( structureHeader->type )
    {
        case kSMBTypeBIOSInformation:
            processSMBIOSStructureType0( &u->bios, &strings );
            break;

        case kSMBTypeSystemInformation:
            processSMBIOSStructureType1( &u->system, &strings );
            break;

		case kSMBTypeBaseBoard:
			processSMBIOSStructureType2( &u->baseBoard, &strings );
			break;
			
        case kSMBTypeMemoryModule:
            processSMBIOSStructureType6( &u->memoryModule, &strings );
            break;

        case kSMBTypeSystemSlot:
            processSMBIOSStructureType9( &u->slot, &strings );
            break;

        case kSMBTypePhysicalMemoryArray:
            processSMBIOSStructureType16( &u->memoryArray, &strings );
            break;

        case kSMBTypeMemoryDevice:
            processSMBIOSStructureType17( &u->memoryDevice, &strings );
            break;

        case kSMBTypeFirmwareVolume:
			processSMBIOSStructureType128( &u->fv, &strings );
			break;
		
		case kSMBTypeMemorySPD:
			processSMBIOSStructureType130( &u->spd, &strings );
			break;
    }
}

//---------------------------------------------------------------------------

bool AppleSMBIOS::findSMBIOSStructure(
    SMBAnchor * anchor, uint8_t inType, uint32_t minSize )
{
    const SMBStructHeader *	header;
    const uint8_t *			next;
    const uint8_t *			end;
	bool					found = false;

	if (!fDMIMemoryMap || !anchor)
		return false;

	if (anchor->next == NULL)
		next = (const UInt8 *) fDMIMemoryMap->getVirtualAddress();
	else
		next = anchor->next;

	end = (const UInt8 *) fDMIMemoryMap->getVirtualAddress() +
		fDMIMemoryMap->getLength();

    while (end > next + sizeof(SMBStructHeader))
    {
        header = (const SMBStructHeader *) next;
        if (header->length > end - next) break;

		// Skip the formatted area of the structure
		next += header->length;

		// Skip the unformatted structure area at the end (strings).
		// Look for a terminating double NULL.

		for ( ; end > next + sizeof(SMBStructHeader); next++ )
		{
			if ( next[0] == 0 && next[1] == 0 )
			{
				next += 2;
				break;
			}
		}

		if ((header->type == inType) && (header->length >= minSize))
		{
			anchor->header = header;
			anchor->next   = next;
			anchor->end    = end;
			found = true;
			break;
		}
    }

	return found;
}

//---------------------------------------------------------------------------

void AppleSMBIOS::processSMBIOSStructureType0(
    const SMBBIOSInformation * bios,
    SMBPackedStrings * strings )
{
    char location[9];

    if (bios->header.length < sizeof(SMBBIOSInformation))
        return;

	if (!fROMNode)
	{
		fROMNode = OSTypeAlloc( IOService );
		if (fROMNode && (false == fROMNode->init()))
		{
			fROMNode->release();
			fROMNode = 0;
		}
	}
    if (fROMNode)
    {
        snprintf(location, sizeof(location), "%x", bios->startSegment << 4);
        fROMNode->setLocation(location);

        strings->setDataProperty(fROMNode, "vendor", bios->vendor);
        strings->setDataProperty(fROMNode, "version", bios->version);
        strings->setDataProperty(fROMNode, "release-date", bios->releaseDate);
        strings->setDataProperty(fROMNode, "characteristics",
                                           bios->characteristics);

        fROMNode->setProperty("rom-size", (bios->romSize + 1) * 0x10000, 32 );
    }
}

//---------------------------------------------------------------------------

void AppleSMBIOS::processSMBIOSStructureType1(
    const SMBSystemInformation * sys,
    SMBPackedStrings * strings )
{
	UInt8 length;
	
    if (sys->header.length < 8)
        return;

    strings->setDataProperty(fRoot, "manufacturer",  sys->manufacturer);
    strings->setDataProperty(fRoot, "product-name",  sys->productName);
    strings->setDataProperty(fRoot, "version",       sys->version);

    const char *serialNumberString = strings->stringAtIndex(sys->serialNumber, &length);
	// The serial-number property in the IORegistry is a 43-byte data object.
	// Bytes 0 through 2 are the last three bytes of the serial number string.
	// Bytes 11 through 20, inclusive, are the serial number string itself.
	// All other bytes are '\0'.
	OSData * data = OSData::withCapacity(43);
	if (data)
	{
		data->appendBytes(serialNumberString + (length - 3), 3);
		data->appendBytes(NULL, 10);
		data->appendBytes(serialNumberString, length);
		data->appendBytes(NULL, 43 - length - 10 - 3);
		fRoot->setProperty("serial-number", data);
		data->release();
	}

	strings->setStringProperty(fRoot, kIOPlatformSerialNumberKey, sys->serialNumber);
}


//---------------------------------------------------------------------------

void AppleSMBIOS::processSMBIOSStructureType2(
    const SMBBaseBoard * baseBoard,
    SMBPackedStrings * strings )
{
    if (baseBoard->header.length < sizeof(SMBBaseBoard))
	{
		kprintf("AppleSMBIOS: invalid type 2 record size: %d, expected %ld\n",
				baseBoard->header.length, sizeof(SMBBaseBoard)+1);
		return;
	}

	// If this type 2 record is for the processor and memory board then it is
	// a riser card. Grab the serial number and publish it to the registry.
	if ((baseBoard->boardType & kSMBBaseBoardProcessorMemoryModule) == kSMBBaseBoardProcessorMemoryModule)
	{
		UInt8 length = 0;
		strings->stringAtIndex(baseBoard->serialNumber, &length);
		if (length == 0)
			IOLog("AppleSMBIOS: processor/memory board serial number is empty\n");
		else
			strings->setStringProperty(fRoot, "processor-memory-board-serial-number", baseBoard->serialNumber);
	}
}


//---------------------------------------------------------------------------

void AppleSMBIOS::processSMBIOSStructureType6(
    const SMBMemoryModule * memory,
    SMBPackedStrings * strings )
{
    UInt8        socketLength;
    const char * socketString;
    UInt8        memorySize;

    union {
        UInt64 ull;
        UInt32 ul[2];
    } memoryBytes;

    if (memory->header.length < sizeof(SMBMemoryModule))
        return;

    if (memInfoSource == kMemoryDeviceInfo)
        return;

    memInfoSource = kMemoryModuleInfo;

    // update memSlotsData

    socketString = strings->stringAtIndex( memory->socketDesignation, 
                                           &socketLength );

    if ( socketString )
    {
        if (memSlotsData->getLength() == 0)
            memSlotsData->appendBytes("   ", 4);
        if (socketLength)
            memSlotsData->appendBytes( socketString, socketLength );
        memSlotsData->appendByte('\0', 1);
    }

    // update memTypesData

    memTypesData->appendBytes("DRAM", 5);

    // update memSizesData

    memorySize = memory->enabledSize & 0x7F;
    if (memorySize >= kSMBMemoryModuleSizeNotDeterminable)
        memoryBytes.ull = 0;
    else
        memoryBytes.ull = (1ULL << memorySize) * (1024 * 1024);

    memSizeTotal += memoryBytes.ull;
    memSizesData->appendBytes( &memoryBytes.ul[1], 4 );
    memSizesData->appendBytes( &memoryBytes.ul[0], 4 );
}

//---------------------------------------------------------------------------

void AppleSMBIOS::processSMBIOSStructureType9(
    const SMBSystemSlot * slot,
    SMBPackedStrings * strings )
{
    SystemSlotEntry * slotEntry;

    if (slot->header.length < 12)
        return;

    slotEntry = IONew(SystemSlotEntry, 1);
    if (slotEntry)
    {
        memset(slotEntry, 0, sizeof(*slotEntry));

        slotEntry->slotID    = slot->slotID;
        slotEntry->slotType  = slot->slotType;
        slotEntry->slotUsage = slot->currentUsage;
        slotEntry->slotName  = strings->stringAtIndex(slot->slotDesignation);

        queue_enter(fSlotQueueHead, slotEntry, SystemSlotEntry *, chain);
    }

    DEBUG_LOG("Slot type %x, width %x, usage %x, ID %x, char1 %x\n",
        slot->slotType, slot->slotDataBusWidth, slot->currentUsage,
        slot->slotID, slot->slotCharacteristics1);
}

//---------------------------------------------------------------------------

void AppleSMBIOS::processSMBIOSStructureType16(
    const SMBPhysicalMemoryArray * memory,
    SMBPackedStrings * strings )
{
    if (memory->header.length < sizeof(SMBPhysicalMemoryArray))
        return;

    if ((memory->arrayUse == kSMBMemoryArrayUseSystemMemory) &&
        ((memory->errorCorrection == kSMBMemoryArrayErrorCorrectionTypeSingleBitECC) ||
         (memory->errorCorrection == kSMBMemoryArrayErrorCorrectionTypeMultiBitECC)))
    {
        memECCEnabled = true;
    }
}

//---------------------------------------------------------------------------

void AppleSMBIOS::processSMBIOSStructureType17(
    const SMBMemoryDevice * memory,
    SMBPackedStrings * strings )
{
    UInt8        deviceLocatorLength;
    const char * deviceLocatorString;
    UInt8        bankLocatorLength;
    const char * bankLocatorString;
    UInt8        stringLength;
    const char * string;
    UInt8        memoryType;

    union {
        UInt64 ull;
        UInt32 ul[2];
    } memoryBytes;

    if (memory->header.length < 21)
        return;

    if (memInfoSource == kMemoryModuleInfo)
    {
        memSlotsData->initWithCapacity(kMemDataSize);
        memTypesData->initWithCapacity(kMemDataSize);
        memSizesData->initWithCapacity(kMemDataSize);
        memSpeedData->initWithCapacity(kMemDataSize);
        memManufData->initWithCapacity(kMemDataSize);
        memSerialData->initWithCapacity(kMemDataSize);
        memPartData->initWithCapacity(kMemDataSize);
        memSizeTotal = 0;
    }

    memInfoSource = kMemoryDeviceInfo;

    // update memSlotsData

    deviceLocatorString = strings->stringAtIndex( memory->deviceLocator, 
                                                  &deviceLocatorLength );

    bankLocatorString = strings->stringAtIndex( memory->bankLocator, 
                                                &bankLocatorLength );

    // Device location is mandatory, but bank location is optional.

	if (!memory->deviceLocator || !deviceLocatorLength )
	{
        // Dummy device location string
		deviceLocatorString = "Location";
		deviceLocatorLength = strlen(deviceLocatorString);
	}

    if ( deviceLocatorLength )
    {
        if ( memSlotsData->getLength() == 0 )
		   memSlotsData->appendBytes("   ", 4);
        if ( bankLocatorLength )
        {
            memSlotsData->appendBytes( bankLocatorString, bankLocatorLength );
            memSlotsData->appendByte('/', 1);
        }
        memSlotsData->appendBytes( deviceLocatorString, deviceLocatorLength );
        memSlotsData->appendByte('\0', 1);
    }

    // update memTypesData

	memoryType = memory->memoryType;
	if ( memoryType > kSMBMemoryDeviceTypeCount - 1 )
		memoryType = 0x02; // unknown type

	memTypesData->appendBytes( SMBMemoryDeviceTypes[memoryType],
						strlen(SMBMemoryDeviceTypes[memoryType]) + 1 );


    // update memSizesData

    memoryBytes.ull = (memory->memorySize & 0x7fff) * 1024;
    if ((memory->memorySize & 0x8000) == 0)
        memoryBytes.ull *= 1024;

    memSizeTotal += memoryBytes.ull;
    memSizesData->appendBytes( &memoryBytes.ul[1], 4 );
    memSizesData->appendBytes( &memoryBytes.ul[0], 4 );

    if (memory->header.length >= 27)
    {
        char speedText[16];

		snprintf(speedText, sizeof(speedText), "%u MHz", memory->memorySpeed);
		memSpeedData->appendBytes(speedText, strlen(speedText) + 1);
    }

    string = strings->stringAtIndex( memory->manufacturer, &stringLength );
	memManufData->appendBytes( string, stringLength + 1 );

    string = strings->stringAtIndex( memory->serialNumber, &stringLength );
	memSerialData->appendBytes( string, stringLength + 1 );

    string = strings->stringAtIndex( memory->partNumber, &stringLength );
	memPartData->appendBytes( string, stringLength + 1 );

    // What about "available", "mem-info" prop?
}

//---------------------------------------------------------------------------

void AppleSMBIOS::processSMBIOSStructureType128(
    const SMBFirmwareVolume *   fv,
    SMBPackedStrings *          strings )
{
	const FW_REGION_INFO * regionInfo = NULL;

    if (fv->header.length < sizeof(SMBFirmwareVolume))
        return;

	for (int i = 0; i < fv->RegionCount; i++)
	{
		if (fv->RegionType[i] == FW_REGION_MAIN)
		{
			regionInfo = &fv->FlashMap[i];
			break;
		}
	}

	if (regionInfo && (regionInfo->EndAddress > regionInfo->StartAddress))
	{
		if (!fROMNode)
		{
			fROMNode = OSTypeAlloc( IOService );
			if (fROMNode && (false == fROMNode->init()))
			{
				fROMNode->release();
				fROMNode = 0;
			}
		}
		if (fROMNode)
		{
			fROMNode->setProperty("fv-main-address",
				regionInfo->StartAddress, 32 );

			fROMNode->setProperty("fv-main-size",
				regionInfo->EndAddress - regionInfo->StartAddress + 1, 32 );
		}
	}
}

//---------------------------------------------------------------------------

void AppleSMBIOS::processSMBIOSStructureType130(
    const SMBMemorySPD *    spd,
    SMBPackedStrings *      strings )
{
	unsigned int dataSize;
	
	if(spd->Offset > 127) return; // Only care about the first 128 bytes of spd data
	
	dataSize = (spd->Size + spd->Offset) > 128 ? 128 - spd->Offset : spd->Size;
	memInfoData->appendBytes(spd->Data, dataSize);

}

//---------------------------------------------------------------------------

void AppleSMBIOS::updateDeviceTree( void )
{
	SMBAnchor		anchor;
    uint32_t        busSpeedMTs = 0;
    uint32_t        itcSpeedMTs = 0;
    uint32_t        cpuSpeedMHz = 0;
    SMBWord         cpuType     = 0;

    IOService * memoryNode = OSTypeAlloc( IOService );

    if (memoryNode && (false == memoryNode->init()))
    {
        memoryNode->release();
        memoryNode = 0;
    }
    if (memoryNode)
    {
        memoryNode->setName("memory");
        //memoryNode->setLocation("0");
        memoryNode->setProperty( "slot-names",  memSlotsData );
        memoryNode->setProperty( "dimm-types",  memTypesData );
        memoryNode->setProperty( "reg",         memSizesData );
        if (memSpeedData->getLength())
        {
            memoryNode->setProperty( "dimm-speeds", memSpeedData );
		}

		if (memInfoData->getLength() == 0)
		{
            memInfoData->appendBytes(0, (memSizesData->getLength() / 8) * 128);
		}

		memoryNode->setProperty( "dimm-info", memInfoData );

		if (memManufData->getLength())
			memoryNode->setProperty( "dimm-manufacturer", memManufData );

		if (memSerialData->getLength())
			memoryNode->setProperty( "dimm-serial-number", memSerialData );

		if (memPartData->getLength())
			memoryNode->setProperty( "dimm-part-number", memPartData );

        memoryNode->setProperty( "ecc-enabled",
            memECCEnabled ? kOSBooleanTrue : kOSBooleanFalse );

        memoryNode->attachToParent( fRoot, gIODTPlane );
        memoryNode->release();
    }

    // Update max_mem kernel variable with the total size of installed RAM

    if (memSizeTotal && (memSizeTotal > max_mem))
    {
        UInt32 bootMaxMem = 0;
        
        if (PE_parse_boot_argn("maxmem", &bootMaxMem, sizeof(bootMaxMem)) && bootMaxMem)
        {
            UInt64 limit = ((UInt64) bootMaxMem) * 1024ULL * 1024ULL;

            if (memSizeTotal > limit)
                memSizeTotal = limit;
        }

        max_mem = memSizeTotal;
    }

	if (fROMNode)
	{
		fROMNode->setName("rom");
        fROMNode->attachToParent( fRoot, gIODTPlane );
        fROMNode->release();
		fROMNode = 0;
	}

    // Fetch Processor Type from Type 131 structure (optional).

    SMB_ANCHOR_RESET(&anchor);
    if (findSMBIOSStructure(&anchor, kSMBTypeOemProcessorType,
        sizeof(SMBOemProcessorType)))
    {
        const SMBOemProcessorType * processorType =
            (const SMBOemProcessorType *) anchor.header;
        
        cpuType = processorType->ProcessorType;
        DEBUG_LOG("SMBIOS: processor type = 0x%04x\n", cpuType);
    }

    // Fetch bus transfer rate from Type 132 structure (optional).

    SMB_ANCHOR_RESET(&anchor);
    if (findSMBIOSStructure(&anchor, kSMBTypeOemProcessorBusSpeed,
        sizeof(SMBOemProcessorBusSpeed)))
    {
        const SMBOemProcessorBusSpeed * speed =
            (const SMBOemProcessorBusSpeed *) anchor.header;
        
        busSpeedMTs = itcSpeedMTs = speed->ProcessorBusSpeed;
        DEBUG_LOG("SMBIOS: Bus speed (MT/s) = %u\n", busSpeedMTs);
    }

    // Fetch cpu and bus nominal frequencies.

    SMB_ANCHOR_RESET(&anchor);
    if (findSMBIOSStructure(&anchor, kSMBTypeProcessorInformation,
        kSMBProcessorInformationMinSize))
    {
        const SMBProcessorInformation * cpuInfo =
            (const SMBProcessorInformation *) anchor.header;

        cpuSpeedMHz = cpuInfo->maximumClock;
        DEBUG_LOG("SMBIOS: CPU speed (MHz) = %u\n", cpuSpeedMHz);
        if (busSpeedMTs == 0)
        {
            busSpeedMTs = cpuInfo->externalClock;
            busSpeedMTs *= 4;  // Assume quad-pumped FSB
            DEBUG_LOG("SMBIOS: FSB speed (MT/s) = %u\n", busSpeedMTs);
        }
    }

    if (busSpeedMTs)
    {
        uint64_t rateInTs = ((uint64_t) busSpeedMTs) * 1000000ULL;

        gPEClockFrequencyInfo.bus_frequency_hz     = rateInTs;
        gPEClockFrequencyInfo.bus_frequency_min_hz = rateInTs;
        gPEClockFrequencyInfo.bus_frequency_max_hz = rateInTs;
        gPEClockFrequencyInfo.bus_clock_rate_hz    =
            (rateInTs < (1ULL << 32)) ? (uint32_t) rateInTs : 0xFFFFFFFF;
    }

    if (cpuSpeedMHz)
    {
        uint64_t rateInHz = ((uint64_t) cpuSpeedMHz) * 1000000ULL;

        gPEClockFrequencyInfo.cpu_frequency_max_hz = rateInHz;
        gPEClockFrequencyInfo.cpu_frequency_min_hz = rateInHz;
        gPEClockFrequencyInfo.cpu_frequency_hz     = rateInHz;
        gPEClockFrequencyInfo.cpu_clock_rate_hz    = rateInHz;
    }

    if (cpuType || itcSpeedMTs)
    {
        IORegistryEntry *		cpus;
        IORegistryEntry *		child;
        IORegistryIterator *	iter;

        cpus = IORegistryEntry::fromPath("/cpus", gIODTPlane);
        if (cpus && itcSpeedMTs)
        {
            uint64_t rateInTs = ((uint64_t) itcSpeedMTs) * 1000000ULL;
            cpus->setProperty(
                "interconnect-speed", &rateInTs,
                sizeof(rateInTs));
        }
        if (cpus && cpuType)
        {
            iter = IORegistryIterator::iterateOver( cpus, gIODTPlane );
            if (iter)
            {
                while ((child = iter->getNextObject()))
                {
                    child->setProperty(
                        "cpu-type", (void *) &cpuType,
                        sizeof(cpuType));
                }
                iter->release();
            }
        }
        if (cpus)
            cpus->release();
    }
}
