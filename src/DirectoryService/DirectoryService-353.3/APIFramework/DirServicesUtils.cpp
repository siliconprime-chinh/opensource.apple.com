/*
 * Copyright (c) 2002 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

/*!
 * @header DirServicesUtils
 */

#include "DirServicesConst.h"
#include "DirServicesUtils.h"
#include "DirServicesUtilsPriv.h"
#include "DirServicesPriv.h"
#include "DirServices.h"
#include "DirServicesTypesPriv.h"
#include "PrivateTypes.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define kDSUserAuthAuthorityMarker					"{*AuthenticationAuthority*}"

// -- Static ---------------------------------------------------------------------------------------

static tDataNodePtr	dsGetThisNodePriv 				( tDataNode *inFirsNode, const unsigned long inIndex );
static tDataNodePtr	dsGetLastNodePriv 				( tDataNode *inFirsNode );
static tDataNodePtr	dsAllocListNodeFromStringPriv	( const char *inString );
static tDataNodePtr	dsAllocListNodeFromBuffPriv		( const void *inData, const uInt32 inSize );
static tDirStatus	dsVerifyDataListPriv			( const tDataList *inDataList );
static tDirStatus	dsAppendAuthBufferWithAuthority( const char *inUserName, tDataBufferPtr inAuthAuthorityBuffer,
														tDataBufferPtr inOutAuthBuffer );

//--------------------------------------------------------------------------------------------------
//	Name:	dsDataBufferAllocate
//
//--------------------------------------------------------------------------------------------------

tDataBufferPtr dsDataBufferAllocate ( tDirReference inDirRef, unsigned long inBufferSize )
{
#pragma unused ( inDirRef )
	uInt32				size		= 0;
	tDataBufferPtr		outBuff		= nil;

	size = sizeof( tDataBufferPriv ) + inBufferSize;
	outBuff = (tDataBuffer *)::calloc( size + 1, sizeof( char ) );		// +1 for null term
	if ( outBuff != nil )
	{
		outBuff->fBufferSize	= inBufferSize;
		outBuff->fBufferLength	= 0;
	}

	return( outBuff );

} // dsDataBufferAllocate



//--------------------------------------------------------------------------------------------------
//	Name:	dsDataBufferDeAllocate
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsDataBufferDeAllocate ( tDirReference inDirRef, tDataBuffer *inDataBufferPtr )
{
#pragma unused ( inDirRef )
	tDirStatus		tResult	= eDSNoErr;

	if ( inDataBufferPtr != nil )
	{
		free( inDataBufferPtr );
		inDataBufferPtr = nil;
	}
	else
	{
		tResult = eDSNullDataBuff;
	}

	return( tResult );

} // dsDataBufferDeAllocate



//--------------------------------------------------------------------------------------------------
// Data Node Routines


//--------------------------------------------------------------------------------------------------
//	Name:	dsDataNodeAllocateBlock
//
//--------------------------------------------------------------------------------------------------

tDataNodePtr dsDataNodeAllocateBlock (	tDirReference	inDirRef,
										unsigned long	inDataNodeSize,
										unsigned long	inDataNodeLength,
										tBuffer			inDataNodeBuffer )
{
	tDataNode		   *pOutBuff	= nil;

	if ( inDataNodeBuffer != nil )
	{
		if ( (inDataNodeSize >= inDataNodeLength) && (inDataNodeSize != 0) && (inDataNodeLength != 0) )
		{
			pOutBuff = ::dsDataBufferAllocate( inDirRef, inDataNodeSize );
			if ( pOutBuff != nil )
			{
				::memcpy( pOutBuff->fBufferData, inDataNodeBuffer, inDataNodeLength );
				::dsDataNodeSetLength( pOutBuff, inDataNodeLength );
			}
			else
			{
				LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
			}
		}
	}
	else
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
	}

	return( pOutBuff );

} // dsDataNodeAllocateBlock


//--------------------------------------------------------------------------------------------------
//	Name:	dsDataNodeAllocateString
//
//--------------------------------------------------------------------------------------------------

tDataNodePtr dsDataNodeAllocateString ( tDirReference inDirRef, const char *inCString )
{
	tDataNode		   *pOutBuff	= nil;
	uInt32				len			= 0;

	if ( inCString != nil )
	{
		len = ::strlen( inCString );
	}

	pOutBuff = ::dsDataBufferAllocate( inDirRef, len );
	if ( (pOutBuff != nil) && (inCString != nil) )
	{
		::strcpy( pOutBuff->fBufferData, inCString );
		::dsDataNodeSetLength( pOutBuff, len );
	}

	return( pOutBuff );

} // dsDataNodeAllocateString


//--------------------------------------------------------------------------------------------------
//	Name:	dsDataNodeDeAllocate
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsDataNodeDeAllocate ( tDirReference inDirRef, tDataNode *inDataNodePtr )
{
#pragma unused ( inDirRef )
	tDirStatus		tResult	= eDSNoErr;

	if ( inDataNodePtr != nil )
	{
		free( inDataNodePtr );
		inDataNodePtr = nil;
	}
	else
	{
		tResult = eDSNullParameter;
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
	}

	return( tResult );

} // dsDataNodeDeAllocate


//-----------------------------------------------

//--------------------------------------------------------------------------------------------------
//	Name:	dsDataNodeSetLength
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsDataNodeSetLength ( tDataNode *inDataNodePtr, unsigned long inDataNodeLength )
{
	tDirStatus		tResult	= eDSNoErr;

	if ( inDataNodePtr != nil )
	{
		inDataNodePtr->fBufferLength = inDataNodeLength;
	}
	else
	{
		tResult = eDSNullParameter;
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
	}

	return( tResult );

} // dsDataNodeSetLength



//--------------------------------------------------------------------------------------------------
//	Name:	dsDataNodeGetLength
//
//--------------------------------------------------------------------------------------------------

unsigned long dsDataNodeGetLength ( tDataNode *inDataNodePtr )
{
	unsigned long		uiResult	= 0;

	if ( inDataNodePtr != nil )
	{
		uiResult = inDataNodePtr->fBufferLength;
	}

	return( uiResult );

} // dsDataNodeGetLength


//-----------------------------------------------

//--------------------------------------------------------------------------------------------------
//	Name:	dsDataNodeGetSize
//
//--------------------------------------------------------------------------------------------------

unsigned long dsDataNodeGetSize ( tDataNode *inDataNodePtr )
{
	unsigned long		uiResult	= 0;

	if ( inDataNodePtr != nil )
	{
		uiResult = inDataNodePtr->fBufferSize;
	}

	return( uiResult );

} // dsDataNodeGetSize



//-----------------------------------------------
//-----------------------------------------------
// Data list Routines

//--------------------------------------------------------------------------------------------------
//	Name:	dsDataListAllocate
//
//--------------------------------------------------------------------------------------------------

tDataListPtr dsDataListAllocate ( tDirReference inDirRef )
{
#pragma unused ( inDirRef )
	tDataList		   *outResult	= nil;

	outResult = (tDataList *)::calloc( 1, sizeof( tDataList ) );
	if ( outResult == nil )
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
	}

	return( outResult );

} // dsDataListAllocate



//--------------------------------------------------------------------------------------------------
//	Name:	dsDataListDeAllocate
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsDataListDeAllocate (	tDirReference	inDirRef,
									tDataList	   *inDataList,
									dsBool			inDeAllocateNodesFlag )
{
#pragma unused ( inDirRef )

	return( dsDataListDeallocate( inDirRef, inDataList ) );

} // dsDataListDeAllocate



//--------------------------------------------------------------------------------------------------
//	Name:	dsDataListDeallocate
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsDataListDeallocate (	tDirReference	inDirRef,
									tDataList	   *inDataList )
{
#pragma unused ( inDirRef )
	tDirStatus			tResult		= eDSNoErr;
	tDataBufferPriv	   *pPrivData	= nil;
	tDataBuffer		   *pTmpBuff	= nil;
	tDataBuffer		   *pDataBuff	= nil;

	if ( inDataList == nil )
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( eDSNullDataList );
	}

	//KW need to determine HOW to free the actual tDataList ie. what if stack variable passed in
	//plan to include setting inside tDataList that determines if utility routine actually calloc'ed it

	if ( inDataList->fDataListHead != nil )
	{
		pDataBuff = inDataList->fDataListHead;

            inDataList->fDataListHead = nil;
            inDataList->fDataNodeCount = 0;
            
		pPrivData = (tDataBufferPriv *)pDataBuff;
		while ( pDataBuff != nil )
		{
			pTmpBuff = pDataBuff;

			if ( pPrivData != nil )
			{
				pDataBuff = pPrivData->fNextPtr;
				if ( pDataBuff != nil )
				{
					pPrivData = (tDataBufferPriv *)pDataBuff;
				}
			}
			else
			{
				pDataBuff = nil;
			}

			free( pTmpBuff );
			pTmpBuff = nil;
		}
	}

	return( tResult ); //by above code this never fails

} // dsDataListDeallocate



//-----------------------------------------------

//--------------------------------------------------------------------------------------------------
//	Name:	dsBuildListFromStrings
//
//--------------------------------------------------------------------------------------------------

tDataListPtr dsBuildListFromStrings ( tDirReference inDirRef, const char *in1stCString, ... )
{
	tDirStatus			tResult		= eDSNoErr;
	tDataList		   *pOutList	= nil;
	va_list				args;

	pOutList = ::dsDataListAllocate( inDirRef );
	if ( pOutList == nil )
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( nil );
	}

	va_start( args, in1stCString );

	tResult = ::dsBuildListFromStringsAllocV( inDirRef, pOutList, in1stCString, args );
	return( pOutList );

} // dsBuildListFromStrings



//--------------------------------------------------------------------------------------------------
//	Name:	dsGetPathFromList
//
//--------------------------------------------------------------------------------------------------

