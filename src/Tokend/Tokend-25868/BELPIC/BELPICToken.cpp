/*
 *  Copyright (c) 2004 Apple Computer, Inc. All Rights Reserved.
 * 
 *  @APPLE_LICENSE_HEADER_START@
 *  
 *  This file contains Original Code and/or Modifications of Original Code
 *  as defined in and that are subject to the Apple Public Source License
 *  Version 2.0 (the 'License'). You may not use this file except in
 *  compliance with the License. Please obtain a copy of the License at
 *  http://www.opensource.apple.com/apsl/ and read it before using this
 *  file.
 *  
 *  The Original Code and all software distributed under the License are
 *  distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 *  EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 *  INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 *  Please see the License for the specific language governing rights and
 *  limitations under the License.
 *  
 *  @APPLE_LICENSE_HEADER_END@
 */

/*
 *  BELPICToken.cpp
 *  TokendMuscle
 */

#include "BELPICToken.h"

#include "Adornment.h"
#include "AttributeCoder.h"
#include "BELPICError.h"
#include "BELPICRecord.h"
#include "BELPICSchema.h"
#include <security_cdsa_client/aclclient.h>
#include <map>
#include <vector>

using CssmClient::AclFactory;

#define OFF_CLA  0
#define OFF_INS  1
#define OFF_P1   2
#define OFF_P2   3
#define OFF_LC   4
#define OFF_DATA 5

#define CLA_STANDARD      0x00
#define INS_SELECT_FILE   0xA4
#define INS_MANAGE_SECURITY_ENVIRONMENT   0x22

#define P1_SELECT_APPLET  0x04
#define P2_SELECT_APPLET  0x0C

#define SELECT_APPLET \
	CLA_STANDARD, INS_SELECT_FILE, P1_SELECT_APPLET, P2_SELECT_APPLET

//static const unsigned char kBELPICPKCS15Applet[] =
//	{ 0xA0, 0x00, 0x00, 0x01, 0x77, 0x50, 0x4B, 0x43, 0x53, 0x2D, 0x31, 0x35 };

static const unsigned char kDF_BELPIC[] =           { 0xDF, 0x00 };
static const unsigned char kDF_ID[] =               { 0xDF, 0x01 };

static const unsigned char kEF_DIR[] =              { 0x2F, 0x00 };

static const unsigned char kBELPIC_EF_ODF[] =       { 0x50, 0x31 };
static const unsigned char kBELPIC_EF_TokenInfo[] = { 0x50, 0x32 };
static const unsigned char kBELPIC_EF_AODF[] =      { 0x50, 0x34 };
static const unsigned char kBELPIC_EF_PrKDF[] =     { 0x50, 0x35 };
static const unsigned char kBELPIC_EF_PukDF[] =     { 0x50, 0x36 };
static const unsigned char kBELPIC_EF_CDF[] =       { 0x50, 0x37 };
static const unsigned char kBELPIC_EF_Cert2[] =     { 0x50, 0x38 };
static const unsigned char kBELPIC_EF_Cert3[] =     { 0x50, 0x39 };
static const unsigned char kBELPIC_EF_Cert4[] =     { 0x50, 0x3A };
static const unsigned char kBELPIC_EF_Cert6[] =     { 0x50, 0x3B };
static const unsigned char kBELPIC_EF_Cert8[] =     { 0x50, 0x3C };

static const unsigned char kID_EF_ID_RN[] =         { 0x40, 0x31 };
static const unsigned char kID_EF_SGN_RN[] =        { 0x40, 0x32 };
static const unsigned char kID_EF_ID_ADDRESS[] =    { 0x40, 0x33 };
static const unsigned char kID_EF_SGN_ADDRESS[] =   { 0x40, 0x34 };
static const unsigned char kID_EF_ID_PHOTO[] =      { 0x40, 0x35 };
static const unsigned char kID_EF_PuK7_ID[] =       { 0x40, 0x38 };
static const unsigned char kID_EF_Preferences[] =   { 0x40, 0x39 };

static const unsigned char kPIN_Cardholder_Id[] =   { 0x01 };
static const unsigned char kPIN_Reset_Id[] =        { 0x02 };
static const unsigned char kPUK_Unblock_Id[] =      { 0x03 };
static const unsigned char kPIN_Activate_Id[] =     { 0x84 };

