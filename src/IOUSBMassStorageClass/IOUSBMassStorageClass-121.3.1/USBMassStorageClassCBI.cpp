/*
 * Copyright (c) 1998-2002 Apple Computer, Inc. All rights reserved.
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

//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	Includes
//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

#include <IOKit/usb/IOUSBMassStorageClass.h>


//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	Macros
//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

// Macros for printing debugging information
#if (USB_MASS_STORAGE_DEBUG == 1)
#define STATUS_LOG(x)	IOLog x
#else
#define STATUS_LOG(x)
#endif

// CBI State Machine States
enum
{
	kCBIExecuteCommand	= 1,			// Begin execution of user command
	kCBIExecuteCommandCompletion,		// Complete the user command
	kCBIBulkIOComplete,					// Complete the bulk I/O
	kCBIReadInterruptComplete,
	kCBIGetStatusControlEndpointComplete,
	kCBIClearControlEndpointComplete,
	kCBIGetStatusBulkEndpointComplete,
	kCBIClearBulkEndpointComplete
};

#pragma mark -
#pragma mark Protocol Services Methods


//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯ AbortSCSICommandForCBIProtocol -	The AbortSCSICommand helper method for
//										CBI and CB protocol devices.
//																	[PROTECTED]
//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

IOReturn 
IOUSBMassStorageClass::AbortSCSICommandForCBIProtocol(
							SCSITaskIdentifier abortTask )
{
	return kIOReturnError;
}


//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯ SendSCSICommandForCBIProtocol -	The SendSCSICommand helper method for
//										CBI and CB protocol devices.
//																	[PROTECTED]
//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

IOReturn 
IOUSBMassStorageClass::SendSCSICommandForCBIProtocol( SCSITaskIdentifier request )
{
	IOReturn			status;
	CBIRequestBlock *	theCBIRequestBlock;
	
	if ( GetInterfaceReference() == NULL )
	{
 		// We have an invalid interface, the device has probably been removed.
 		// Nothing else to do except to report an error.
 		return kIOReturnDeviceError;
	}

	theCBIRequestBlock = GetCBIRequestBlock();

	bzero( theCBIRequestBlock, sizeof(CBIRequestBlock) );

	// Get a local copy of the callers cdb
	GetCommandDescriptorBlock( request, &theCBIRequestBlock->cbiCDB );
	
	// Save the SCSI Task
	theCBIRequestBlock->request = request; 	
	
	// Set up the IOUSBCompletion structure
	theCBIRequestBlock->cbiCompletion.target 		= this;
	theCBIRequestBlock->cbiCompletion.action 		= &this->CBIProtocolUSBCompletionAction;
	theCBIRequestBlock->cbiCompletion.parameter 	= theCBIRequestBlock;

	theCBIRequestBlock->currentState 				= kCBIExecuteCommand;

	// Build the USB command	
    theCBIRequestBlock->cbiDevRequest.bmRequestType 	= USBmakebmRequestType( kUSBOut, kUSBClass, kUSBInterface );	
   	theCBIRequestBlock->cbiDevRequest.bRequest 			= 0;
   	theCBIRequestBlock->cbiDevRequest.wValue			= 0;
	theCBIRequestBlock->cbiDevRequest.wIndex			= 0;
	theCBIRequestBlock->cbiDevRequest.wLength			= 12; //kCommandMaxCDBSize
   	theCBIRequestBlock->cbiDevRequest.pData				= &theCBIRequestBlock->cbiCDB;

	// Send the command over the control endpoint
	status = GetInterfaceReference()->GetDevice()->DeviceRequest( 	&theCBIRequestBlock->cbiDevRequest, 
														GetTimeoutDuration( theCBIRequestBlock->request ),  // Use the client's timeout for both
														GetTimeoutDuration( theCBIRequestBlock->request ),
														&theCBIRequestBlock->cbiCompletion );
   	STATUS_LOG(("%s: SendSCSICommandForCBIProtocol DeviceRequest returned %d\n", 
   				getName(), 
   				status));
   	if ( status != kIOReturnSuccess )
   	{
   		ReleaseCBIRequestBlock( theCBIRequestBlock );
   	}
   	
	return status;
}

#pragma mark -
#pragma mark SendSCSICommand Helper methods


//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯ CBIProtocolUSBCompletionAction								[PROTECTED]
//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void 
IOUSBMassStorageClass::CBIProtocolUSBCompletionAction(
					                void *			target,
					                void *			parameter,
					                IOReturn		status,
					                UInt32			bufferSizeRemaining)
{
	IOUSBMassStorageClass *		theMSC;
	CBIRequestBlock	*			cbiRequestBlock;
	
	theMSC 				= (IOUSBMassStorageClass *) target;
	cbiRequestBlock 	= (CBIRequestBlock *) parameter;
	theMSC->CBIProtocolCommandCompletion( 	cbiRequestBlock, 
											status, 
											bufferSizeRemaining );
}


//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯ CBIProtocolTransferData										[PROTECTED]
//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

IOReturn 
IOUSBMassStorageClass::CBIProtocolTransferData( 
						CBIRequestBlock *		cbiRequestBlock,
						UInt32					nextExecutionState )
{
	IOReturn	status = kIOReturnError;
	
	// Set the next state to be executed
	cbiRequestBlock->currentState = nextExecutionState;

	// Start a bulk in or out transaction
	if (GetDataTransferDirection(cbiRequestBlock->request) == kSCSIDataTransfer_FromTargetToInitiator)
	{
		status = GetBulkInPipe()->Read( 
					GetDataBuffer( cbiRequestBlock->request ), 
					GetTimeoutDuration( cbiRequestBlock->request ),  // Use the client's timeout for both
					GetTimeoutDuration( cbiRequestBlock->request ),
#if 0
					GetRequestedDataTransferCount( cbiRequestBlock->request ),
#endif
					&cbiRequestBlock->cbiCompletion );
	}
	else if (GetDataTransferDirection(cbiRequestBlock->request) == kSCSIDataTransfer_FromInitiatorToTarget)
	{
		status = GetBulkOutPipe()->Write(
					GetDataBuffer( cbiRequestBlock->request ), 
					GetTimeoutDuration( cbiRequestBlock->request ),  // Use the client's timeout for both
					GetTimeoutDuration( cbiRequestBlock->request ),
#if 0
					GetRequestedDataTransferCount( cbiRequestBlock->request ),
#endif
					&cbiRequestBlock->cbiCompletion );
	}

   	STATUS_LOG(("%s: CBIProtocolTransferData returned %d\n", 
   				getName(), 
   				status));
	return status;
}


//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯ CBIProtocolReadInterrupt										[PROTECTED]
//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

IOReturn 
IOUSBMassStorageClass::CBIProtocolReadInterrupt( 
						CBIRequestBlock *		cbiRequestBlock,
						UInt32					nextExecutionState )
{
	IOReturn 			status;

	// Allocate the memory descriptor needed to send the CBW out
	cbiRequestBlock->cbiPhaseDesc = IOMemoryDescriptor::withAddress(
										&cbiRequestBlock->cbiGetStatusBuffer, 
										kUSBStorageAutoStatusSize, 
										kIODirectionIn);
	if ( cbiRequestBlock->cbiPhaseDesc == NULL )
	{
		// The memory descriptor could not be allocated and so the command
		// can not be sent to the device, return an error.
		return kIOReturnNoResources;
	}
	
	// Set the next state to be executed
	cbiRequestBlock->currentState = nextExecutionState;

	// Start a read from the interrupt pipe
	status = GetInterruptPipe()->Read( cbiRequestBlock->cbiPhaseDesc, &cbiRequestBlock->cbiCompletion);
   	STATUS_LOG(("%s: CBIProtocolReadInterrupt returned %d\n", 
   				getName(), 
   				status));
	return status;
}


//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯ CBIGetStatusEndpointStatus									[PROTECTED]
//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

IOReturn 
IOUSBMassStorageClass::CBIGetStatusEndpointStatus( 
						IOUSBPipe *				targetPipe,
						CBIRequestBlock *		cbiRequestBlock,
						UInt32					nextExecutionState )
{
	IOReturn 			status;

	// Set the next state to be executed
	cbiRequestBlock->currentState = nextExecutionState;
	
	// Call the default GetStatusEndpointStatus method
	status = GetStatusEndpointStatus( targetPipe, &cbiRequestBlock->cbiGetStatusBuffer, &cbiRequestBlock->cbiCompletion );
   	STATUS_LOG(("%s: CBIGetStatusEndpointStatus returned %d\n", 
   				getName(), 
   				status));
	return status;
}


//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯ CBIClearFeatureEndpointStall									[PROTECTED]
//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

IOReturn 
IOUSBMassStorageClass::CBIClearFeatureEndpointStall( 
						IOUSBPipe *				targetPipe,
						CBIRequestBlock *		cbiRequestBlock,
						UInt32					nextExecutionState )
{
	IOReturn 			status;

	// Set the next state to be executed
	cbiRequestBlock->currentState = nextExecutionState;
	
	// Call the default ClearFeatureEndpointStall method
	status = ClearFeatureEndpointStall( targetPipe, &cbiRequestBlock->cbiCompletion );
   	STATUS_LOG(("%s: CBIClearFeatureEndpointStall returned %d\n", 
   				getName(), 
   				status));
	return status;
}


//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯ CBIProtocolCommandCompletion									[PROTECTED]
//ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void 
IOUSBMassStorageClass::CBIProtocolCommandCompletion(
						CBIRequestBlock *		cbiRequestBlock,
		                IOReturn				resultingStatus,
		                UInt32					bufferSizeRemaining )
{
	IOReturn 		status = kIOReturnError;
	bool			commandInProgress = false;
	
	if ( GetInterfaceReference() == NULL )
	{
		// Our interface has been closed, probably because of an
		// unplug, return an error for the command since there it
		// can no longer be executed.
		SCSITaskIdentifier	request = cbiRequestBlock->request;
		
		ReleaseCBIRequestBlock( cbiRequestBlock );
		CompleteSCSICommand( request, status );
		return;
	}		
	
	switch( cbiRequestBlock->currentState )
	{
		case kCBIExecuteCommand:		// Device request completion
		{
   			STATUS_LOG(("%s: kCBIExecuteCommand status %d\n", getName(), resultingStatus));
			
			// First check to see if an error occurred on the command out
			if (resultingStatus != kIOReturnSuccess)
			{
				status = CBIGetStatusEndpointStatus( GetControlPipe(), cbiRequestBlock, kCBIGetStatusControlEndpointComplete);
				if ( status == kIOReturnSuccess )
				{
					commandInProgress = true;
				}
   				
   				STATUS_LOG(("%s: kCBIExecuteCommand GetStatusEndpointStatus status %d\n", getName(), status));
			}
			else
			{
				// If there is to be no data transfer then we are done and can return to the caller
				// We will only get to here if no Error occurred.
				if ( GetDataTransferDirection( cbiRequestBlock->request ) == kSCSIDataTransfer_NoDataTransfer )
				{
					status = kIOReturnSuccess;
   					STATUS_LOG(("%s: kCBIExecuteCommand no data to transfer status %d\n", getName(), status));
					break;
				}
				
				status = CBIProtocolTransferData( cbiRequestBlock, kCBIBulkIOComplete );
				if ( status == kIOReturnSuccess )
				{
					commandInProgress = true;
				}

   				STATUS_LOG(("%s: kCBIExecuteCommand CBIProtocolTransferData status %d\n", getName(), status));
			}
		}
		break;
		
		case kCBIBulkIOComplete:
		{
   			STATUS_LOG(("%s: kCBIBulkIOComplete status %x\n", getName(), resultingStatus));
			if ( resultingStatus == kIOReturnOverrun )
			{
				// If we got more data than expected, act like we got exactly the amount
				// requested.
				resultingStatus = kIOReturnSuccess;
				SetRealizedDataTransferCount( cbiRequestBlock->request, GetRequestedDataTransferCount( cbiRequestBlock->request ) );

				// Clear the hale status on the host side for the pipe in use.
				if ( GetDataTransferDirection( cbiRequestBlock->request ) == kSCSIDataTransfer_FromTargetToInitiator )
				{
					GetBulkInPipe()->Reset();
				}
				else if ( GetDataTransferDirection( cbiRequestBlock->request ) == kSCSIDataTransfer_FromInitiatorToTarget )
				{
					GetBulkOutPipe()->Reset();
				}
			}
			else
			{
				SetRealizedDataTransferCount( cbiRequestBlock->request, GetRequestedDataTransferCount( cbiRequestBlock->request ) - bufferSizeRemaining );
			}
			
			if (resultingStatus != kIOReturnSuccess)
			{
				// Check if the bulk endpoint was stalled
				IOUSBPipe * thePipe = NULL;

				if ( GetDataTransferDirection( cbiRequestBlock->request ) == kSCSIDataTransfer_FromTargetToInitiator )
				{
					thePipe = GetBulkInPipe();
				}
				else if ( GetDataTransferDirection( cbiRequestBlock->request ) == kSCSIDataTransfer_FromInitiatorToTarget )
				{
					thePipe = GetBulkOutPipe();
				}
				else
				{
					thePipe = GetControlPipe();
				}

				status = CBIGetStatusEndpointStatus( thePipe, cbiRequestBlock, kCBIGetStatusBulkEndpointComplete);
				if ( status == kIOReturnSuccess )
				{
					commandInProgress = true;
				}
   					
   				STATUS_LOG(("%s: kCBIBulkIOComplete GetStatusEndpointStatus status %d\n", getName(), status));
			}
			else
			{
				if (( GetInterruptPipe() != NULL ) && (GetDataTransferDirection(cbiRequestBlock->request) == kSCSIDataTransfer_FromInitiatorToTarget)
						&& ((GetInterfaceSubclass() == kUSBStorageSFF8070iSubclass ) || ( GetInterfaceSubclass() == kUSBStorageUFISubclass )))
				{
					// We have an interrupt pipe, and device uses it to determine a command is done
					status = CBIProtocolReadInterrupt( cbiRequestBlock, kCBIReadInterruptComplete );
					if ( status == kIOReturnSuccess )
					{
						commandInProgress = true;
					}

   					STATUS_LOG(("%s: kCBIBulkIOComplete CBIProtocolReadInterrupt status %d\n", getName(), status));
				}
				else
				{
					status = kIOReturnSuccess;
				}
			}
		}
		break;

		case kCBIReadInterruptComplete:
		{
   			STATUS_LOG(("%s: kCBIReadInterruptComplete status %d\n", getName(), resultingStatus));

			// Release the memory descriptor for the interrupt pipe.
			cbiRequestBlock->cbiPhaseDesc->release();
			
			// What should the status really be, should probably process and return
			// a relevent error.
			if ((resultingStatus == kIOReturnSuccess) && ((GetInterfaceSubclass() == kUSBStorageSFF8070iSubclass ) || ( GetInterfaceSubclass() == kUSBStorageUFISubclass )))
			{
				// Decide what error to return based on the Interrupt data
				if (( cbiRequestBlock->cbiGetStatusBuffer[0] == 0x00 ) && ( cbiRequestBlock->cbiGetStatusBuffer[1] == 0x00 ))
				{
					status = kIOReturnSuccess;
				}
				else
				{
					status = kIOReturnError;
				}
			}
			else
			{
				// The Class doesn't know how to interpret the data
				// return an error and mark interrupt data as invalid
				status = kIOReturnError;
			}
   			
   			STATUS_LOG(("%s: kCBIReadInterruptComplete ending status %d\n", getName(), status));
		}
		break;

		case kCBIGetStatusControlEndpointComplete:
		{
   			STATUS_LOG(("%s: kCBIGetStatusControlEndpointComplete status %d\n", getName(), resultingStatus));

			if ( resultingStatus == kIOReturnSuccess )
			{
				if ((cbiRequestBlock->cbiGetStatusBuffer[0] & 1) == 1 ) 
				{
					// This endpoint was stalled, go ahead and clear it
					status = CBIClearFeatureEndpointStall( GetControlPipe(), cbiRequestBlock, kCBIClearControlEndpointComplete );
					if ( status == kIOReturnSuccess )
					{
						commandInProgress = true;
					}

   					STATUS_LOG(("%s: kCBIGetStatusControlEndpointComplete CBIClearFeatureEndpointStall status %d\n", getName(), status));
				}
				else
				{
					if ( GetDataTransferDirection( cbiRequestBlock->request ) == kSCSIDataTransfer_NoDataTransfer )
					{
						SetRealizedDataTransferCount( cbiRequestBlock->request, 0 );
						status = kIOReturnError;
					}
					else
					{
						// Check if the bulk endpoint was stalled
						IOUSBPipe * thePipe = NULL;

						if ( GetDataTransferDirection( cbiRequestBlock->request ) == kSCSIDataTransfer_FromTargetToInitiator )
						{
							thePipe = GetBulkInPipe();
						}
						else if ( GetDataTransferDirection( cbiRequestBlock->request ) == kSCSIDataTransfer_FromInitiatorToTarget )
						{
							thePipe = GetBulkOutPipe();
						}
						else
						{
							thePipe = GetControlPipe();
						}
				
						status = CBIGetStatusEndpointStatus( GetControlPipe(), cbiRequestBlock, kCBIGetStatusBulkEndpointComplete);
						if ( status == kIOReturnSuccess )
						{
							commandInProgress = true;
						}

   						STATUS_LOG(("%s: kCBIGetStatusControlEndpointComplete CBIGetStatusEndpointStatus status %d\n", getName(), status));
					}
				}
			}
			else
			{
				// An error occurred to GET_STATUS ( shouldn't happen!!) reset the endpoint anyway
				status = CBIClearFeatureEndpointStall( GetControlPipe(), cbiRequestBlock, kCBIClearControlEndpointComplete );
				if ( status == kIOReturnSuccess )
				{
					commandInProgress = true;
				}
   						
   				STATUS_LOG(("%s: kCBIGetStatusControlEndpointComplete CBIClearFeatureEndpointStall status %d\n", getName(), status));
			}
		}
		break;
		
		case kCBIClearControlEndpointComplete:
		{
   			STATUS_LOG(("%s: kCBIClearControlEndpointComplete status %d\n", getName(), resultingStatus));

			if (resultingStatus == kIOReturnSuccess)
			{
				if ( GetDataTransferDirection( cbiRequestBlock->request ) == kSCSIDataTransfer_NoDataTransfer )
				{
					SetRealizedDataTransferCount( cbiRequestBlock->request, 0 );
					status = kIOReturnError;
				}
				else
				{
					// Check if the bulk endpoint was stalled
					IOUSBPipe * thePipe = NULL;

					if ( GetDataTransferDirection( cbiRequestBlock->request ) == kSCSIDataTransfer_FromTargetToInitiator )
					{
						thePipe = GetBulkInPipe();
					}
					else if ( GetDataTransferDirection( cbiRequestBlock->request ) == kSCSIDataTransfer_FromInitiatorToTarget )
					{
						thePipe = GetBulkOutPipe();
					}
					else
					{
						thePipe = GetControlPipe();
					}
			
					status = CBIGetStatusEndpointStatus( thePipe, cbiRequestBlock, kCBIGetStatusBulkEndpointComplete);
					if ( status == kIOReturnSuccess )
					{
						commandInProgress = true;
					}

   					STATUS_LOG(("%s: kCBIClearControlEndpointComplete CBIGetStatusEndpointStatus status %d\n", getName(), status));
				}
			}
			else
			{
				status = resultingStatus;
			}
		}
		break;
		
		case kCBIGetStatusBulkEndpointComplete:
		{
   			STATUS_LOG(("%s: kCBIGetStatusBulkEndpointComplete status %d\n", getName(), resultingStatus));

			if (resultingStatus == kIOReturnSuccess)
			{
				if ( (cbiRequestBlock->cbiGetStatusBuffer[0] & 1) == 1 ) 
				{
					IOUSBPipe * thePipe = NULL;
					
					if ( GetDataTransferDirection( cbiRequestBlock->request ) == kSCSIDataTransfer_FromTargetToInitiator )
					{
						thePipe = GetBulkInPipe();
					}
					else if ( GetDataTransferDirection( cbiRequestBlock->request ) == kSCSIDataTransfer_FromInitiatorToTarget )
					{
						thePipe = GetBulkOutPipe();
					}
					else
					{
						thePipe = GetControlPipe();
					}
					
					status = CBIClearFeatureEndpointStall( thePipe, cbiRequestBlock, kCBIClearBulkEndpointComplete );
					if ( status == kIOReturnSuccess )
					{
						commandInProgress = true;
					}
   					
   					STATUS_LOG(("%s: kCBIGetStatusBulkEndpointComplete CBIClearFeatureEndpointStall status %d\n", getName(), status));
				}
				else
				{
					SetRealizedDataTransferCount( cbiRequestBlock->request, 0 );
					status = kIOReturnError;
				}
			}
			else
			{
				IOUSBPipe * thePipe = NULL;
				
				if ( GetDataTransferDirection( cbiRequestBlock->request ) == kSCSIDataTransfer_FromTargetToInitiator )
				{
					thePipe = GetBulkInPipe();
				}
				else if ( GetDataTransferDirection( cbiRequestBlock->request ) == kSCSIDataTransfer_FromInitiatorToTarget )
				{
					thePipe = GetBulkOutPipe();
				}
				else
				{
					thePipe = GetControlPipe();
				}
				
				status = CBIClearFeatureEndpointStall( thePipe, cbiRequestBlock, kCBIClearBulkEndpointComplete );
				if ( status == kIOReturnSuccess )
				{
					commandInProgress = true;
				}

   				STATUS_LOG(("%s: kCBIGetStatusBulkEndpointComplete CBIClearFeatureEndpointStall status %d\n", getName(), status));
			}
		}
		break;

		case kCBIClearBulkEndpointComplete:
		{
   			STATUS_LOG(("%s: kCBIClearBulkEndpointComplete status %d\n", getName(), resultingStatus));

			SetRealizedDataTransferCount( cbiRequestBlock->request, 0 );
			status = kIOReturnError;
		}
		break;
		
		default:
		{
   			STATUS_LOG(("%s: default case status %d\n", getName(), resultingStatus));

			SetRealizedDataTransferCount( cbiRequestBlock->request, 0 );
			status = kIOReturnError;
		}
		break;
	}

	// If the command has been completed ( no longer pending ), call the clients completion routine.	
	if ( commandInProgress == false )
	{
		SCSITaskIdentifier	request = cbiRequestBlock->request;
		
		ReleaseCBIRequestBlock( cbiRequestBlock );
		CompleteSCSICommand( request, status );
	}
}

