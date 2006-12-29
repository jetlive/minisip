/*
  Copyright (C) 2005, 2004 Erik Eliasson, Johan Bilien, Joachim Orrblad
  Copyright (C) 2006 Mikael Magnusson
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * Authors: Erik Eliasson <eliasson@it.kth.se>
 *          Johan Bilien <jobi@via.ecp.fr>
 *	    Joachim Orrblad <joachim@orrblad.com>
 *          Mikael Magnusson <mikma@users.sourceforge.net>
*/


#include<config.h>

#include<libmikey/MikeyException.h>
#include"MikeyMessageDHHMAC.h"
#include<libmikey/MikeyPayload.h>
#include<libmikey/MikeyPayloadERR.h>
#include<libmikey/MikeyPayloadDH.h>
#include<libmikey/MikeyPayloadHDR.h>
#include<libmikey/MikeyPayloadID.h>
#include<libmikey/MikeyPayloadKEMAC.h>
#include<libmikey/MikeyPayloadRAND.h>
#include<libmikey/MikeyPayloadT.h>

#include<libmcrypto/hmac.h>

#include<map>

using namespace std;

MikeyMessageDHHMAC::MikeyMessageDHHMAC(){
}

bool MikeyMessageDHHMAC::deriveTranspKeys( KeyAgreementDHHMAC* ka,
					   byte_t*& authKey, unsigned int& authKeyLength,
					   int encrAlg, int macAlg,
					   MikeyMessage* errorMessage ){
	bool error = false;

	authKeyLength = 0;
	authKey = NULL;

	switch( encrAlg ){
		case MIKEY_ENCR_NULL:
			break;
		default:
			error = true;
			if( errorMessage ){
				errorMessage->addPayload( 
					new MikeyPayloadERR( MIKEY_ERR_TYPE_INVALID_EA ) );
			}
			else{
				throw MikeyException( "Unknown encryption algorithm" );
			}
	}			

	switch( macAlg ){
		case MIKEY_MAC_HMAC_SHA1_160:
			authKeyLength = 20;
			authKey = new byte_t[ authKeyLength ];
			ka->genTranspAuthKey( authKey, authKeyLength );
			break;
		case MIKEY_MAC_NULL:
			break;
		default:
			error = true;
			if( errorMessage ){
				errorMessage->addPayload( 
					new MikeyPayloadERR( MIKEY_ERR_TYPE_INVALID_HA ) );
			}
			else{
				throw MikeyException( "Unknown MAC algorithm" );
			}
				
	}

	return !error;
}

MikeyMessageDHHMAC::MikeyMessageDHHMAC( KeyAgreementDHHMAC * ka,
					int macAlg){

	/* generate random a CryptoSessionBundle ID */
	unsigned int csbId = rand();
	ka->setCsbId( csbId );

	addPayload( 
		new MikeyPayloadHDR( HDR_DATA_TYPE_DHHMAC_INIT, 1, 
			HDR_PRF_MIKEY_1, csbId,
			ka->nCs(), ka->getCsIdMapType(), 
			ka->csIdMap() ) );

	addPayload( new MikeyPayloadT() );

	addPolicyToPayload( ka ); //Is in MikeyMessage.cxx

	MikeyPayloadRAND * payload;
	addPayload( payload = new MikeyPayloadRAND() );
	
	//keep a copy of the random value!
	ka->setRand( payload->randData(), 
		     payload->randLength() );

	// TODO add IDr

	addPayload( new MikeyPayloadDH( ka->group(),
					ka->publicKey(),
					ka->keyValidity() ) );

	// Add KEMAC
	byte_t* authKey = NULL;
	unsigned int authKeyLength = 0;
	int encrAlg = MIKEY_ENCR_NULL;
	deriveTranspKeys( ka, authKey, authKeyLength, encrAlg, macAlg, NULL );
	addKemacPayload( NULL, 0, NULL, NULL, authKey,
			 encrAlg, macAlg );
}
//-----------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------//