char* dsGetPathFromList ( tDirReference	inDirRef, const tDataList *inDataList, const char *inDelimiter )
{
#pragma unused ( inDirRef )
	char			   *outStr			= nil;
	uInt32				uiSafetyCntr	= 0;
	uInt32				uiStrLen		= 0;
	tDataNode		   *pCurrNode		= nil;
	tDataBufferPriv	   *pPrivData		= nil;
	char			   *prevStr			= nil;
	uInt32				cStrLen			= 256;
	char			   *nextStr			= nil;

	if ( (inDataList == nil) || (inDelimiter == nil) )
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( nil );
	}

	if ( (inDataList->fDataNodeCount == 0) || (inDataList->fDataListHead == nil) )
	{
		LOG2( kStdErr, "*** DS Parameter Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( nil );
	}

	prevStr = (char *)calloc(cStrLen,sizeof(char));
	pCurrNode = inDataList->fDataListHead;
	while ( pCurrNode != nil )
	{
		// Append the delimiter
		strncat(prevStr,inDelimiter,strlen(inDelimiter));

		//check if there is more char buffer length required
		// Append the string
		pPrivData = (tDataBufferPriv *)pCurrNode;
		//check if there is more char buffer length required ie. look at the next string plus
		//the delimiter plus termination null plus pad of 4
		while (cStrLen < (1+strlen(prevStr)+pPrivData->fBufferLength+4))
		{
			nextStr = (char *)calloc(strlen(prevStr)+1,sizeof(char));
			strcpy(nextStr,prevStr);
			free(prevStr);
			cStrLen *= 2;
			prevStr = (char *)calloc(cStrLen,sizeof(char));
			strcpy(prevStr,nextStr);
			free(nextStr);
		}
		strncat(prevStr,(const char *)pPrivData->fBufferData,pPrivData->fBufferLength);

		pCurrNode = pPrivData->fNextPtr;

		uiSafetyCntr++;

		if ( uiSafetyCntr == inDataList->fDataNodeCount )
		{
			// Yes, we are done
			pCurrNode = nil;

			if ( pPrivData->fNextPtr != nil )
			{
				LOG2( kStdErr, "*** DS Parameter Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
			}
		}
	}

	if ( uiSafetyCntr != inDataList->fDataNodeCount )
	{
		LOG2( kStdErr, "*** DS Parameter Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
	}

	uiStrLen = strlen(prevStr);
	if ( uiStrLen != 0 )
	{
		outStr = (char *)calloc( uiStrLen + 1, sizeof(char));
		::strcpy( outStr, prevStr );
	}

	free(prevStr);

	return( outStr );

} // dsGetPathFromList



//--------------------------------------------------------------------------------------------------
//	Name:	dsBuildStringsFromList
//
//--------------------------------------------------------------------------------------------------

char** dsAllocStringsFromList( tDirReference inDirRef, const tDataList *inDataList )
{
#pragma unused ( inDirRef )
	tDataNode		   *pCurrNode		= nil;
	tDataBufferPriv	   *pPrivData		= nil;
	char			  **listOfStrings   = nil;

	if (inDataList == nil)
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( nil );
	}

	if ( (inDataList->fDataNodeCount == 0) || (inDataList->fDataListHead == nil) )
	{
		LOG2( kStdErr, "*** DS Parameter Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( nil );
	}

	listOfStrings = (char **)calloc(inDataList->fDataNodeCount + 1, sizeof(char *));
	pCurrNode = inDataList->fDataListHead;
	uInt32 strCount = 0;
	while ( pCurrNode != nil )
	{
		pPrivData = (tDataBufferPriv *)pCurrNode;

		listOfStrings[strCount] = (char *)calloc(1, strlen((const char *)pPrivData->fBufferData) + 1);
		strcpy(listOfStrings[strCount], (const char *)pPrivData->fBufferData);

		pCurrNode = pPrivData->fNextPtr;

		strCount++;

		if ( strCount == inDataList->fDataNodeCount )
		{
			// Yes, we are done
			pCurrNode = nil;

			if ( pPrivData->fNextPtr != nil )
			{
				LOG2( kStdErr, "*** DS Parameter Corrupt Data Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
			}
		}
	}

	if ( strCount != inDataList->fDataNodeCount )
	{
		LOG2( kStdErr, "*** DS Parameter Corrupt Index Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
	}

	return( listOfStrings );

} // dsBuildStringsFromList



//--------------------------------------------------------------------------------------------------
//	Name:	dsBuildFromPath
//
//--------------------------------------------------------------------------------------------------

tDataListPtr dsBuildFromPath ( tDirReference	inDirRef,
								const char	   *inPathCString,
								const char	   *inPathSeparatorCString )
{
	const char		   *inStr		= nil;
	char			   *ptr			= nil;
	const char		   *inDelim		= nil;
	sInt32				delimLen	= 0;
	dsBool				done		= false;
	sInt32				len			= 0;
	tDataList		   *outDataList	= nil;
	char			   *cStr		= nil;

	if ( (inPathCString == nil) || (inPathSeparatorCString == nil) )
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( nil );
	}

	inStr = inPathCString;
	len = ::strlen( inStr );

   	inDelim = inPathSeparatorCString;
   	delimLen = ::strlen( inDelim );

	// Does the string == the delimiter
	if ( ::strcmp( inStr, inDelim ) == 0 )
	{
		return( nil );
	}

	outDataList = ::dsDataListAllocate( inDirRef );
	if ( outDataList == nil )
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( nil );
	}

	ptr = strstr( inStr, inDelim );

	// Does the first char(s) == the delimiter
	if ( (ptr != nil) && (ptr == inStr) )
	{
		inStr += delimLen;
	}

	while ( !done && (*inStr != '\0') )
	{
		ptr = ::strstr( inStr, inDelim );
		if ( ptr == nil )
		{
			len = ::strlen( inStr );

			cStr = (char *)calloc(len + 1, sizeof(char));
			strncpy(cStr,inStr,len);

			::dsAppendStringToListAlloc( 0x00F0F0F0, outDataList, cStr );
			free(cStr);
			
			done = true;
		}
		else
		{
			len = ptr - inStr;

			cStr = (char *)calloc(len + 1, sizeof(char));
			strncpy(cStr,inStr,len);

			::dsAppendStringToListAlloc( 0x00F0F0F0, outDataList, cStr );
			free(cStr);
			
			inStr += len + delimLen;
		}
	}

	return( outDataList );

} // dsBuildFromPath


//--------------------------------------------------------------------------------------------------
//	Name:	dsBuildListFromPathAlloc
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsBuildListFromPathAlloc ( tDirReference	inDirRef,
									  tDataList		*inDataList,
									  const char	*inPathCString,
									  const char	*inPathSeparatorCString )
{
	tDirStatus			tResult		= eDSNoErr;
	const char		   *inStr		= nil;
	char			   *ptr			= nil;
	const char		   *inDelim		= nil;
	sInt32				delimLen	= 0;
	dsBool				done		= false;
	sInt32				len			= 0;
	char			   *cStr		= nil;

	if ( inDataList == nil )
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( eDSNullDataList );
	}

	if ( (inPathCString == nil) || (inPathSeparatorCString == nil) )
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( eDSNullParameter );
	}

	// Does the string == the delimiter
	if ( ::strcmp( inPathCString, inPathSeparatorCString ) == 0 )
	{
		return( eDSEmptyParameter );
	}

	inStr = inPathCString;

   	inDelim = inPathSeparatorCString;
   	delimLen = ::strlen( inDelim );

	// This could leak
	inDataList->fDataNodeCount = 0;
	inDataList->fDataListHead  = nil;

	ptr = ::strstr( inStr, inDelim );

	// Does the first char(s) == the delimiter
	if ( (ptr != nil) && (ptr == inStr) )
	{
		inStr += delimLen;
	}

	while ( !done && (tResult == eDSNoErr) && (*inStr != '\0') )
	{
		ptr = ::strstr( inStr, inDelim );
		if ( ptr == nil )
		{
			len = ::strlen( inStr );
			done = true;
		}
		else
		{
			len = ptr - inStr;
		}

		cStr = (char *)::calloc(len + 1, sizeof(char));
		::strncpy(cStr,inStr,len);
		tResult = ::dsAppendStringToListAlloc( 0x00F0F0F0, inDataList, cStr );
		::free(cStr);
			
		inStr += len + delimLen;
	}

	if ( tResult != eDSNoErr )
	{
		::dsDataListDeallocate ( inDirRef, inDataList );
	}

	return( tResult );

} // dsBuildListFromPathAlloc


//--------------------------------------------------------------------------------------------------
//	Name:	dsBuildListFromNodes
//
//--------------------------------------------------------------------------------------------------

tDataListPtr dsBuildListFromNodes ( tDirReference inDirRef, tDataNode *in1stDataNodePtr, ... )
{

#pragma unused ( inDirRef, in1stDataNodePtr )

	return( nil );

} // dsBuildListFromNodes


//--------------------------------------------------------------------------------------------------
//	Name:	dsBuildListFromStringsAlloc
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsBuildListFromStringsAlloc ( tDirReference	inDirRef,
										 tDataList	   *inDataList,
										 const char	   *inStr, ... )
{
	va_list		args;
	tDirStatus	tResult	= eDSNoErr;

	va_start( args, inStr );
	tResult = ::dsBuildListFromStringsAllocV(inDirRef, inDataList, inStr, args);
	va_end( args );
	return tResult;
}

//--------------------------------------------------------------------------------------------------
//	Name:	dsBuildListFromStringsAlloc
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsBuildListFromStringsAllocV ( tDirReference	inDirRef,
										  tDataList		*inDataList,
										  const char	*inStr,
										  va_list		args )
{
#pragma unused ( inDirRef )
	tDirStatus			tResult		= eDSNoErr;
	const char		   *pStr		= nil;
	tDataNode		   *pCurrNode	= nil;
	tDataNode		   *pPrevNode	= nil;
	tDataBufferPriv    *pPrivData	= nil;

	if ( inDataList == nil )
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( eDSNullDataList );
	}

	// This could leak
	inDataList->fDataNodeCount = 0;
	inDataList->fDataListHead  = nil;

    pStr = inStr;

	while ( (pStr != nil) && (tResult == eDSNoErr) )
	{
		// Node size is: struct size + string length + null term byte
		pCurrNode = ::dsAllocListNodeFromStringPriv( pStr );
		if ( pCurrNode != nil )
		{
			if ( inDataList->fDataNodeCount == 0 )
			{
				inDataList->fDataListHead = pCurrNode;
				pPrivData = (tDataBufferPriv *)pCurrNode;
				pPrivData->fPrevPtr		= nil;
				pPrivData->fNextPtr		= nil;
				pPrivData->fScriptCode	= kASCIICodeScript;

				inDataList->fDataNodeCount++;
			}
			else if ( pPrevNode != nil )
			{
				// Get the previous node's header and point it to the next
				pPrivData = (tDataBufferPriv *)pPrevNode;
				pPrivData->fNextPtr = pCurrNode;

				// Get the current node's header and point it to the prevous
				pPrivData = (tDataBufferPriv *)pCurrNode;
				pPrivData->fPrevPtr = pPrevNode;

				// Set the script code to ASCII
				pPrivData->fScriptCode = kASCIICodeScript;

				inDataList->fDataNodeCount++;
			}
			else
			{
				tResult = eMemoryAllocError;
				LOG3( kStdErr, "*** DS OSError: File: %s. Line: %d. Error = %d.\n", __FILE__, __LINE__, tResult );
			}
		}
		else
		{
			tResult = eMemoryAllocError;
			LOG3( kStdErr, "*** DS OSError: File: %s. Line: %d. Error = %d.\n", __FILE__, __LINE__, tResult );
		}

		pStr = va_arg( args, char * );

		pPrevNode = pCurrNode;
		pCurrNode = nil;
	}

	va_end( args );

	return( tResult );

} // dsBuildListFromStringsAlloc


//--------------------------------------------------------------------------------------------------
//	Name:	dsAppendStringToList
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsAppendStringToList ( tDataList *inDataList, const char *inCString )
{
#pragma unused ( inDataList, inCString )

	LOG2( kStdErr, "*** DS Call to Unsupported function: File: %s. Line: %d.\n", __FILE__, __LINE__ );

	return( eNoLongerSupported );

} // dsAppendStringToList