static const unsigned char kPrK1_Id[] =             { 0x81 };
static const unsigned char kPrK2_Id[] =             { 0x82 };
static const unsigned char kPrK3_Id[] =             { 0x83 };
static const unsigned char kPuK5_Id[] =             { 0x85 };
static const unsigned char kPuK7_Id[] =             { 0x87 };


BELPICToken::BELPICToken() :
	mCurrentDF(NULL),
	mCurrentEF(NULL),
	mPinStatus(0)
{
	mTokenContext = this;
	mSession.open();
}

BELPICToken::~BELPICToken()
{
	delete mSchema;
}

void BELPICToken::select(const uint8_t *df, const uint8_t *ef)
{
	unsigned char result[MAX_BUFFER_SIZE];
	size_t resultLength = sizeof(result);
	if (isInTransaction() && mCurrentDF == df)
	{
		if (mCurrentEF == ef)
			return;

		uint8_t command[] = { 0x00, 0xA4, 0x02, 0x0C, 0x02, ef[0], ef[1] };
		BELPICError::check(exchangeAPDU(command, sizeof(command), result,
			resultLength));
		mCurrentEF = ef;
	}
	else
	{
		uint8_t command[] =
			{ 0x00, 0xA4, 0x08, 0x0C, 0x04, df[0], df[1], ef[0], ef[1] };
		BELPICError::check(exchangeAPDU(command, sizeof(command), result,
			resultLength));
		if (isInTransaction())
		{
			mCurrentDF = df;
			mCurrentEF = ef;
		}
	}
}

void BELPICToken::selectKeyForSign(const uint8_t *keyId)
{
	bool encrypt = true;
	uint8_t p1 = (encrypt ? 0x41 : 0x81);
    // Select signing, algorithm pkcs1 padding and key keyId
	unsigned char command[] =
		{ 0x00, 0x22, p1, 0xB6, 0x05, 0x04, 0x80, 0x01, 0x84, *keyId };
    // @@@ This would be the command when letting the card itself to the
	// DigestInfo wrapping for a SHA1 hash.
	//unsigned char command[] =
	//  { 0x00, 0x22, p1, 0xB6, 0x05, 0x04, 0x80, 0x02, 0x84, *keyId };

	//if (isInTransaction() && mCurrentKeyId == keyId)
	//	return;

	unsigned char result[MAX_BUFFER_SIZE];
	size_t resultLength = sizeof(result);
	BELPICError::check(exchangeAPDU(command, sizeof(command), result,
		resultLength));
}

#define READ_BLOCK_SIZE  0xF4

void BELPICToken::readBinary(uint8_t *result, size_t &resultLength)
{
	uint32_t offset = 0;

	// Attempt to read READ_BLOCK_SIZE bytes
	uint8_t command[] = { 0x00, 0xB0, 0x00, 0x00, READ_BLOCK_SIZE };
	uint32_t status;
    bool last_read = false;
	for (;;)
	{
		command[OFF_P1] = offset >> 8;
		command[OFF_P2] = offset & 0xFF;
		// Never read more than we have room for.
		size_t bytes_read = resultLength - offset;
		status = exchangeAPDU(command, sizeof(command), result + offset,
			bytes_read);
		bytes_read -= 2;
		offset += bytes_read;

		if ((status & SCARD_LE_IN_SW2) == SCARD_LE_IN_SW2)
        {
            command[OFF_LC] = status & 0xFF;
            last_read = true;  // The next read will be the last
            continue;
        }

		// We have read at least one byte already and we just happened to read
		// until the end of the file the last time, so now we are asking for a
		// block past the end of the file.  Which means we are done.
		if (status == SCARD_WRONG_PARAMETER_P1_P2 && offset > 0)
			break;

        BELPICError::check(status);

        if (bytes_read < command[OFF_LC])
		{
			// We recieved less bytes than we requsted, so we are done.
			break;
		}
		else if (bytes_read > command[OFF_LC])
		{
			// We got too many bytes that's a no no
			PCSC::Error::throwMe(SCARD_E_PROTO_MISMATCH);
		}

        if (last_read)
            break;
	}

	resultLength = offset;
}