void MikeyMessageDHHMAC::setOffer( KeyAgreement * kaBase ){
	KeyAgreementDHHMAC* ka = dynamic_cast<KeyAgreementDHHMAC*>(kaBase);

	if( !ka ){
		throw MikeyExceptionMessageContent( 
				"Not a DHHMAC keyagreement" );
	}

	MikeyPayload * i = extractPayload( MIKEYPAYLOAD_HDR_PAYLOAD_TYPE );
	bool error = false;
	MikeyMessage * errorMessage = new MikeyMessage();

	if( i == NULL ){
		throw MikeyExceptionMessageContent(
				"DHHMAC init message had no HDR payload" );
	}

#define hdr ((MikeyPayloadHDR *)i)
	if( hdr->dataType() != HDR_DATA_TYPE_DHHMAC_INIT )
		throw MikeyExceptionMessageContent( 
				"Expected DHHMAC init message" );

	ka->setnCs( hdr->nCs() );
	ka->setCsbId( hdr->csbId() );
	

	if( hdr->csIdMapType() == HDR_CS_ID_MAP_TYPE_SRTP_ID || hdr->csIdMapType() == HDR_CS_ID_MAP_TYPE_IPSEC4_ID){ 
		ka->setCsIdMap( hdr->csIdMap() );
		ka->setCsIdMapType( hdr->csIdMapType() );
	}
	else{
		throw MikeyExceptionMessageContent( 
				"Unknown type of CS ID map" );
	}
	payloads.remove( i );
#undef hdr


	errorMessage->addPayload(
			new MikeyPayloadHDR( HDR_DATA_TYPE_ERROR, 0,
			HDR_PRF_MIKEY_1, ka->csbId(),
			ka->nCs(), ka->getCsIdMapType(), 
			ka->csIdMap() ) );
	
	i = extractPayload( MIKEYPAYLOAD_T_PAYLOAD_TYPE );

	if( i == NULL ){
		error = true;
		errorMessage->addPayload( 
			new MikeyPayloadERR( MIKEY_ERR_TYPE_UNSPEC ) );
	}

	if( ((MikeyPayloadT*)i)->checkOffset( MAX_TIME_OFFSET ) ){
		error = true;
		errorMessage->addPayload( 
			new MikeyPayloadERR( MIKEY_ERR_TYPE_INVALID_TS ) );
	}
	
	payloads.remove( i );

	addPolicyTo_ka(ka); //Is in MikeyMessage.cxx

	i = extractPayload( MIKEYPAYLOAD_RAND_PAYLOAD_TYPE );

	if( i == NULL ){
		error = true;
		errorMessage->addPayload( 
			new MikeyPayloadERR( MIKEY_ERR_TYPE_UNSPEC ) );
	}

	ka->setRand( ((MikeyPayloadRAND *)i)->randData(),
		     ((MikeyPayloadRAND *)i)->randLength() );

	payloads.remove( i );


	//FIXME treat the case of an ID payload
	i = extractPayload( MIKEYPAYLOAD_ID_PAYLOAD_TYPE );
	if( i != NULL ){
		payloads.remove( i );
	}

	i = extractPayload( MIKEYPAYLOAD_DH_PAYLOAD_TYPE );

	if( i == NULL ){
		error = true;
		errorMessage->addPayload( 
			new MikeyPayloadERR( MIKEY_ERR_TYPE_UNSPEC ) );
	}


	if( ka->group() != ((MikeyPayloadDH *)i)->group() ){
		ka->setGroup( ((MikeyPayloadDH *)i)->group() );
	}

	ka->setPeerKey( ((MikeyPayloadDH *)i)->dhKey(),
		        ((MikeyPayloadDH *)i)->dhKeyLength() );
	
	ka->setKeyValidity( ((MikeyPayloadDH *)i)->kv() );
	
	payloads.remove( i );

	i = extractPayload( MIKEYPAYLOAD_KEMAC_PAYLOAD_TYPE );

	if( i == NULL ){
		error = true;
		errorMessage->addPayload( 
			new MikeyPayloadERR( MIKEY_ERR_TYPE_UNSPEC ) );
	}	

	// KMAC
#define kemac ((MikeyPayloadKEMAC *)i)
	int encrAlg = kemac->encrAlg();
	int macAlg  = kemac->macAlg();

	byte_t * authKey = NULL;
	unsigned int authKeyLength = 0;

	error |= !deriveTranspKeys( ka, authKey, authKeyLength, encrAlg, macAlg, errorMessage );
	ka->setAuthKey( macAlg, authKey, authKeyLength);

	if( error ){
		if( authKey != NULL )
			delete [] authKey;
		throw MikeyExceptionMessageContent( errorMessage );
	}
#undef kemac
}
//-----------------------------------------------------------------------------------------------//
//
//-----------------------------------------------------------------------------------------------//