//--------------------------------------------------------------------------------------------------
//	Name:	dsAppendStringToListAlloc
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsAppendStringToListAlloc (	tDirReference	inDirRef,
										tDataList	   *inOutDataList,
										const char	   *inCString )
{
#pragma unused ( inDirRef )
	tDirStatus			tResult			= eDSNoErr;
	const char		   *pInString		= inCString;
	tDataNode		   *pNewNode		= nil;
	tDataNode		   *pCurrNode		= nil;
	tDataBufferPriv    *pNewNodePriv	= nil;
	tDataBufferPriv    *pCurNodePriv	= nil;

	if ( inOutDataList == nil )
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( eDSNullDataList );
	}

	if ( inCString == nil )
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( eDSNullParameter );
	}

	if ( (inOutDataList->fDataNodeCount == 0) || (inOutDataList->fDataListHead == nil) )
	{
		// This could leak
		inOutDataList->fDataNodeCount	= 0;
		inOutDataList->fDataListHead	= nil;
	}

	pNewNode = ::dsAllocListNodeFromStringPriv( pInString );
	if ( pNewNode == nil )
	{
		LOG3( kStdErr, "*** DS OSError: File: %s. Line: %d. Error = %d.\n", __FILE__, __LINE__, eMemoryAllocError );
		return( eMemoryAllocError );
	}

	if ( inOutDataList->fDataNodeCount == 0 )
	{
		inOutDataList->fDataListHead = pNewNode;
		pNewNodePriv = (tDataBufferPriv *)pNewNode;
		pNewNodePriv->fPrevPtr		= nil;
		pNewNodePriv->fNextPtr		= nil;
		pNewNodePriv->fScriptCode	= kASCIICodeScript;

		inOutDataList->fDataNodeCount++;
	}
	else
	{
		pCurrNode = ::dsGetLastNodePriv( inOutDataList->fDataListHead );
		if ( pCurrNode != nil )
		{
			// Get the current node's header and point it to the new
			pCurNodePriv = (tDataBufferPriv *)pCurrNode;
			pCurNodePriv->fNextPtr = pNewNode;

			// Get the new node's header and point it to the prevous end
			pNewNodePriv = (tDataBufferPriv *)pNewNode;
			pNewNodePriv->fPrevPtr	= pCurrNode;
			pNewNodePriv->fNextPtr	= nil;

			// Set the script code to ASCII
			pNewNodePriv->fScriptCode = kASCIICodeScript;

			inOutDataList->fDataNodeCount++;
		}
		else
		{
			tResult = eDSInvalidIndex;
			LOG3( kStdErr, "*** DS OSError: File: %s. Line: %d. Error = %d.\n", __FILE__, __LINE__, tResult );
		}
	}

	return( tResult );

} // dsAppendStringToListAlloc



//--------------------------------------------------------------------------------------------------
//	Name:	dsBuildListFromNodesAlloc
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsBuildListFromNodesAlloc (	tDirReference	inDirRef,
										tDataList	   *inDataList,
										tDataNode	   *in1stDataNodePtr,
										... )
{
#pragma unused ( inDirRef )
	tDirStatus			tResult		= eDSNoErr;
	tDataNode		   *pNodePtr	= in1stDataNodePtr;;
	tDataNode		   *pCurrNode	= nil;
	tDataNode		   *pPrevNode	= nil;
	tDataBufferPriv    *pPrivData	= nil;
	va_list				args;

	if ( inDataList == nil )
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( eDSNullDataList );
	}

	// This could leak
	inDataList->fDataNodeCount	= 0;
	inDataList->fDataListHead	= nil;

	va_start( args, in1stDataNodePtr );

	while ( (pNodePtr != nil) && (tResult == eDSNoErr) )
	{
		pCurrNode = ::dsAllocListNodeFromBuffPriv( pNodePtr->fBufferData, pNodePtr->fBufferLength );
		if ( pCurrNode != nil )
		{
			// Is it the first node in the list
			if ( inDataList->fDataNodeCount == 0 )
			{
				inDataList->fDataListHead = pCurrNode;
				pPrivData = (tDataBufferPriv *)pCurrNode;
				pPrivData->fPrevPtr		= nil;
				pPrivData->fNextPtr		= nil;
				pPrivData->fScriptCode	= kASCIICodeScript;

				inDataList->fDataNodeCount++;
			}
			else if ( pPrevNode != nil )
			{
				// Get the previous node's header and point it to the next
				pPrivData = (tDataBufferPriv *)pPrevNode;
				pPrivData->fNextPtr = pCurrNode;

				// Get the current node's header and point it to the prevous
				pPrivData = (tDataBufferPriv *)pCurrNode;
				pPrivData->fPrevPtr = pPrevNode;

				// Set the script code to ASCII
				pPrivData->fScriptCode = kASCIICodeScript;

				inDataList->fDataNodeCount++;
			}
			else
			{
				tResult = eMemoryAllocError;
				LOG3( kStdErr, "*** DS OSError: File: %s. Line: %d. Error = %d.\n", __FILE__, __LINE__, tResult );
			}
		}
		else
		{
			tResult = eMemoryAllocError;
			LOG3( kStdErr, "*** DS OSError: File: %s. Line: %d. Error = %d.\n", __FILE__, __LINE__, tResult );
		}

		pNodePtr = va_arg( args, tDataNode * );

		pPrevNode = pCurrNode;
		pCurrNode = nil;
	}

	return( tResult );

} // dsBuildListFromNodesAlloc



//-----------------------------------------------

//--------------------------------------------------------------------------------------------------
//	Name:	dsDataListGetNodeCount
//
//--------------------------------------------------------------------------------------------------

unsigned long dsDataListGetNodeCount ( const tDataList *inDataList )
{
	tDirStatus		tResult		= eDSNoErr;
	unsigned long	outCount	= 0;

	if ( inDataList != nil )
	{
		// Verify node count first
		tResult = ::dsVerifyDataListPriv( inDataList );
		if ( tResult == eDSNoErr )
		{
			outCount = inDataList->fDataNodeCount;
		}
	}
	else
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
	}

	return( outCount );

} // dsDataListGetNodeCount


//--------------------------------------------------------------------------------------------------
//	Name:	dsGetDataLength
//
//--------------------------------------------------------------------------------------------------

unsigned long dsGetDataLength ( const tDataList *inDataList )
{
	unsigned long		outStrLen	= 0;
	tDataNode		   *pCurrNode	= nil;
	tDataBufferPriv    *pPrivData	= nil;

	if ( inDataList != nil )
	{
		pCurrNode = inDataList->fDataListHead;

		// Get the list total length
		while ( pCurrNode != nil )
		{
			// Get this node's length
			outStrLen += pCurrNode->fBufferLength;

			// Get the private header
			pPrivData = (tDataBufferPriv *)pCurrNode;

			// Assign the current node to the next one
			pCurrNode = pPrivData->fNextPtr;
		}
	}
	else
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
	}

	return( outStrLen );

} // dsGetDataLength


//--------------------------------------------------------------------------------------------------
//	Name:	dsDataListInsertNode
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsDataListInsertNode (	tDataList	*inDataList,
									tDataNode	*inAfterDataNode,
									tDataNode	*inInsertDataNode )
{
#pragma unused ( inDataList, inAfterDataNode, inInsertDataNode )

	LOG2( kStdErr, "*** DS Call to unsupported function: File: %s. Line: %d.\n", __FILE__, __LINE__ );

	return( eNoLongerSupported );

} // dsDataListInsertNode



//--------------------------------------------------------------------------------------------------
//	Name:	dsDataListInsertAfter
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsDataListInsertAfter (	tDirReference		inDirRef,
									tDataList		   *inDataList,
									tDataNode		   *inDataNode,
									const unsigned long	inIndex )
{
#pragma unused ( inDirRef )
	tDirStatus			tResult			= eDSNoErr;
	tDataNode		   *pNewNode		= nil;
	tDataNode		   *pCurrNode		= nil;
	tDataNode		   *pNextNode		= nil;
	tDataBufferPriv    *pNewNodePriv		= nil;
	tDataBufferPriv    *pCurNodePriv		= nil;
	tDataBufferPriv    *pNextNodeHdr	= nil;

	if ( inDataList == nil )
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( eDSNullDataList );
	}

	if ( inDataNode == nil )
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( eDSNullParameter );
	}

	if ( inDataNode->fBufferLength > inDataNode->fBufferSize )
	{
		// Length is bigger than its size

		LOG2( kStdErr, "*** DS Parameter Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );

		return( eDSBadDataNodeLength );
	}

	if ( ((inDataList->fDataNodeCount != 0) && (inDataList->fDataListHead == nil)) ||
		 ((inDataList->fDataNodeCount == 0) && (inDataList->fDataListHead != nil)) )
	{
		// Can't trust this node list

		LOG2( kStdErr, "*** DS Parameter Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );

		return( eDSBadDataNodeFormat );
	}

	if ( inIndex > inDataList->fDataNodeCount )
	{
		LOG3( kStdErr, "*** DS OSError: File: %s. Line: %d. Error = %d.\n", __FILE__, __LINE__, eDSIndexOutOfRange );
		return( eDSIndexOutOfRange );
	}

	pNewNode = ::dsAllocListNodeFromBuffPriv( inDataNode->fBufferData, inDataNode->fBufferLength );
	if ( pNewNode == nil )
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( eMemoryAllocError );
	}

	if ( inIndex == 0 )
	{
		pNextNode = inDataList->fDataListHead;
        inDataList->fDataListHead = pNewNode;

		if ( pNextNode != nil )
		{
			pNextNodeHdr = (tDataBufferPriv *)pNextNode;

			// Set the next node's back pointer
			pNextNodeHdr->fPrevPtr = pNewNode;
		}

		pNewNodePriv = (tDataBufferPriv *)pNewNode;
		pNewNodePriv->fPrevPtr		= nil;
		pNewNodePriv->fNextPtr		= pNextNode;
		pNewNodePriv->fScriptCode	= kASCIICodeScript;

		inDataList->fDataNodeCount++;
	}
	else
	{
		pCurrNode = ::dsGetThisNodePriv( inDataList->fDataListHead, inIndex );
		if ( pCurrNode != nil )
		{
			// Get the current node's header and point it to the new
			pCurNodePriv = (tDataBufferPriv *)pCurrNode;

			// Get the new node's header and point it to the prevous end
			pNewNodePriv = (tDataBufferPriv *)pNewNode;

			pNextNode = pCurNodePriv->fNextPtr;
			if ( pNextNode != nil )
			{
				pNextNodeHdr = (tDataBufferPriv *)pNextNode;

				// Set the next node's back pointer
				pNextNodeHdr->fPrevPtr = pNewNode;
			}

			// Set the current node's front pointer
			pCurNodePriv->fNextPtr = pNewNode;

			// Set the new nodes front and back pointer
			pNewNodePriv->fPrevPtr	= pCurrNode;
			pNewNodePriv->fNextPtr	= pNextNode;

			// Set the script code to ASCII
			pNewNodePriv->fScriptCode = kASCIICodeScript;

			inDataList->fDataNodeCount++;
		}
		else
		{
			free( pNewNode );
			pNewNode = nil;

			tResult = eDSInvalDataList;
		}
	}

	return( tResult );

} // dsDataListInsertAfter