uint32_t BELPICToken::exchangeAPDU(const uint8_t *apdu, size_t apduLength,
	uint8_t *result, size_t &resultLength)
{
	size_t savedLength = resultLength;

	transmit(apdu, apduLength, result, resultLength);
	if (resultLength == 2 && result[0] == 0x61)
	{
		resultLength = savedLength;
		uint8 expectedLength = result[1];
		unsigned char getResult[] = { 0x00, 0xC0, 0x00, 0x00, expectedLength };
		transmit(getResult, sizeof(getResult), result, resultLength);
		if (resultLength - 2 != expectedLength)
        {
            if (resultLength < 2)
                PCSC::Error::throwMe(SCARD_E_PROTO_MISMATCH);
            else
                BELPICError::throwMe((result[resultLength - 2] << 8)
					+ result[resultLength - 1]);
        }
	}

	if (resultLength < 2)
		PCSC::Error::throwMe(SCARD_E_PROTO_MISMATCH);

    return (result[resultLength - 2] << 8) + result[resultLength - 1];
}

void BELPICToken::didDisconnect()
{
	PCSC::Card::didDisconnect();
	mCurrentDF = NULL;
	mCurrentEF = NULL;
	mPinStatus = 0;
}

void BELPICToken::didEnd()
{
	PCSC::Card::didEnd();
	mCurrentDF = NULL;
	mCurrentEF = NULL;
	mPinStatus = 0;
}

uint8_t BELPICToken::pinDigit(uint8_t digit)
{
	if ('0' <= digit && digit <= '9')
		return digit - '0';
	else if ('A' <= digit && digit <= 'F')
		return digit - 'A' + 0x10;
	else if ('a' <= digit && digit <= 'f')
		return digit - 'a' + 0x10;
	else
		CssmError::throwMe(CSSM_ERRCODE_SAMPLE_VALUE_NOT_SUPPORTED);
}

void BELPICToken::changePIN(int pinNum,
	const unsigned char *oldPin, size_t oldPinLength,
	const unsigned char *newPin, size_t newPinLength)
{
	if (pinNum != 1)
		CssmError::throwMe(CSSM_ERRCODE_SAMPLE_VALUE_NOT_SUPPORTED);

	if (oldPinLength < 0 ||
		oldPinLength > BELPIC_MAX_PIN_LEN ||
		newPinLength < BELPIC_MIN_PIN_LEN ||
		newPinLength > BELPIC_MAX_PIN_LEN)
	{
		CssmError::throwMe(CSSM_ERRCODE_INVALID_SAMPLE_VALUE);
	}

	PCSC::Transaction _(*this);
	uint8_t apdu[] =
		{ 0x00, 0x24, 0x00, uint8_t(pinNum), 0x10,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
		};

	uint32_t offset = 5;

	apdu[offset++] = 0x20 + oldPinLength;
	for (uint32_t ix = 0; ix < oldPinLength;)
	{
		apdu[offset++] = (pinDigit(oldPin[ix++]) << 4) +
			(ix < oldPinLength ? pinDigit(oldPin[ix++]) : pinDigit('F'));
	}

	offset = 5 + 8;
	apdu[offset++] = 0x20 + newPinLength;
	for (uint32_t ix = 0; ix < newPinLength;)
	{
		apdu[offset++] = (pinDigit(newPin[ix++]) << 4) +
			(ix < newPinLength ? pinDigit(newPin[ix++]) : pinDigit('F'));
	}

	unsigned char result[MAX_BUFFER_SIZE];
	size_t resultLength = sizeof(result);
	mPinStatus = exchangeAPDU(apdu, sizeof(apdu), result, resultLength);
	memset(apdu + 5, 0, 16);
	BELPICError::check(mPinStatus);
}

uint32_t BELPICToken::pinStatus(int pinNum)
{
	if (pinNum != 1)
		CssmError::throwMe(CSSM_ERRCODE_SAMPLE_VALUE_NOT_SUPPORTED);

#if 0
	if (mPinStatus && isInTransaction())
		return mPinStatus;

	// Always checks PIN1
	PCSC::Transaction _(*this);

	unsigned char result[2];
	size_t resultLength = sizeof(result);
	unsigned char apdu[] = { 0x00, 0x20, 0x00, *kPIN_Cardholder_Id };

	mPinStatus = exchangeAPDU(apdu, 4, result, resultLength);
	if ((mPinStatus & 0xFF00) != 0x6300
		&& mPinStatus != SCARD_AUTHENTICATION_BLOCKED)
		BELPICError::check(mPinStatus);
#endif

	return mPinStatus;
}

void BELPICToken::verifyPIN(int pinNum, const uint8_t *pin, size_t pinLength)
{
	_verifyPIN(pinNum, pin, pinLength);
	// Start a new transaction which we never get rid of until someone calls
	// unverifyPIN()
	begin();
}