MikeyMessage * MikeyMessageDHHMAC::buildResponse( KeyAgreement * kaBase ){
	KeyAgreementDHHMAC* ka = dynamic_cast<KeyAgreementDHHMAC*>(kaBase);

	if( !ka ){
		throw MikeyExceptionMessageContent( 
				"Not a DHHMAC keyagreement" );
	}

	// Build the response message
	MikeyMessage * result = new MikeyMessage();
	result->addPayload( 
			new MikeyPayloadHDR( HDR_DATA_TYPE_DHHMAC_RESP, 0, 
			HDR_PRF_MIKEY_1, ka->csbId(),
			ka->nCs(), ka->getCsIdMapType(), 
			ka->csIdMap() ) );

	result->addPayload( new MikeyPayloadT() );

	// FIXME add IDi

	result->addPayload( new MikeyPayloadDH( 
				    ka->group(),
                                    ka->publicKey(),
				    ka->keyValidity() ) );

	result->addPayload( new MikeyPayloadDH( 
				    ka->group(),
                                    ka->peerKey(),
				    ka->keyValidity() ) );

	// KEMAC
	byte_t * authKey;
	unsigned int authKeyLength;
	int encrAlg = MIKEY_ENCR_NULL;

	deriveTranspKeys( ka, authKey, authKeyLength, encrAlg, ka->getMacAlg(), NULL );

	result->addKemacPayload( NULL, 0, NULL, NULL, authKey,
				 encrAlg, ka->getMacAlg() );

	if( authKey )
		delete[] authKey;

	return result;
}

MikeyMessage * MikeyMessageDHHMAC::parseResponse( KeyAgreement * kaBase ){
	KeyAgreementDHHMAC* ka = dynamic_cast<KeyAgreementDHHMAC*>(kaBase);

	if( !ka ){
		throw MikeyExceptionMessageContent( 
				"Not a DHHMAC keyagreement" );
	}

	MikeyPayload * i = extractPayload( MIKEYPAYLOAD_HDR_PAYLOAD_TYPE );
	bool error = false;
	bool gotDhi = false;
	MikeyMessage * errorMessage = new MikeyMessage();
	MRef<MikeyCsIdMap *> csIdMap;
	uint8_t nCs;
	
	if( i == NULL ){
		throw MikeyExceptionMessageContent(
				"DHHMAC resp message had no HDR payload" );
	}

#define hdr ((MikeyPayloadHDR *)(i))
	if( hdr->dataType() != HDR_DATA_TYPE_DHHMAC_RESP ){
		throw MikeyExceptionMessageContent( 
				"Expected DHHMAC resp message" );
	}

	if( hdr->csIdMapType() == HDR_CS_ID_MAP_TYPE_SRTP_ID || hdr->csIdMapType() == HDR_CS_ID_MAP_TYPE_IPSEC4_ID){
		csIdMap = hdr->csIdMap();
	}
	else{
		throw MikeyExceptionMessageContent( 
				"Unknown type of CS ID map" );
	}
	nCs = hdr->nCs();
#undef hdr
	ka->setCsIdMap( csIdMap );
	//FIXME look at the other fields!
	
	errorMessage->addPayload(
			new MikeyPayloadHDR( HDR_DATA_TYPE_ERROR, 0,
			HDR_PRF_MIKEY_1, ka->csbId(),
			nCs, ka->getCsIdMapType(),
			csIdMap ) );
	
	payloads.remove( i );
	i = extractPayload( MIKEYPAYLOAD_T_PAYLOAD_TYPE );

	if( i == NULL ){
		error = true;
		errorMessage->addPayload( 
			new MikeyPayloadERR( MIKEY_ERR_TYPE_UNSPEC ) );
	}

	if( ((MikeyPayloadT*)i)->checkOffset( MAX_TIME_OFFSET ) ){
		error = true;
		errorMessage->addPayload( 
			new MikeyPayloadERR( MIKEY_ERR_TYPE_INVALID_TS ) );
	}

	payloads.remove( i );

	addPolicyTo_ka(ka); //Is in MikeyMessage.cxx

	i = extractPayload( MIKEYPAYLOAD_ID_PAYLOAD_TYPE );
	if( i != NULL ){
		payloads.remove( i );
	}

	i = extractPayload( MIKEYPAYLOAD_DH_PAYLOAD_TYPE );
	
	if( i == NULL ){
		error = true;
		errorMessage->addPayload(
			new MikeyPayloadERR( MIKEY_ERR_TYPE_UNSPEC ) );
	}

#define dh ((MikeyPayloadDH *)i)
	if( string( (const char *)dh->dhKey(), 
				  dh->dhKeyLength() ) ==
	    string( (const char *)ka->publicKey(), 
		    		  ka->publicKeyLength() ) ){
		// This is the DHi payload
		gotDhi = true;
	}
	else{
		// This is the DHr payload
		ka->setPeerKey( dh->dhKey(),
				dh->dhKeyLength() );
	}

	payloads.remove( i );
	i = extractPayload( MIKEYPAYLOAD_DH_PAYLOAD_TYPE );

	if( i == NULL ){
		error = true;
		errorMessage->addPayload(
			new MikeyPayloadERR( MIKEY_ERR_TYPE_UNSPEC ) );
	}

	if( gotDhi ){
		// this one should then be DHr
		ka->setPeerKey( dh->dhKey(), dh->dhKeyLength() );
	}
	else{
		if( string( (const char *)dh->dhKey(), 
					  dh->dhKeyLength() ) !=
	    	    string( (const char *)ka->publicKey(), 
		    	    ka->publicKeyLength() ) ){
			error = true;
			errorMessage->addPayload(
				new MikeyPayloadERR( MIKEY_ERR_TYPE_UNSPEC ) );
		}
	}
#undef dh

	if( error ){
		throw MikeyExceptionMessageContent( errorMessage );
	}

	delete errorMessage;

	return NULL;
}