//--------------------------------------------------------------------------------------------------
//	Name:	dsDataListMergeList
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsDataListMergeList ( tDataListPtr inDataList, tDataNode *inAfterDataNode, tDataListPtr inMergeDataList )
{
#pragma unused ( inDataList, inAfterDataNode, inMergeDataList )

	LOG2( kStdErr, "*** DS Call to unsupported function: File: %s. Line: %d.\n", __FILE__, __LINE__ );

	return( eNoLongerSupported );

} // dsDataListMergeList


//--------------------------------------------------------------------------------------------------
//	Name:	dsDataListMergeListAfter
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsDataListMergeListAfter (	tDataList		   *inTargetList,
										tDataList		   *inSourceList,
										const unsigned long	inIndex )
{
	tDirStatus			tResult			= eDSNoErr;
	tDataNode		   *pCurrNode		= nil;
	tDataNode		   *pNextNode		= nil;
	tDataNode		   *pFirstNode		= nil;
	tDataNode		   *pLastNode		= nil;
	tDataBufferPriv    *pCurrPrivData	= nil;
	tDataBufferPriv    *pNextPrivData	= nil;
	tDataBufferPriv    *pFirstPrivData	= nil;
	tDataBufferPriv    *pLastPrivData	= nil;

	// Do a null check
	if ( (inTargetList == nil) || (inSourceList == nil) )
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( eDSNullDataList );
	}

	// Make sure we have a valid data node list
	tResult = ::dsVerifyDataListPriv( inTargetList );
	if ( tResult != eDSNoErr )
	{
		return( tResult );
	}
	tResult = ::dsVerifyDataListPriv( inSourceList );
	if ( tResult != eDSNoErr )
	{
		return( tResult );
	}

	if ( inIndex > inTargetList->fDataNodeCount  )
	{
		LOG2( kStdErr, "*** DS Out-of-range Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( eDSIndexOutOfRange );
	}

	if ( inSourceList->fDataNodeCount == 0 )
	{
		return( tResult );
	}

	// Get the first node from the source list
	pFirstNode = ::dsGetThisNodePriv( inSourceList->fDataListHead, 1 );

	if ( inSourceList->fDataNodeCount == 1 )
	{
		// First and last node are the same
		pLastNode = pFirstNode;
	}
	else
	{
		// Get the last node from the source list
		pLastNode = ::dsGetLastNodePriv( inSourceList->fDataListHead );
	}

// xxxx deal with 0 --- head of list index

	// Get the merg point node
	pCurrNode = ::dsGetThisNodePriv( inTargetList->fDataListHead, inIndex );

	if ( inIndex < inTargetList->fDataNodeCount )
	{
		// Get the node after the merg point node
		pNextNode = ::dsGetThisNodePriv( inTargetList->fDataListHead, inIndex + 1 );
	}

	if ( (pFirstNode == nil) || (pLastNode == nil) || (pCurrNode == nil) )
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( eDSInvalDataList );
	}

	// Link the first two nodes
	pCurrPrivData	= (tDataBufferPriv *)pCurrNode;
	pFirstPrivData	= (tDataBufferPriv *)pFirstNode;

	pCurrPrivData->fNextPtr		= pFirstNode;
	pFirstPrivData->fPrevPtr	= pCurrNode;

	if ( pNextNode != nil )
	{
		// Link the last two nodes
		pLastPrivData = (tDataBufferPriv *)pLastNode;
		pNextPrivData = (tDataBufferPriv *)pNextNode;

		pLastPrivData->fNextPtr	= pNextNode;
		pNextPrivData->fPrevPtr	= pLastNode;
	}
	else
	{
		pLastPrivData = (tDataBufferPriv *)pLastNode;
		pCurrPrivData->fNextPtr = nil;
	}

	return( tResult );

} // dsDataListMergeListAfter


//--------------------------------------------------------------------------------------------------
//	Name:	dsDataListCopyList
//
//--------------------------------------------------------------------------------------------------

tDataListPtr dsDataListCopyList ( tDirReference inDirRef, const tDataList *inSourceList )
{
	tDirStatus			tResult			= eDSNoErr;
	tDataList		   *pOutList		= nil;
	unsigned long		count			= 0;
	unsigned long		uiSize			= 0;
	unsigned long		uiLength		= 0;
	tDataNode		   *pCurrNode		= nil;
	tDataNode		   *pNewNode		= nil;
	tDataNode		   *pPrevNode		= nil;
	tDataBufferPriv	   *pPrevPrivData	= nil;
	tDataBufferPriv	   *pCurrPrivData	= nil;
	tDataBufferPriv	   *pNewPrivData	= nil;

	// Make sure we have a valid data node list
	tResult = ::dsVerifyDataListPriv( inSourceList );
	if ( tResult != eDSNoErr )
	{
		return( nil );
	}

	pOutList = ::dsDataListAllocate( inDirRef );
	if ( pOutList == nil )
	{
		LOG2( kStdErr, "*** DS NULL Error: File: %s. Line: %d.\n", __FILE__, __LINE__ );
		return( nil );
	}

	for ( count = 1; count <= inSourceList->fDataNodeCount; count++ )
	{
		pCurrNode = ::dsGetThisNodePriv( inSourceList->fDataListHead, count );
		if ( pCurrNode != nil )
		{
			// Duplicate the data into a new node
			pCurrPrivData = (tDataBufferPriv *)pCurrNode;
			uiSize = pCurrPrivData->fBufferSize;
			uiLength = pCurrPrivData->fBufferLength;

			pNewNode = ::dsDataBufferAllocate( inDirRef, uiSize );
			if ( pNewNode != nil )
			{
				pNewNode->fBufferSize	= uiSize;
				pNewNode->fBufferLength	= uiLength;

				pNewPrivData = (tDataBufferPriv *)pNewNode;
				::memcpy( pNewPrivData->fBufferData, pCurrPrivData->fBufferData, uiLength );

				// Set the script code
				pNewPrivData->fScriptCode = pCurrPrivData->fScriptCode;

				// Link the new node in the list
				if ( pPrevNode == nil )
				{
					pPrevNode = pNewNode;
					pOutList->fDataListHead = pNewNode;

					pNewPrivData->fPrevPtr	= nil;
					pNewPrivData->fNextPtr	= nil;

					pNewNode = nil;
				}
				else
				{
					pPrevPrivData = (tDataBufferPriv *)pPrevNode;

					pPrevPrivData->fNextPtr	= pNewNode;

					pNewPrivData->fPrevPtr	= pPrevNode;
					pNewPrivData->fNextPtr	= nil;

					pPrevNode = pNewNode;
					pNewNode = nil;
				}
				pOutList->fDataNodeCount++;
			}
		}
	}

	return( pOutList );

} // dsDataListCopyList



//--------------------------------------------------------------------------------------------------
//	Name:	dsDataListRemoveNodes
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsDataListRemoveNodes ( tDataListPtr inDataList, tDataNode *in1stDataNode, unsigned long inDeleteCount )
{
#pragma unused ( inDataList, in1stDataNode, inDeleteCount )

	LOG2( kStdErr, "*** DS Call to unsupported function: File: %s. Line: %d.\n", __FILE__, __LINE__ );

	return( eNoLongerSupported );

} // dsDataListRemoveNodes


//--------------------------------------------------------------------------------------------------
//	Name:	dsDataListRemoveThisNode
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsDataListRemoveThisNode ( tDataListPtr inDataList, unsigned long inNodeIndex, unsigned long inDeleteCount )
{
#pragma unused ( inDataList, inNodeIndex, inDeleteCount )

	LOG2( kStdErr, "*** DS Call to unsupported function: File: %s. Line: %d.\n", __FILE__, __LINE__ );

	return( eNoLongerSupported );

} // dsDataListRemoveThisNode


//--------------------------------------------------------------------------------------------------
//	Name:	dsDataListDeleteThisNode
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsDataListDeleteThisNode (	tDirReference	inDirRef,
										tDataList	   *inDataList,
										unsigned long	inIndex )
{
	tDirStatus			tResult		= eDSNoErr;
	tDataNode		   *pCurrNode	= nil;
	tDataBufferPriv	   *pCurrPriv	= nil;
	tDataBufferPriv	   *pPrevPriv	= nil;
	tDataBufferPriv	   *pNextPriv	= nil;

	// Make sure we have a valid data node list
	tResult = ::dsVerifyDataListPriv( inDataList );
	if ( tResult != eDSNoErr )
	{
		return( tResult );
	}

	if ( (inIndex > inDataList->fDataNodeCount) && (inIndex != 0) )
	{
		return( eDSIndexOutOfRange );
	}

	// Get the node we are looking for
	pCurrNode = ::dsGetThisNodePriv( inDataList->fDataListHead, inIndex );
	if ( pCurrNode != nil )
	{
		pCurrPriv = (tDataBufferPriv *)pCurrNode;
		pPrevPriv = (tDataBufferPriv *)pCurrPriv->fPrevPtr;
		pNextPriv = (tDataBufferPriv *)pCurrPriv->fNextPtr;

		if ( inIndex == 1 )
		{
			// Delete the head of the list
			inDataList->fDataListHead = (tDataNode *)pNextPriv;
			if ( pNextPriv != nil )
			{
				pNextPriv->fPrevPtr = nil;
			}
		}
		else if ( inIndex == inDataList->fDataNodeCount )
		{
			// Delete the last node from the list
			pPrevPriv->fNextPtr = nil;
		}
		else
		{
			// Delete from the middle
			pPrevPriv->fNextPtr = (tDataNode *)pNextPriv;
			pNextPriv->fPrevPtr = (tDataNode *)pPrevPriv;
		}

		::dsDataBufferDeAllocate( inDirRef, pCurrNode );
		pCurrNode = nil;

		inDataList->fDataNodeCount--;
	}

	return( tResult );

} // dsDataListDeleteThisNode


//--------------------------------------------------------------------------------------------------
//	Name:	dsDataListGetNode
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsDataListGetNode ( tDataListPtr		inDataList,
								unsigned long	inIndex,
								tDataNode	  **outDataNode )
{
#pragma unused ( inDataList, inIndex, outDataNode )
	return( eNoLongerSupported );
} // dsDataListGetNode

