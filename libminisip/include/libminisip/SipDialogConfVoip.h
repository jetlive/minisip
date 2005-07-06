/*
  Copyright (C) 2005, 2004 Erik Eliasson, Johan Bilien
  
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


/* Copyright (C) 2004 
 *
 * Authors: Erik Eliasson <eliasson@it.kth.se>
 *          Johan Bilien <jobi@via.ecp.fr>
 *	    Joachim Orrblad <joachim@orrblad.com>
*/

/* Name
 * 	SipDialogConfVoip.h
 * Author
 * 	Erik Eliasson, eliasson@it.kth.se
 * Purpose
 * 
*/



#ifndef SipDialogConfVoip_H
#define SipDialogConfVoip_H


#include<config.h>


#include<libmsip/SipDialog.h>
#include<libmsip/SipTransaction.h>
#include<libmsip/SipInvite.h>
#include<libmsip/SipAck.h>
#include<libmsip/SipBye.h>
#include<libmsip/SipResponse.h>
#include<libmutil/StateMachine.h>

#include<libmutil/minilist.h>

#include"SipSoftPhoneConfiguration.h"

#ifdef IPSEC_SUPPORT
#include<../ipsec/MsipIpsecAPI.h>
#endif

class Session;
class SipDialogContainer;
class SipDialogConfig;
class LogEntry;

class SipDialogConfVoip: public SipDialog{
	public:
#ifdef IPSEC_SUPPORT
		SipDialogConfVoip(MRef<SipStack*> stack, MRef<SipDialogConfig*> callconfig, MRef<SipSoftPhoneConfiguration*> phoneconf, MRef<Session *> mediaSession, minilist<ConfMember> *list, string confid, string callId="", MRef<MsipIpsecAPI *> ipsecSession=NULL);
#else
		SipDialogConfVoip(MRef<SipStack*> stack, MRef<SipDialogConfig*> callconfig, MRef<SipSoftPhoneConfiguration*> phoneconf, MRef<Session *> mediaSession, minilist<ConfMember> *list, string confid, string callId="");
#endif
#ifdef IPSEC_SUPPORT
		SipDialogConfVoip(MRef<SipStack*> stack, MRef<SipDialogConfig*> callconfig, MRef<SipSoftPhoneConfiguration*> phoneconf, MRef<Session *> mediaSession, string confid, string callId="", MRef<MsipIpsecAPI *> ipsecSession=NULL);
#else
		SipDialogConfVoip(MRef<SipStack*> stack, MRef<SipDialogConfig*> callconfig, MRef<SipSoftPhoneConfiguration*> phoneconf, MRef<Session *> mediaSession, string confid, string callId="");


#endif		
		virtual ~SipDialogConfVoip();

		virtual std::string getMemObjectType(){return "SipDialogConfVoip";}
		
		virtual string getName(){return "SipDialogConfVoip[callid="+dialogState.callId +"]";}


		virtual bool handleCommand(const SipSMCommand &command);

	
		MRef<Session *> getMediaSession();
#ifdef IPSEC_SUPPORT
		MRef<MsipIpsecAPI *> getIpsecSession();
#endif
		void registerSDP(uint32_t sourceId, MRef<SdpPacket*> sdppack);

		//void handleSdp(MRef<SdpPacket*> );
                
		MRef<LogEntry *> getLogEntry();
		void setLogEntry( MRef<LogEntry *> );
	private:
		string confId;
		void setUpStateMachine();
		
		//string connectedList[10]; //old static list
		minilist<ConfMember> *adviceList;
		
		//this is the list that will be sent in an Ok or Ack message.
		minilist<ConfMember> connectedList;
		
		string type;
		int numConnected;
		MRef<SipInvite*> getLastInvite();
		void setLastInvite(MRef<SipInvite*> i);
		
		void sendInvite(const string &branch);
		void sendAuthInvite(const string &branch);
		void sendBye(const string &branch, int);
		void sendCancel(const string &branch);
		void sendAck(const string &branch);
		void sendInviteOk(const string &branch);
		void sendByeOk(MRef<SipBye*> bye, const string &branch);
		void sendReject(const string &branch);
		void sendRinging(const string &branch);
		void sendNotAcceptable(const string &branch);
		void modifyConfJoinInvite(MRef<SipInvite*>inv);
		void modifyConfConnectInvite(MRef<SipInvite*>inv);
		void modifyConfAck(MRef<SipAck*>ack);
		void modifyConfOk(MRef<SipResponse*> ok);
		bool a0_start_callingnoauth_invite( const SipSMCommand &command);
		bool a1_callingnoauth_callingnoauth_18X( const SipSMCommand &command);
		bool a2_callingnoauth_callingnoauth_1xx( const SipSMCommand &command);
		bool a3_callingnoauth_incall_2xx( const SipSMCommand &command);
		bool a5_incall_termwait_BYE( const SipSMCommand &command);
		bool a6_incall_termwait_hangup( const SipSMCommand &command);
		bool a7_callingnoauth_termwait_CANCEL(const SipSMCommand &command);
		bool a8_callingnoauth_termwait_cancel(const SipSMCommand &command);
		bool a9_callingnoauth_termwait_36( const SipSMCommand &command);
		bool a10_start_ringing_INVITE( const SipSMCommand &command);
		bool a11_ringing_incall_accept( const SipSMCommand &command);
		bool a12_ringing_termwait_CANCEL( const SipSMCommand &command);
		bool a13_ringing_termwait_reject( const SipSMCommand &command);
		bool a16_start_termwait_INVITE( const SipSMCommand &command);
		bool a20_callingnoauth_callingauth_40X( const SipSMCommand &command);
		bool a21_callingauth_callingauth_18X( const SipSMCommand &command);
		bool a22_callingauth_callingauth_1xx( const SipSMCommand &command);
		bool a23_callingauth_incall_2xx( const SipSMCommand &command);
		bool a24_calling_termwait_2xx(const SipSMCommand &command);

		bool a25_termwait_terminated_notransactions( const SipSMCommand &command);
		bool a26_callingnoauth_termwait_transporterror( const SipSMCommand &command);

		bool a26_callingauth_termwait_cancel( const SipSMCommand &command);
		bool a27_incall_incall_ACK( const SipSMCommand &command);
		
		bool sortMIME(MRef<SipMessageContent *> Offer, string peerUri, int type);		
		
		
		
		MRef<LogEntry *> logEntry;

		MRef<SipInvite*> lastInvite;
		MRef<SipResponse*> lastResponse;
		bool localCalled;
		string nonce;
		string realm;
		MRef<SipSoftPhoneConfiguration*> phoneconf;
		MRef<Session *> mediaSession;

#ifdef IPSEC_SUPPORT
		MRef<MsipIpsecAPI *> ipsecSession;
		string list;
		
#endif

};

#endif