void BELPICToken::_verifyPIN(int pinNum, const uint8_t *pin, size_t pinLength)
{
	if (pinNum < 1 || pinNum > 3)
		CssmError::throwMe(CSSM_ERRCODE_SAMPLE_VALUE_NOT_SUPPORTED);

	if (pinLength < BELPIC_MIN_PIN_LEN || pinLength > BELPIC_MAX_PIN_LEN)
		CssmError::throwMe(CSSM_ERRCODE_INVALID_SAMPLE_VALUE);

	PCSC::Transaction _(*this);
#ifdef USE_BUILTIN_PIN
	uint8_t apdu[] =
		{ 0x00, 0x20, 0x00, 0x01, 0x08, 0x24,
			0x12, 0x34, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
#else
	uint8_t apdu[] =
		{ 0x00, 0x20, 0x00, uint8_t(pinNum), 0x08,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

	uint32_t offset = 5;

	apdu[offset++] = 0x20 + pinLength;
	for (uint32_t ix = 0; ix < pinLength;)
	{
		apdu[offset++] = (pinDigit(pin[ix++]) << 4) +
			(ix < pinLength ? pinDigit(pin[ix++]) : pinDigit('F'));
	}
#endif

	unsigned char result[MAX_BUFFER_SIZE];
	size_t resultLength = sizeof(result);
	mPinStatus = exchangeAPDU(apdu, sizeof(apdu), result, resultLength);
	memset(apdu + 5, 0, 8);
	BELPICError::check(mPinStatus);
}

void BELPICToken::unverifyPIN(int pinNum)
{
	if (pinNum != -1)
		CssmError::throwMe(CSSM_ERRCODE_SAMPLE_VALUE_NOT_SUPPORTED);

	end(SCARD_RESET_CARD);
}

uint32 BELPICToken::probe(SecTokendProbeFlags flags,
	char tokenUid[TOKEND_MAX_UID])
{
	uint32 score = Tokend::ISO7816Token::probe(flags, tokenUid);

	bool doDisconnect = false; /*!(flags & kSecTokendProbeKeepToken); */

	try
	{
		unsigned char result[MAX_BUFFER_SIZE];
		size_t resultLength = sizeof(result);

		{
			PCSC::Transaction _(*this);
			select(kDF_BELPIC, kBELPIC_EF_TokenInfo);
			readBinary(result, resultLength);
		}

		if (resultLength < 0x29 || memcmp(result + 0x19, "BELPIC", 6))
			doDisconnect = true;
		else
		{
			// If the length is not an exact match only return a score of 100
			score = resultLength == 29 ? 200 : 100;
			// @@@ If the ATR matches one of the built in BELPIC ATR's we
			// should probably return an even better score.

			// Setup the tokendUID
			memcpy(tokenUid, "BELPIC-", 7);
			uint32_t offset = 7;
			// Now stick in the chip serial # as hex bytes.
			for (uint32_t ix = 0x07; ix < 0x17; ++ix)
			{
				sprintf(tokenUid + offset, "%02X", result[ix]);
				offset += 2;
			}
			assert(TOKEND_MAX_UID > offset);
			memset(tokenUid + offset, 0, TOKEND_MAX_UID - offset);
			secdebug("probe", "recognized %s", tokenUid);
		}
	}
	catch (...)
	{
		doDisconnect = true;
		score = 0;
	}

	if (doDisconnect)
		disconnect();

	return score;
}

void BELPICToken::establish(const CSSM_GUID *guid, uint32 subserviceId,
	SecTokendEstablishFlags flags, const char *cacheDirectory,
	const char *workDirectory, char mdsDirectory[PATH_MAX],
	char printName[PATH_MAX])
{
	Tokend::ISO7816Token::establish(guid, subserviceId, flags,
		cacheDirectory, workDirectory, mdsDirectory, printName);

	mSchema = new BELPICSchema();
	mSchema->create();

	populate();
}

//
// Database-level ACLs
//
void BELPICToken::getOwner(AclOwnerPrototype &owner)
{
	// we don't really know (right now), so claim we're owned by PIN #0
	if (!mAclOwner) {
		mAclOwner.allocator(Allocator::standard());
		mAclOwner = AclFactory::PinSubject(Allocator::standard(), 0);
	}
	owner = mAclOwner;
}


void BELPICToken::getAcl(const char *tag, uint32 &count, AclEntryInfo *&acls)
{
	Allocator &alloc = Allocator::standard();
	// get pin list, then for each pin
	if (!mAclEntries)
	{
		mAclEntries.allocator(alloc);
        // Anyone can read the attributes and data of any record on this token
        // (it's further limited by the object itself).
		mAclEntries.add(CssmClient::AclFactory::AnySubject(
			mAclEntries.allocator()),
			AclAuthorizationSet(CSSM_ACL_AUTHORIZATION_DB_READ, 0));
        // We support PIN1 with either a passed in password
        // subject or a prompted password subject.
		mAclEntries.addPin(AclFactory::PWSubject(alloc), 1);
		mAclEntries.addPin(AclFactory::PromptPWSubject(alloc, CssmData()), 1);
	}
	count = mAclEntries.size();
	acls = mAclEntries.entries();
}


#pragma mark ---------------- BELPIC Specific --------------

void BELPICToken::populate()
{
	secdebug("populate", "BELPICToken::populate() begin");
	Tokend::Relation &certRelation =
		mSchema->findRelation(CSSM_DL_DB_RECORD_X509_CERTIFICATE);
	Tokend::Relation &privateKeyRelation =
		mSchema->findRelation(CSSM_DL_DB_RECORD_PRIVATE_KEY);
	Tokend::Relation &dataRelation =
		mSchema->findRelation(CSSM_DL_DB_RECORD_GENERIC);

	RefPointer<Tokend::Record> cert2(new BELPICBinaryFileRecord(kDF_BELPIC,
		kBELPIC_EF_Cert2, "Cert #2 (authentication)"));
	RefPointer<Tokend::Record> cert3(new BELPICBinaryFileRecord(kDF_BELPIC,
		kBELPIC_EF_Cert3, "Cert #3 (signature)"));
	RefPointer<Tokend::Record> cert4(new BELPICBinaryFileRecord(kDF_BELPIC,
		kBELPIC_EF_Cert4, "Cert #4 (CA)"));
	RefPointer<Tokend::Record> cert6(new BELPICBinaryFileRecord(kDF_BELPIC,
		kBELPIC_EF_Cert6, "Cert #6 (root)"));
	RefPointer<Tokend::Record> cert8(new BELPICBinaryFileRecord(kDF_BELPIC,
		kBELPIC_EF_Cert8, "Cert #8 (RN)"));

	certRelation.insertRecord(cert2);
	certRelation.insertRecord(cert3);
	certRelation.insertRecord(cert4);
	certRelation.insertRecord(cert6);
	certRelation.insertRecord(cert8);

	RefPointer<Tokend::Record> key2(new BELPICKeyRecord(kPrK2_Id,
		"PrK#2 (authentication)", privateKeyRelation.metaRecord(), true));
	RefPointer<Tokend::Record> key3(new BELPICKeyRecord(kPrK3_Id,
		"PrK#3 (signature)", privateKeyRelation.metaRecord(), true));

	privateKeyRelation.insertRecord(key2);
	privateKeyRelation.insertRecord(key3);

	key2->setAdornment(mSchema->publicKeyHashCoder().certificateKey(),
		new Tokend::LinkedRecordAdornment(cert2));
	key3->setAdornment(mSchema->publicKeyHashCoder().certificateKey(),
		new Tokend::LinkedRecordAdornment(cert3));

	dataRelation.insertRecord(new BELPICBinaryFileRecord(kDF_ID,
		kID_EF_ID_RN, "ID#RN"));
	dataRelation.insertRecord(new BELPICBinaryFileRecord(kDF_ID,
		kID_EF_SGN_RN, "SGN#RN"));
	dataRelation.insertRecord(new BELPICBinaryFileRecord(kDF_ID,
		kID_EF_ID_ADDRESS, "ID#Address"));
	dataRelation.insertRecord(new BELPICBinaryFileRecord(kDF_ID,
		kID_EF_SGN_ADDRESS, "SGN#Address"));
	dataRelation.insertRecord(new BELPICBinaryFileRecord(kDF_ID,
		kID_EF_ID_PHOTO, "ID#Photo"));
	dataRelation.insertRecord(new BELPICBinaryFileRecord(kDF_ID,
		kID_EF_PuK7_ID, "PuK#7 ID (CA role ID)"));
	dataRelation.insertRecord(new BELPICBinaryFileRecord(kDF_ID,
		kID_EF_Preferences, "Preferences"));

	secdebug("populate", "BELPICToken::populate() end");
}

/* arch-tag: 8A7C3BAF-124C-11D9-A606-000A9595DEEE */