//--------------------------------------------------------------------------------------------------
//	Name:	dsDataListGetNodeAlloc
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsDataListGetNodeAlloc ( tDirReference			inDirRef,
									const tDataList			*inDataList,
									const unsigned long		inIndex,
									tDataNode			  **outDataNode )
{
	tDirStatus			tResult			= eDSNoErr;
	uInt32				uiLength		= 0;
	tDataBuffer		   *pOutDataNode	= nil;
	tDataNode		   *pCurrNode		= nil;
	tDataBufferPriv	   *pPrivData		= nil;

	// NULL check in data list
	if ( inDataList == nil )
	{
		return( eDSNullDataList );
	}

	// NULL check in data list head pointer
	if ( inDataList->fDataListHead == nil )
	{
		return( eDSEmptyDataList );
	}

	pCurrNode = ::dsGetThisNodePriv( inDataList->fDataListHead, inIndex );
	if ( pCurrNode == nil )
	{
		return( eDSIndexOutOfRange );
	}

	if ( outDataNode == nil )
	{
		return( eDSNullTargetArgument );
	}

	pPrivData = (tDataBufferPriv *)pCurrNode;
	uiLength = pPrivData->fBufferLength;

	pOutDataNode = ::dsDataBufferAllocate( inDirRef, uiLength );
	if ( pOutDataNode != nil )
	{
		::memcpy( pOutDataNode->fBufferData, pPrivData->fBufferData, uiLength );
		pOutDataNode->fBufferSize = uiLength + 1;
		pOutDataNode->fBufferLength = uiLength;
		*outDataNode = pOutDataNode;
	}
	else
	{
		tResult = eMemoryAllocError;
	}

	return( tResult );

} // dsDataListGetNodeAlloc


//--------------------------------------------------------------------------------------------------
//	Name:	dsAllocAttributeValueEntry
//
//--------------------------------------------------------------------------------------------------

tAttributeValueEntryPtr dsAllocAttributeValueEntry ( tDirReference		inDirRef,
													 unsigned long		inAttrValueID,
													 void			   *inAttrValueData,
													 unsigned long		inAttrValueDataLen )
{
#pragma unused ( inDirRef )
	uInt32						uiDataSize	= 0;
	tAttributeValueEntryPtr		outEntryPtr	= nil;

	uiDataSize = sizeof( tAttributeValueEntry ) + inAttrValueDataLen + 1;
	outEntryPtr = (tAttributeValueEntry *)::calloc( 1, uiDataSize );
	if ( outEntryPtr != nil )
	{
		outEntryPtr->fAttributeValueID = inAttrValueID;
		::memcpy( outEntryPtr->fAttributeValueData.fBufferData, inAttrValueData, inAttrValueDataLen );
		outEntryPtr->fAttributeValueData.fBufferSize	= inAttrValueDataLen;
		outEntryPtr->fAttributeValueData.fBufferLength	= inAttrValueDataLen;
	}

	return( outEntryPtr );

} // dsAllocAttributeValueEntry


//--------------------------------------------------------------------------------------------------
//	Name:	dsDeallocAttributeValueEntry
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsDeallocAttributeValueEntry ( tDirReference				inDirRef,
										  tAttributeValueEntryPtr	inAttrValueEntry )
{
#pragma unused ( inDirRef )
	tDirStatus			tResult	= eDSNoErr;

	if ( inAttrValueEntry != nil )
	{
		free( inAttrValueEntry ); //sufficient since calloc above in dsAllocAttributeValueEntry done on all including the tDataNode
		inAttrValueEntry = nil;
	}

	return( tResult );

} // dsDeallocAttributeValueEntry


//--------------------------------------------------------------------------------------------------
//	Name:	dsDeallocAttributeEntry
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsDeallocAttributeEntry ( tDirReference		inDirRef,
									 tAttributeEntryPtr	inAttrEntry )
{
#pragma unused ( inDirRef )
	tDirStatus			tResult	= eDSNoErr;

	if ( inAttrEntry != nil )
	{
		free( inAttrEntry );	//sufficient since Add_tAttrEntry_ToMsg calloc done on all including the tDataNode
								//and Get_tRecordEntry_FromMsg retrieves it all
		inAttrEntry = nil;
	}

	return( tResult );
} // dsDeallocAttributeEntry


//--------------------------------------------------------------------------------------------------
//	Name:	dsDeallocRecordEntry
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsDeallocRecordEntry (	tDirReference		inDirRef,
									tRecordEntryPtr		inRecEntry )
{
#pragma unused ( inDirRef )
	tDirStatus			tResult	= eDSNoErr;

	if ( inRecEntry != nil )
	{
		free( inRecEntry );		//sufficient since all calloc's done on all data including the tDataNode
								//and Get_tAttrEntry_FromMsg retrieves it all as well
		inRecEntry = nil;
	}

	return( tResult );
} // dsDeallocRecordEntry


//--------------------------------------------------------------------------------------------------
//	Name:	dsGetRecordNameFromEntry
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsGetRecordNameFromEntry ( tRecordEntryPtr inRecEntryPtr, char **outRecName )
{
	tDirStatus		tResult		= eDSNoErr;
	uInt32			uiOffset	= 2;
	uInt32			uiBuffSize	= 0;
	uInt16			usLength	= 0;
	tDataNodePtr	dataNode 	= nil;
	char		   *pData	 	= nil;
	char		   *pOutData 	= nil;

	if (outRecName == nil)
	{
		tResult = eDSNullParameter;
		LOG3( kStdErr, "*** DS OSError: File: %s. Line: %d. Error = %d.\n", __FILE__, __LINE__, tResult );
	}
	else if ( inRecEntryPtr != nil )
	{
		dataNode = &inRecEntryPtr->fRecordNameAndType;
		if ( (dataNode->fBufferSize != 0) &&
			 (dataNode->fBufferLength != 0) &&
			 (dataNode->fBufferLength <= dataNode->fBufferSize) )
		{
			pData = dataNode->fBufferData;
			uiBuffSize = dataNode->fBufferSize;

			::memcpy( &usLength, pData, 2 );
			if ( (usLength == 0) || (usLength > (uiBuffSize - uiOffset)) )
			{
				tResult = eDSCorruptRecEntryData;
				LOG3( kStdErr, "*** DS OSError: File: %s. Line: %d. Error = %d.\n", __FILE__, __LINE__, tResult );
				LOG3( kStdErr, "***   length = %u. buff size = %u. offset = %u.\n", usLength, uiBuffSize, uiOffset );
			}
			else
			{
				if ( outRecName != nil )
				{
					pOutData = (char *)::calloc( usLength + 1, sizeof( char ) );
					if ( pOutData != nil )
					{
						::memcpy( pOutData, pData + 2, usLength );
						*outRecName = pOutData;
					}
				}
			}
		}
		else
		{
			tResult = eDSCorruptBuffer;
			LOG3( kStdErr, "*** DS OSError: File: %s. Line: %d. Error = %d.\n", __FILE__, __LINE__, tResult );
		}
	}
	else
	{
		tResult = eDSNullRecEntryPtr;
		LOG3( kStdErr, "*** DS OSError: File: %s. Line: %d. Error = %d.\n", __FILE__, __LINE__, tResult );
	}

	return( tResult );

} // dsGetRecordNameFromEntry


//--------------------------------------------------------------------------------------------------
//	Name:	dsGetRecordTypeFromEntry
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsGetRecordTypeFromEntry ( tRecordEntryPtr inRecEntryPtr, char **outRecType )
{
	tDirStatus		tResult		= eDSNoErr;
	uInt16			usLength	= 0;
	uInt32			uiOffset	= 2;
	uInt32			uiBuffSize	= 0;
	tDataNodePtr	dataNode 	= nil;
	char		   *pData	 	= nil;
	char		   *pOutData	= nil;

	if (outRecType == nil)
	{
		tResult = eDSNullParameter;
		LOG3( kStdErr, "*** DS OSError: File: %s. Line: %d. Error = %d.\n", __FILE__, __LINE__, tResult );
	}
	else if ( inRecEntryPtr != nil )
	{
		dataNode = &inRecEntryPtr->fRecordNameAndType;
		if ( (dataNode->fBufferSize != 0) &&
			 (dataNode->fBufferLength != 0) &&
			 (dataNode->fBufferLength <= dataNode->fBufferSize) )
		{
			pData = dataNode->fBufferData;
			uiBuffSize = dataNode->fBufferSize;

			::memcpy( &usLength, pData, 2 );
			if ( (usLength == 0) || (usLength > (uiBuffSize - uiOffset)) )
			{
				tResult = eDSCorruptRecEntryData;
				LOG3( kStdErr, "*** DS OSError: File: %s. Line: %d. Error = %d.\n", __FILE__, __LINE__, tResult );
				LOG3( kStdErr, "***   length = %u. buff size = %u. offset = %u.\n", usLength, uiBuffSize, uiOffset );
			}
			else
			{
				uiOffset += 2 + usLength;
				pData += 2 + usLength;

				::memcpy( &usLength, pData, 2 );
				if ( (usLength == 0) || (usLength > (uiBuffSize - uiOffset)) )
				{
					tResult = eDSCorruptRecEntryData;
					LOG3( kStdErr, "*** DS OSError: File: %s. Line: %d. Error = %d.\n", __FILE__, __LINE__, tResult );
					LOG3( kStdErr, "***   length = %u. buff size = %u. offset = %u.\n", usLength, uiBuffSize, uiOffset );
				}
				else
				{
					if ( outRecType != nil )
					{
						pOutData = (char *)::calloc( usLength + 1, sizeof( char ) );
						if ( pOutData != nil )
						{
							::memcpy( pOutData, pData + 2, usLength );

							*outRecType = pOutData;
						}
					}
				}
			}
		}
		else
		{
			tResult = eDSCorruptBuffer;
			LOG3( kStdErr, "*** DS OSError: File: %s. Line: %d. Error = %d.\n", __FILE__, __LINE__, tResult );
		}
	}
	else
	{
		tResult = eDSNullRecEntryPtr;
		LOG3( kStdErr, "*** DS OSError: File: %s. Line: %d. Error = %d.\n", __FILE__, __LINE__, tResult );
	}

	return( tResult );

} // dsGetRecordTypeFromEntry


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
//	Name:	dsGetThisNodePriv
//
//--------------------------------------------------------------------------------------------------

tDataNodePtr dsGetThisNodePriv ( tDataNode *inFirsNode, const unsigned long inIndex )
{
	uInt32				i			= 1;
	tDataNode		   *pCurrNode	= nil;
	tDataBufferPriv    *pPrivData	= nil;

	pCurrNode = inFirsNode;
	while ( pCurrNode != nil )
	{
		if ( i == inIndex )
		{
			break;
		}
		else
		{
			pPrivData = (tDataBufferPriv *)pCurrNode;
			pCurrNode = pPrivData->fNextPtr;
		}
		i++;
	}

	return( pCurrNode );

} // dsGetThisNodePriv


//--------------------------------------------------------------------------------------------------
//	Name:	dsGetLastNodePriv
//
//--------------------------------------------------------------------------------------------------

tDataNodePtr dsGetLastNodePriv ( tDataNode *inFirsNode )
{
	tDataNode		   *pCurrNode	= nil;
	tDataBufferPriv    *pPrivData	= nil;

	pCurrNode = inFirsNode;
	pPrivData = (tDataBufferPriv *)pCurrNode;

	while ( pPrivData->fNextPtr != nil )
	{
		pCurrNode = pPrivData->fNextPtr;
		pPrivData = (tDataBufferPriv *)pCurrNode;
	}

	return( pCurrNode );

} // dsGetLastNodePriv


//--------------------------------------------------------------------------------------------------
//	Name:	dsAllocListNodeFromStringPriv
//
//--------------------------------------------------------------------------------------------------

