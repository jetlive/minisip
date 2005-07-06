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


/* Copyright (C) 2004, 2005
 *
 * Authors: Erik Eliasson <eliasson@it.kth.se>
 *          Johan Bilien <jobi@via.ecp.fr>
*/

#include<config.h>
#include<libminisip/DtmfSender.h>
#include<libminisip/Session.h>
#include<libminisip/MediaStream.h>


DtmfSender::DtmfSender( MRef<Session *> session ){
        this->session = session;
}

void DtmfSender::timeout( DtmfEvent * event ){
        uint8_t payload[4];
        payload[0] = event->symbol;
        payload[1] = ( (event->endOfEvent << 7) & 0x80 ) | ( event->volume & 0x3F );
        payload[2] = (event->duration >> 8) & 0xFF;
        payload[3] = event->duration & 0xFF;

        sendPayload( payload, event->startOfEvent, event->ts );

        if( event->lastBlock ){
                delete event->ts;
        }

        delete event;
}

void DtmfSender::sendPayload( byte_t payload[], bool mark, uint32_t * ts ){
        std::list<MRef<MediaStream *> >::iterator iSender;

        session->mediaStreamSendersLock.lock();
        for( iSender =  session->mediaStreamSenders.begin();
             iSender != session->mediaStreamSenders.end();
             iSender++ ){

                if( !(**iSender)->disabled ){
                        ((MediaStreamSender *)(**iSender))->send( payload, 4, ts, mark, true );
                }

        }
        session->mediaStreamSendersLock.unlock();

}
