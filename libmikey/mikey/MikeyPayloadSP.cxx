/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Copyright (C) 2004 
 *
 * Authors: Erik Eliasson <eliasson@it.kth.se>
 *          Johan Bilien <jobi@via.ecp.fr>
 *	    Joachim Orrblad <joachim@orrblad.com>
*/

#include<config.h>
#include<libmikey/MikeyPayloadSP.h>
#include<libmikey/MikeyException.h>


//
// MikeyPolicyParam
//
MikeyPolicyParam::MikeyPolicyParam( uint8_t type, uint8_t length, byte_t * value ):
	type(type), length(length), value(value){};

//
// MikeyPayloadSP
//
//Constructor when receiving Mikey message i.e. contruct MikeyPayloadSP from bytestream
MikeyPayloadSP::MikeyPayloadSP(byte_t *start, int lengthLimit):MikeyPayload(start){
	this->payloadTypeValue = MIKEYPAYLOAD_SP_PAYLOAD_TYPE;
	this->policy_param_length = 0;
	this->nextPayloadTypeValue = start[0]; 
	this->policy_no = start[1];
	this->prot_type = start[2];
	int i= 5,j;
	byte_t *value;
	while(i < lengthLimit) {
		this->addMikeyPolicyParam(start[i], start[i+1], &start[i+2]);
		i = i + 2 + start[i+1];
	}
}
//Constructor when constructing new Mikey message, policy type entries added later with MikeyPayloadSP::addMikeyPolicyParam
MikeyPayloadSP::MikeyPayloadSP(uint8_t policy_no, uint8_t prot_type){
	this->payloadTypeValue = MIKEYPAYLOAD_SP_PAYLOAD_TYPE;
	this->policy_param_length = 0;
	this->policy_no = policy_no;
	this->prot_type = prot_type;
}
//Constructing new Mikey message with default policy for some protocols
MikeyPayloadSP::MikeyPayloadSP(uint8_t policy_no, uint8_t prot_type, bool def){
	this->payloadTypeValue = MIKEYPAYLOAD_SP_PAYLOAD_TYPE;
	this->policy_param_length = 0;
	this->policy_no = policy_no;
	this->prot_type = prot_type;
	int arraysize,i;
	char * values;
	//Default polict values for some protocol
	byte_t ipsec4values[] = {MIKEY_IPSEC_SATYPE_ESP,MIKEY_IPSEC_MODE_TRANSPORT,MIKEY_IPSEC_SAFLAG_PSEQ,MIKEY_IPSEC_EALG_3DESCBC,24,MIKEY_IPSEC_AALG_SHA1HMAC,16};
	byte_t srtpvalues[] ={MIKEY_SRTP_EALG_AESCM,16,MIKEY_SRTP_AALG_SHA1HMAC,94,14,MIKEY_SRTP_PRF_AESCM,0,1,1,MIKEY_FEC_ORDER_FEC_SRTP,1,4,0};
	switch (prot_type) {
	case MIKEY_PROTO_SRTP:
		arraysize = 13;
		for(i=0; i< arraysize; i++)
			this->addMikeyPolicyParam(i, 1, &srtpvalues[i] );
		break;
	case MIKEY_PROTO_IPSEC4:
		arraysize = 7;
		for(i=0; i< arraysize; i++)
			this->addMikeyPolicyParam(i, 1, &ipsec4values[i] );	
		break;
	default:
		throw new MikeyException( "Not supported prot_type" );
	}
}	
//Destructor
MikeyPayloadSP::~MikeyPayloadSP(){
	list<MikeyPolicyParam *>::iterator i;
	for( i = param.begin(); i != param.end() ; i++ ){
		free((*i)->value);
		delete *i;
	}
}
//Add a policytype i.e. add one MikeyPolicyParam in list<MikeyPolicyParam *> param
void MikeyPayloadSP::addMikeyPolicyParam( uint8_t type, uint8_t length, byte_t * value){
	if(this->getParameterType(type) != NULL)
		this->deleteMikeyPolicyParam(type);
	byte_t *val  = (byte_t*) calloc (length,sizeof(byte_t));
	for(int i=0; i< length; i++)
			val[i] = value[i];
	param.push_back (new MikeyPolicyParam(type, length, val));
	this->policy_param_length = this->policy_param_length + length + 2;
}
//Get the MikeyPolicyParam in list<MikeyPolicyParam *> param with type type
MikeyPolicyParam * MikeyPayloadSP::getParameterType(uint8_t type){
	list<MikeyPolicyParam *>::iterator i;
	for( i = param.begin(); i != param.end()  ; i++ )
		if( (*i)->type == type )
			return *i;
	return NULL;
}
//Generate bytestream
void MikeyPayloadSP::writeData(byte_t *start, int expectedLength){
	assert( expectedLength == this->length() );
	start[0] = this->nextPayloadTypeValue;
	start[1] = this->policy_no;
	start[2] = this->prot_type;
	start[3] = (byte_t) ((this->policy_param_length & 0xFF00) >> 8);
	start[4] = (byte_t) (this->policy_param_length & 0xFF);
	//Add policy params
	list<MikeyPolicyParam *>::iterator i = param.begin();
	int j=5,k;
	while (i != param.end() && j < expectedLength){
		start[j++] = (*i)->type;
		start[j++] = (*i)->length;
		for(k=0; k < ((*i)->length); k++)
			start[j++] = ((*i)->value)[k];
		i++;
	}
}
//Return total length of the MikeyPayloadSP data in bytes
int MikeyPayloadSP::length(){
	return 5 + this->policy_param_length;
}
//Return number of policy param entries
int MikeyPayloadSP::noOfPolicyParam(){
	return param.size();
}
//Delete the MikeyPolicyParam in list<MikeyPolicyParam *> param with type type
void MikeyPayloadSP::deleteMikeyPolicyParam(uint8_t type){
	list<MikeyPolicyParam *>::iterator i;
	for( i = param.begin(); i != param.end()  ; i++ )
		if( (*i)->type == type )
			param.erase(i);
}