tDataNodePtr dsAllocListNodeFromStringPriv ( const char *inString )
{
	uInt32				nodeSize	= 0;
	uInt32				strLen		= 0;
	tDataNode		   *pOutNode	= nil;
	tDataBufferPriv	   *pPrivData	= nil;

	if ( inString != nil )
	{
		strLen = ::strlen( inString );
		nodeSize = sizeof( tDataBufferPriv ) + strLen + 1;
		pOutNode = (tDataNode *)::calloc( nodeSize, sizeof( char ) );
		if ( pOutNode != nil )
		{
			pOutNode->fBufferSize = nodeSize;
			pOutNode->fBufferLength = nodeSize;

			pPrivData = (tDataBufferPriv *)pOutNode;
			pPrivData->fBufferSize = strLen;
			pPrivData->fBufferLength = strLen;

			::strcpy( pPrivData->fBufferData, inString );
		}
	}

	return( pOutNode );

} // dsAllocListNodeFromStringPriv


//--------------------------------------------------------------------------------------------------
//	Name:	dsAllocListNodeFromBuffPriv
//
//--------------------------------------------------------------------------------------------------

tDataNodePtr dsAllocListNodeFromBuffPriv ( const void *inData, const uInt32 inSize )
{
	uInt32				nodeSize	= 0;
	tDataNode		   *pOutNode	= nil;
	tDataBufferPriv	   *pPrivData	= nil;

	if ( inData != nil )
	{
		nodeSize = sizeof( tDataBufferPriv ) + inSize + 1;
		pOutNode = (tDataNode *)::calloc( nodeSize, sizeof( char ) );
		if ( pOutNode != nil )
		{
			pOutNode->fBufferSize = nodeSize;
			pOutNode->fBufferLength = nodeSize;

			pPrivData = (tDataBufferPriv *)pOutNode;
			pPrivData->fBufferSize = inSize;
			pPrivData->fBufferLength = inSize;

			::memcpy( pPrivData->fBufferData, inData, inSize );
		}
	}

	return( pOutNode );

} // dsAllocListNodeFromBuffPriv


//--------------------------------------------------------------------------------------------------
//	Name:	dsVerifyDataListPriv
//
//--------------------------------------------------------------------------------------------------

tDirStatus dsVerifyDataListPriv ( const tDataList *inDataList )
{
	unsigned long		count		= 0;
	tDataNode		   *pCurrNode	= nil;
	tDataBufferPriv    *pPrivData	= nil;

	if ( inDataList == nil )
		return eDSNullDataList;

	pCurrNode = inDataList->fDataListHead;

	while ( pCurrNode != nil )
	{
		// Bump the count and limit loop lengths for bad or corrupted data.
		if ( ++count > inDataList->fDataNodeCount )
			break;

		pPrivData = (tDataBufferPriv *)pCurrNode;

		pCurrNode = pPrivData->fNextPtr;
	}

	if ( inDataList->fDataNodeCount == count )
		return eDSNoErr;

	LOG3( kStdErr, "*** DS DataList (@0x%lX) verification error: File: %s. Line: %d.\n",
			(unsigned long) inDataList, __FILE__, __LINE__ );
	// Probably should have a custom error for this condition.
	return eDSInvalidBuffFormat;
} // dsVerifyDataListPriv


// ---------------------------------------------------------------------------
//	* dsParseAuthAuthority
//    retrieve version, tag, and data from authauthority
//    format is version;tag;data
// ---------------------------------------------------------------------------

tDirStatus dsParseAuthAuthority( const char *inAuthAuthority, char **outVersion, char **outAuthTag, char **outAuthData )
{
	char* authAuthority = NULL;
	char* current = NULL;
	char* tempPtr = NULL;
	tDirStatus result = eDSAuthFailed;
	
	if ( inAuthAuthority == NULL || outVersion == NULL 
		 || outAuthTag == NULL || outAuthData == NULL )
	{
		return eDSAuthFailed;
	}
	authAuthority = strdup(inAuthAuthority);
	if (authAuthority == NULL)
	{
		return eDSAuthFailed;
	}
	current = authAuthority;
	do {
		tempPtr = strsep(&current, ";");
		if (tempPtr == NULL) break;
		*outVersion = strdup(tempPtr);
		
		tempPtr = strsep(&current, ";");
		if (tempPtr == NULL) break;
		*outAuthTag = strdup(tempPtr);
		
		tempPtr = strsep(&current, ";");
		if (tempPtr == NULL) break;
		*outAuthData = strdup(tempPtr);
		
		result = eDSNoErr;
	} while (false);
	
	free(authAuthority);
	authAuthority = NULL;
	
	if (result != eDSNoErr)
	{
		if (*outVersion != NULL)
		{
			free(*outVersion);
			*outVersion = NULL;
		}
		if (*outAuthTag != NULL)
		{
			free(*outAuthTag);
			*outAuthTag = NULL;
		}
		if (*outAuthData != NULL)
		{
			free(*outAuthData);
			*outAuthData = NULL;
		}
	}
	return result;
} // dsParseAuthAuthority


//--------------------------------------------------------------------------------------------------
//	Name:	dsCopyDirStatusName
//
//--------------------------------------------------------------------------------------------------