bool MikeyMessageDHHMAC::authenticate( KeyAgreement * kaBase ){
	KeyAgreementDHHMAC* ka = dynamic_cast<KeyAgreementDHHMAC*>(kaBase);

	if( !ka ){
		throw MikeyExceptionMessageContent( 
				"Not a DHHMAC keyagreement" );
	}

	MikeyPayload * payload = *(lastPayload());
	int i;
	int macAlg;
	byte_t * receivedMac;
	byte_t * macInput;
	unsigned int macInputLength;
	list<MikeyPayload *>::iterator payload_i;
 
	if( ka->rand() == NULL ){
		
		MikeyPayloadRAND * randPayload;
		
		randPayload = (MikeyPayloadRAND*) extractPayload(MIKEYPAYLOAD_RAND_PAYLOAD_TYPE );
		
		if( randPayload == NULL ){
			ka->setAuthError(
				"The MIKEY init has no"
				"RAND payload."
			);
			
			return true;
		}

		ka->setRand( randPayload->randData(), 
			     randPayload->randLength() );
	}

	
	if( type() == HDR_DATA_TYPE_DHHMAC_INIT ||
	    type() == HDR_DATA_TYPE_DHHMAC_RESP ){
		MikeyPayloadKEMAC * kemac;
		if( payload->payloadType() != MIKEYPAYLOAD_KEMAC_PAYLOAD_TYPE){
			cerr << "Last payload type = " << (int)payload->payloadType() << endl;
			throw MikeyException( 
			   "DHHMAC init did not end with a KEMAC payload" );
		}
		

		kemac = (MikeyPayloadKEMAC *)payload;
		macAlg = kemac->macAlg();
		receivedMac = kemac->macData();
		macInput = rawMessageData();
		macInputLength = rawMessageLength() - 20;
		ka->setCsbId( csbId() );
	}
	else{
		throw MikeyException( "Invalide type for a DHHMAC message" );
	}

	byte_t authKey[20];
	byte_t computedMac[20];
	unsigned int computedMacLength;
	
	switch( macAlg ){
		case MIKEY_MAC_HMAC_SHA1_160:
			ka->genTranspAuthKey( authKey, 20 );

			hmac_sha1( authKey, 20,
				   macInput,
				   macInputLength,
				   computedMac, &computedMacLength );

			for( i = 0; i < 20; i++ ){
				if( computedMac[i] != receivedMac[i] ){
					ka->setAuthError(
						"MAC mismatch: the shared"
						"key probably differs."
					);
					return true;
				}
			}
			return false;
		case MIKEY_MAC_NULL:
			return false;
		default:
			throw MikeyException( "Unknown MAC algorithm" );
	}

}

bool MikeyMessageDHHMAC::isInitiatorMessage() const{
	return type() == MIKEY_TYPE_DHHMAC_INIT;
}

bool MikeyMessageDHHMAC::isResponderMessage() const{
	return type() == MIKEY_TYPE_DHHMAC_RESP;
}

int32_t MikeyMessageDHHMAC::keyAgreementType() const{
	return KEY_AGREEMENT_TYPE_DHHMAC;
}