char* dsCopyDirStatusName ( long inDirStatus )
{
	char		   *outString   = nil;
	unsigned long   caseIndex   = (-1 * inDirStatus) - 14000;
	
	if (inDirStatus == 0)
	{
		outString = strdup("eDSNoErr");
	}
	else if ( (caseIndex < 1000) && (caseIndex > 0) )
	{
		switch (caseIndex)
		{
			case 0:
				outString = strdup("eDSOpenFailed");
				break;
			case 1:
				outString = strdup("eDSCloseFailed");
				break;
			case 2:
				outString = strdup("eDSOpenNodeFailed");
				break;
			case 3:
				outString = strdup("eDSBadDirRefences");
				break;
			case 4:
				outString = strdup("eDSNullRecordReference");
				break;
			case 5:
				outString = strdup("eDSMaxSessionsOpen");
				break;
			case 6:
				outString = strdup("eDSCannotAccessSession");
				break;
			case 7:
				outString = strdup("eDSDirSrvcNotOpened");
				break;
			case 8:
				outString = strdup("eDSNodeNotFound");
				break;
			case 9:
				outString = strdup("eDSUnknownNodeName");
				break;
			case 10:
				outString = strdup("eDSRegisterCustomFailed");
				break;
			case 11:
				outString = strdup("eDSGetCustomFailed");
				break;
			case 12:
				outString = strdup("eDSUnRegisterFailed");
				break;
			case 50:
				outString = strdup("eDSAllocationFailed");
				break;
			case 51:
				outString = strdup("eDSDeAllocateFailed");
				break;
			case 52:
				outString = strdup("eDSCustomBlockFailed");
				break;
			case 53:
				outString = strdup("eDSCustomUnblockFailed");
				break;
			case 54:
				outString = strdup("eDSCustomYieldFailed");
				break;
			case 60:
				outString = strdup("eDSCorruptBuffer");
				break;
			case 61:
				outString = strdup("eDSInvalidIndex");
				break;
			case 62:
				outString = strdup("eDSIndexOutOfRange");
				break;
			case 63:
				outString = strdup("eDSIndexNotFound");
				break;
			case 65:
				outString = strdup("eDSCorruptRecEntryData");
				break;
			case 69:
				outString = strdup("eDSRefSpaceFull");
				break;
			case 70:
				outString = strdup("eDSRefTableAllocError");
				break;
			case 71:
				outString = strdup("eDSInvalidReference");
				break;
			case 72:
				outString = strdup("eDSInvalidRefType");
				break;
			case 73:
				outString = strdup("eDSInvalidDirRef");
				break;
			case 74:
				outString = strdup("eDSInvalidNodeRef");
				break;
			case 75:
				outString = strdup("eDSInvalidRecordRef");
				break;
			case 76:
				outString = strdup("eDSInvalidAttrListRef");
				break;
			case 77:
				outString = strdup("eDSInvalidAttrValueRef");
				break;
			case 78:
				outString = strdup("eDSInvalidContinueData");
				break;
			case 79:
				outString = strdup("eDSInvalidBuffFormat");
				break;
			case 80:
				outString = strdup("eDSInvalidPatternMatchType");
				break;
			case 81:
				outString = strdup("eDSRefTableError");
				break;
			case 82:
				outString = strdup("eDSRefTableNilError");
				break;
			case 83:
				outString = strdup("eDSRefTableIndexOutOfBoundsError");
				break;
			case 84:
				outString = strdup("eDSRefTableEntryNilError");
				break;
			case 85:
				outString = strdup("eDSRefTableCSBPAllocError");
				break;
			case 86:
				outString = strdup("eDSRefTableFWAllocError");
				break;
			case 90:
				outString = strdup("eDSAuthFailed");
				break;
			case 91:
				outString = strdup("eDSAuthMethodNotSupported");
				break;
			case 92:
				outString = strdup("eDSAuthResponseBufTooSmall");
				break;
			case 93:
				outString = strdup("eDSAuthParameterError");
				break;
			case 94:
				outString = strdup("eDSAuthInBuffFormatError");
				break;
			case 95:
				outString = strdup("eDSAuthNoSuchEntity");
				break;
			case 96:
				outString = strdup("eDSAuthBadPassword");
				break;
			case 97:
				outString = strdup("eDSAuthContinueDataBad");
				break;
			case 98:
				outString = strdup("eDSAuthUnknownUser");
				break;
			case 99:
				outString = strdup("eDSAuthInvalidUserName");
				break;
			case 100:
				outString = strdup("eDSAuthCannotRecoverPasswd");
				break;
			case 101:
				outString = strdup("eDSAuthFailedClearTextOnly");
				break;
			case 102:
				outString = strdup("eDSAuthNoAuthServerFound");
				break;
			case 103:
				outString = strdup("eDSAuthServerError");
				break;
			case 104:
				outString = strdup("eDSInvalidContext");
				break;
			case 105:
				outString = strdup("eDSBadContextData");
				break;
			case 120:
				outString = strdup("eDSPermissionError");
				break;
			case 121:
				outString = strdup("eDSReadOnly");
				break;
			case 122:
				outString = strdup("eDSInvalidDomain");
				break;
			case 123:
				outString = strdup("eNetInfoError");
				break;
			case 130:
				outString = strdup("eDSInvalidRecordType");
				break;
			case 131:
				outString = strdup("eDSInvalidAttributeType");
				break;
			case 133:
				outString = strdup("eDSInvalidRecordName");
				break;
			case 134:
				outString = strdup("eDSAttributeNotFound");
				break;
			case 135:
				outString = strdup("eDSRecordAlreadyExists");
				break;
			case 136:
				outString = strdup("eDSRecordNotFound");
				break;
			case 137:
				outString = strdup("eDSAttributeDoesNotExist");
				break;
			case 140:
				outString = strdup("eDSNoStdMappingAvailable");
				break;
			case 141:
				outString = strdup("eDSInvalidNativeMapping");
				break;
			case 142:
				outString = strdup("eDSSchemaError");
				break;
			case 143:
				outString = strdup("eDSAttributeValueNotFound");
				break;
			case 149:
				outString = strdup("eDSVersionMismatch");
				break;
			case 150:
				outString = strdup("eDSPlugInConfigFileError");
				break;
			case 151:
				outString = strdup("eDSInvalidPlugInConfigData");
				break;
			case 161:
				outString = strdup("eDSAuthNewPasswordRequired");
				break;
			case 162:
				outString = strdup("eDSAuthPasswordExpired");
				break;
			case 165:
				outString = strdup("eDSAuthPasswordQualityCheckFailed");
				break;
			case 167:
				outString = strdup("eDSAuthAccountDisabled");
				break;
			case 168:
				outString = strdup("eDSAuthAccountExpired");
				break;
			case 169:
				outString = strdup("eDSAuthAccountInactive");
				break;
			case 170:
				outString = strdup("eDSAuthPasswordTooShort");
				break;
			case 171:
				outString = strdup("eDSAuthPasswordTooLong");
				break;
			case 172:
				outString = strdup("eDSAuthPasswordNeedsLetter");
				break;
			case 173:
				outString = strdup("eDSAuthPasswordNeedsDigit");
				break;
			case 174:
				outString = strdup("eDSAuthPasswordChangeTooSoon");
				break;
			case 175:
				outString = strdup("eDSAuthInvalidLogonHours");
				break;
			case 176:
				outString = strdup("eDSAuthInvalidComputer");
				break;
			case 177:
				outString = strdup("eDSAuthMasterUnreachable");
				break;
			case 200:
				outString = strdup("eDSNullParameter");
				break;
			case 201:
				outString = strdup("eDSNullDataBuff");
				break;
			case 202:
				outString = strdup("eDSNullNodeName");
				break;
			case 203:
				outString = strdup("eDSNullRecEntryPtr");
				break;
			case 204:
				outString = strdup("eDSNullRecName");
				break;
			case 205:
				outString = strdup("eDSNullRecNameList");
				break;
			case 206:
				outString = strdup("eDSNullRecType");
				break;
			case 207:
				outString = strdup("eDSNullRecTypeList");
				break;
			case 208:
				outString = strdup("eDSNullAttribute");
				break;
			case 209:
				outString = strdup("eDSNullAttributeAccess");
				break;
			case 210:
				outString = strdup("eDSNullAttributeValue");
				break;
			case 211:
				outString = strdup("eDSNullAttributeType");
				break;
			case 212:
				outString = strdup("eDSNullAttributeTypeList");
				break;
			case 213:
				outString = strdup("eDSNullAttributeControlPtr");
				break;
			case 214:
				outString = strdup("eDSNullAttributeRequestList");
				break;
			case 215:
				outString = strdup("eDSNullDataList");
				break;
			case 216:
				outString = strdup("eDSNullDirNodeTypeList");
				break;
			case 217:
				outString = strdup("eDSNullAutMethod");
				break;
			case 218:
				outString = strdup("eDSNullAuthStepData");
				break;
			case 219:
				outString = strdup("eDSNullAuthStepDataResp");
				break;
			case 220:
				outString = strdup("eDSNullNodeInfoTypeList");
				break;
			case 221:
				outString = strdup("eDSNullPatternMatch");
				break;
			case 222:
				outString = strdup("eDSNullNodeNamePattern");
				break;
			case 223:
				outString = strdup("eDSNullTargetArgument");
				break;
			case 230:
				outString = strdup("eDSEmptyParameter");
				break;
			case 231:
				outString = strdup("eDSEmptyBuffer");
				break;
			case 232:
				outString = strdup("eDSEmptyNodeName");
				break;
			case 233:
				outString = strdup("eDSEmptyRecordName");
				break;
			case 234:
				outString = strdup("eDSEmptyRecordNameList");
				break;
			case 235:
				outString = strdup("eDSEmptyRecordType");
				break;
			case 236:
				outString = strdup("eDSEmptyRecordTypeList");
				break;
			case 237:
				outString = strdup("eDSEmptyRecordEntry");
				break;
			case 238:
				outString = strdup("eDSEmptyPatternMatch");
				break;
			case 239:
				outString = strdup("eDSEmptyNodeNamePattern");
				break;
			case 240:
				outString = strdup("eDSEmptyAttribute");
				break;
			case 241:
				outString = strdup("eDSEmptyAttributeType");
				break;
			case 242:
				outString = strdup("eDSEmptyAttributeTypeList");
				break;
			case 243:
				outString = strdup("eDSEmptyAttributeValue");
				break;
			case 244:
				outString = strdup("eDSEmptyAttributeRequestList");
				break;
			case 245:
				outString = strdup("eDSEmptyDataList");
				break;
			case 246:
				outString = strdup("eDSEmptyNodeInfoTypeList");
				break;
			case 247:
				outString = strdup("eDSEmptyAuthMethod");
				break;
			case 248:
				outString = strdup("eDSEmptyAuthStepData");
				break;
			case 249:
				outString = strdup("eDSEmptyAuthStepDataResp");
				break;
			case 250:
				outString = strdup("eDSEmptyPattern2Match");
				break;
			case 255:
				outString = strdup("eDSBadDataNodeLength");
				break;
			case 256:
				outString = strdup("eDSBadDataNodeFormat");
				break;
			case 257:
				outString = strdup("eDSBadSourceDataNode");
				break;
			case 258:
				outString = strdup("eDSBadTargetDataNode");
				break;
			case 260:
				outString = strdup("eDSBufferTooSmall");
				break;
			case 261:
				outString = strdup("eDSUnknownMatchType");
				break;
			case 262:
				outString = strdup("eDSUnSupportedMatchType");
				break;
			case 263:
				outString = strdup("eDSInvalDataList");
				break;
			case 264:
				outString = strdup("eDSAttrListError");
				break;
			case 270:
				outString = strdup("eServerNotRunning");
				break;
			case 271:
				outString = strdup("eUnknownAPICall");
				break;
			case 272:
				outString = strdup("eUnknownServerError");
				break;
			case 273:
				outString = strdup("eUnknownPlugIn");
				break;
			case 274:
				outString = strdup("ePlugInDataError");
				break;
			case 275:
				outString = strdup("ePlugInNotFound");
				break;
			case 276:
				outString = strdup("ePlugInError");
				break;
			case 277:
				outString = strdup("ePlugInInitError");
				break;
			case 278:
				outString = strdup("ePlugInNotActive");
				break;
			case 279:
				outString = strdup("ePlugInFailedToInitialize");
				break;
			case 280:
				outString = strdup("ePlugInCallTimedOut");
				break;
			case 290:
				outString = strdup("eNoSearchNodesFound");
				break;
			case 291:
				outString = strdup("eSearchPathNotDefined");
				break;
			case 292:
				outString = strdup("eNotHandledByThisNode");
				break;
			case 330:
				outString = strdup("eIPCSendError");
				break;
			case 331:
				outString = strdup("eIPCReceiveError");
				break;
			case 332:
				outString = strdup("eServerReplyError");
				break;
			case 350:
				outString = strdup("eDSTCPSendError");
				break;
			case 351:
				outString = strdup("eDSTCPReceiveError");
				break;
			case 352:
				outString = strdup("eDSTCPVersionMismatch");
				break;
			case 353:
				outString = strdup("eDSIPUnreachable");
				break;
			case 354:
				outString = strdup("eDSUnknownHost");
				break;
			case 400:
				outString = strdup("ePluginHandlerNotLoaded");
				break;
			case 402:
				outString = strdup("eNoPluginsLoaded");
				break;
			case 404:
				outString = strdup("ePluginAlreadyLoaded");
				break;
			case 406:
				outString = strdup("ePluginVersionNotFound");
				break;
			case 408:
				outString = strdup("ePluginNameNotFound");
				break;
			case 410:
				outString = strdup("eNoPluginFactoriesFound");
				break;
			case 412:
				outString = strdup("ePluginConfigAvailNotFound");
				break;
			case 414:
				outString = strdup("ePluginConfigFileNotFound");
				break;
			case 450:
				outString = strdup("eCFMGetFileSysRepErr");
				break;
			case 452:
				outString = strdup("eCFPlugInGetBundleErr");
				break;
			case 454:
				outString = strdup("eCFBndleGetInfoDictErr");
				break;
			case 456:
				outString = strdup("eCFDictGetValueErr");
				break;
			case 470:
				outString = strdup("eDSServerTimeout");
				break;
			case 471:
				outString = strdup("eDSContinue");
				break;
			case 472:
				outString = strdup("eDSInvalidHandle");
				break;
			case 473:
				outString = strdup("eDSSendFailed");
				break;
			case 474:
				outString = strdup("eDSReceiveFailed");
				break;
			case 475:
				outString = strdup("eDSBadPacket");
				break;
			case 476:
				outString = strdup("eDSInvalidTag");
				break;
			case 477:
				outString = strdup("eDSInvalidSession");
				break;
			case 478:
				outString = strdup("eDSInvalidName");
				break;
			case 479:
				outString = strdup("eDSUserUnknown");
				break;
			case 480:
				outString = strdup("eDSUnrecoverablePassword");
				break;
			case 481:
				outString = strdup("eDSAuthenticationFailed");
				break;
			case 482:
				outString = strdup("eDSBogusServer");
				break;
			case 483:
				outString = strdup("eDSOperationFailed");
				break;
			case 484:
				outString = strdup("eDSNotAuthorized");
				break;
			case 485:
				outString = strdup("eDSNetInfoError");
				break;
			case 486:
				outString = strdup("eDSContactMaster");
				break;
			case 487:
				outString = strdup("eDSServiceUnavailable");
				break;
			case 501:
				outString = strdup("eFWGetDirNodeNameErr1");
				break;
			case 502:
				outString = strdup("eFWGetDirNodeNameErr2");
				break;
			case 503:
				outString = strdup("eFWGetDirNodeNameErr3");
				break;
			case 504:
				outString = strdup("eFWGetDirNodeNameErr4");
				break;
			case 700:
				outString = strdup("eParameterSendError");
				break;
			case 720:
				outString = strdup("eParameterReceiveError");
				break;
			case 740:
				outString = strdup("eServerSendError");
				break;
			case 760:
				outString = strdup("eServerReceiveError");
				break;
			case 900:
				outString = strdup("eMemoryError");
				break;
			case 901:
				outString = strdup("eMemoryAllocError");
				break;
			case 910:
				outString = strdup("eServerError");
				break;
			case 915:
				outString = strdup("eParameterError");
				break;
			case 950:
				outString = strdup("eDataReceiveErr_NoDirRef");
				break;
			case 951:
				outString = strdup("eDataReceiveErr_NoRecRef");
				break;
			case 952:
				outString = strdup("eDataReceiveErr_NoAttrListRef");
				break;
			case 953:
				outString = strdup("eDataReceiveErr_NoAttrValueListRef");
				break;
			case 954:
				outString = strdup("eDataReceiveErr_NoAttrEntry");
				break;
			case 955:
				outString = strdup("eDataReceiveErr_NoAttrValueEntry");
				break;
			case 956:
				outString = strdup("eDataReceiveErr_NoNodeCount");
				break;
			case 957:
				outString = strdup("eDataReceiveErr_NoAttrCount");
				break;
			case 958:
				outString = strdup("eDataReceiveErr_NoRecEntry");
				break;
			case 959:
				outString = strdup("eDataReceiveErr_NoRecEntryCount");
				break;
			case 960:
				outString = strdup("eDataReceiveErr_NoRecMatchCount");
				break;
			case 961:
				outString = strdup("eDataReceiveErr_NoDataBuff");
				break;
			case 962:
				outString = strdup("eDataReceiveErr_NoContinueData");
				break;
			case 963:
				outString = strdup("eDataReceiveErr_NoNodeChangeToken");
				break;
			case 986:
				outString = strdup("eNoLongerSupported");
				break;
			case 987:
				outString = strdup("eUndefinedError");
				break;
			case 988:
				outString = strdup("eNotYetImplemented");
				break;
			case 999:
				outString = strdup("eDSLastValue");
				break;
			default:
				break;
		}
	}
	else
	{
		outString = strdup("Not a known DirStatus");
	}
	
	return(outString);
} // dsCopyDirStatusName


tDirStatus dsFillAuthBuffer (	tDataBufferPtr inOutAuthBuffer,
								unsigned long inCount,
								unsigned long inLen,
								const void *inData, ... )
{
	unsigned long		curr		= 0;
	unsigned long		buffSize	= 0;
	unsigned long		count		= inCount;
	unsigned long		len			= inLen;
	const void			*data		= inData;
	bool				firstPass	= true;
	char				*p			= NULL;
	va_list				args;
	
	if ( inOutAuthBuffer == NULL || inOutAuthBuffer->fBufferData == NULL || inCount == 0 )
		return eParameterError;
	
	// Make sure we have data to copy
	if ( (inLen != 0) && (inData == NULL) )
		return eParameterError;
	
	// Get buffer info
	p		 = inOutAuthBuffer->fBufferData;
	buffSize = inOutAuthBuffer->fBufferSize;
	
	// Set up the arg list
	va_start( args, inData );

	while ( count-- > 0 )
	{
		if ( !firstPass )
		{
			len = va_arg( args, unsigned long );
			data = va_arg( args, void * );
		}
		
		if ( (curr + len) > buffSize )
		{
			// Out of buffer space, bail
			return eDSBufferTooSmall;
		}
		
		::memcpy( &(p[curr]), &len, sizeof( sInt32 ) );
		curr += sizeof( sInt32 );
		
		if ( len > 0 && data != NULL )
		{
			memcpy( &(p[curr]), data, len );
			curr += len;
		}
		firstPass = false;
	}
	
	inOutAuthBuffer->fBufferLength = curr;
	
	return eDSNoErr;
} // dsFillAuthBuffer


tDirStatus dsAppendAuthBuffer (	tDataBufferPtr inOutAuthBuffer,
								unsigned long inCount,
								unsigned long inLen,
								const void *inData, ... )
{
	unsigned long		curr		= 0;
	unsigned long		buffSize	= 0;
	unsigned long		count		= inCount;
	unsigned long		len			= inLen;
	const void			*data		= inData;
	bool				firstPass	= true;
	char				*p			= NULL;
	va_list				args;
	
	if ( inOutAuthBuffer == NULL || inOutAuthBuffer->fBufferData == NULL || inCount == 0 )
		return eParameterError;
	
	// Make sure we have data to copy
	if ( (inLen != 0) && (inData == NULL) )
		return eParameterError;
	
	// Get buffer info
	p		 = inOutAuthBuffer->fBufferData + inOutAuthBuffer->fBufferLength;
	buffSize = inOutAuthBuffer->fBufferSize - inOutAuthBuffer->fBufferLength;
	
	// Set up the arg list
	va_start( args, inData );

	while ( count-- > 0 )
	{
		if ( !firstPass )
		{
			len = va_arg( args, unsigned long );
			data = va_arg( args, void * );
		}
		
		if ( (curr + len) > buffSize )
		{
			// Out of buffer space, bail
			return eDSBufferTooSmall;
		}
		
		::memcpy( &(p[curr]), &len, sizeof( sInt32 ) );
		curr += sizeof( sInt32 );
		
		if ( len > 0 && data != NULL )
		{
			memcpy( &(p[curr]), data, len );
			curr += len;
		}
		firstPass = false;
	}
	
	inOutAuthBuffer->fBufferLength += curr;
	
	return eDSNoErr;
} // dsAppendAuthBuffer


//-----------------------------------------------------------------------------
/*!
    @function dsAppendAuthBufferWithAuthorityAttribute
    @abstract   Inserts a user name with authentication authority data into
				an existing buffer.
    @discussion Use this function for authentication methods that contain user
				or authenticator names and the authentication authority attribute
				has already been retrieved.
    @param      inNodeRef a node reference for the record to parse
    @param      inRecordListBuffPtr the data returned from dsGetDataList
    @param      inAttributePtr an attribute with authentication authority data
    @param      inValueRef the reference for the kDSNAttrAuthenticationAuthority
							attribute.
    @param      inUserName the name of the user to authenticate
    @param      inOutAuthBuffer pass in a preallocated buffer, returns with
				the user data appended.
    @result    tDirStatus code
*/
//-----------------------------------------------------------------------------

tDirStatus dsAppendAuthBufferWithAuthorityAttribute(
	tDirNodeReference inNodeRef,
	tDataBufferPtr inRecordListBuffPtr,
	tAttributeEntryPtr inAttributePtr,
	tAttributeValueListRef inValueRef,
	const char *inUserName,
	tDataBufferPtr inOutAuthBuffer )
{
    tDirStatus					status				= eDSNoErr;
	tDataBufferPtr				innerDataBuff		= NULL;
	tAttributeValueEntry	   *attrValue			= NULL;
	unsigned long				attrValIndex		= 0;
    unsigned long				attrValCount		= 0;

	if ( inOutAuthBuffer == NULL )
		return eParameterError;
	
	attrValCount = inAttributePtr->fAttributeValueCount;
	innerDataBuff = dsDataBufferAllocate( 0, inAttributePtr->fAttributeDataSize + sizeof(uInt32) * attrValCount );
	if ( innerDataBuff != NULL )
	{
		// run through the values create a tDataBuffer with the list of authentication authorities.
		for ( attrValIndex = 1; attrValIndex <= attrValCount && status == eDSNoErr; attrValIndex++ )
		{
			status = dsGetAttributeValue( inNodeRef, inRecordListBuffPtr, attrValIndex, inValueRef, &attrValue );
			if ( status == eDSNoErr )
			{
				status = dsAppendAuthBuffer( innerDataBuff, 1, attrValue->fAttributeValueData.fBufferLength,
												attrValue->fAttributeValueData.fBufferData );
				dsDeallocAttributeValueEntry( 0, attrValue );
			}
		}
		
		status = dsAppendAuthBufferWithAuthority( inUserName, innerDataBuff, inOutAuthBuffer );
				
		dsDataBufferDeAllocate( 0, innerDataBuff );
	}
	
	return status;
}


//-----------------------------------------------------------------------------
/*!
    @function dsAppendAuthBufferWithAuthorityStrings
    @abstract   Inserts a user name with authentication authority data into
				an existing buffer.
    @discussion Use this function for authentication methods that contain user
				or authenticator names and the authentication authority attribute
				has already been retrieved.
    @param      inUserName the name of the user to authenticate
	@param		inAuthAuthority a NULL terminated array of C strings
    @param      inOutAuthBuffer pass in a preallocated buffer, returns with
				the user data appended.
    @result    tDirStatus code
*/
//-----------------------------------------------------------------------------

tDirStatus dsAppendAuthBufferWithAuthorityStrings(
	const char *inUserName,
	const char *inAuthAuthority[],
	tDataBufferPtr inOutAuthBuffer )
{
    tDirStatus					status				= eDSNoErr;
	tDataBufferPtr				innerDataBuff		= NULL;
	const char					*tptr				= NULL;
	unsigned long				attrValIndex		= 0;
    unsigned long				attrValCount		= 0;
	unsigned long				neededSize			= sizeof(tDataBuffer);
	
	if ( inOutAuthBuffer == NULL )
		return eParameterError;
	
	for ( tptr = inAuthAuthority[0]; tptr != NULL; )
	{
		neededSize += sizeof(long) + strlen( tptr );
		tptr = inAuthAuthority[++attrValCount];
	}
	
	innerDataBuff = dsDataBufferAllocate( 0, neededSize );
	if ( innerDataBuff != NULL )
	{
		// run through the values create a tDataBuffer with the list of authentication authorities.
		for ( attrValIndex = 0; attrValIndex < attrValCount && status == eDSNoErr; attrValIndex++ )
		{
			status = dsAppendAuthBuffer( innerDataBuff, 1, strlen(inAuthAuthority[attrValIndex]),
												inAuthAuthority[attrValIndex] );
		}
		
		if ( status == eDSNoErr )
			status = dsAppendAuthBufferWithAuthority( inUserName, innerDataBuff, inOutAuthBuffer );
		
		dsDataBufferDeAllocate( 0, innerDataBuff );
	}
	
	return status;
}


//-----------------------------------------------------------------------------
/*!
    @function dsAppendAuthBufferWithAuthority
    @abstract   Inserts a user name with authentication authority data into
				an existing buffer.
    @discussion Internal static function for appending data to a buffer.
    @param      inUserName the name of the user to authenticate
    @param      inOutAuthBuffer pass in a preallocated buffer, returns with
				the user data appended.
    @result    tDirStatus code
*/
//-----------------------------------------------------------------------------

tDirStatus dsAppendAuthBufferWithAuthority(
	const char *inUserName,
	tDataBufferPtr inAuthAuthorityBuffer,
	tDataBufferPtr inOutAuthBuffer )
{
    tDirStatus				status				= eDSNoErr;
	int						userNameLen			= 0;
	int						userBufferLen		= 0;
	char					*userBuffer			= NULL;
	
	if ( inUserName == NULL || inAuthAuthorityBuffer == NULL || inOutAuthBuffer == NULL )
		return eParameterError;
	
	userNameLen = strlen( inUserName );
	userBufferLen = userNameLen + sizeof(kDSUserAuthAuthorityMarker) + sizeof(tDataBuffer) +
					inAuthAuthorityBuffer->fBufferLength;
	
	userBuffer = (char *) malloc( userBufferLen );
	if ( userBuffer == NULL )
		return eMemoryError;
	
	strcpy( userBuffer, inUserName );
	strcpy( userBuffer + userNameLen + 1, kDSUserAuthAuthorityMarker );
	memcpy( userBuffer + userNameLen + sizeof(kDSUserAuthAuthorityMarker), inAuthAuthorityBuffer,
			sizeof(tDataBuffer) + inAuthAuthorityBuffer->fBufferLength );
	status = dsAppendAuthBuffer( inOutAuthBuffer, 1, userBufferLen, userBuffer );
	free( userBuffer );
	
	return status;
}

