/*
  Copyright (C) 2007 Erik Eliasson

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

 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

/**
 * @author Erik Eliasson <eliasson@it.kth.se>
 *	    Werner Dittmann <Werner.Dittmann@t-online.de>
 */

#include <config.h>
#include <libmcrypto/sha1.h>
#include <gcrypt.h>

#ifndef SHA1_DIGEST_LENGTH
# define SHA1_DIGEST_LENGTH 20
#endif

void sha1(uint8_t* data, uint32_t dataLength,
            uint8_t* mac)
{
    gcry_md_hd_t hd;
    gcry_error_t err = 0;

    err = gcry_md_open(&hd, GCRY_MD_SHA1, 0);
    gcry_md_write (hd, data, dataLength);

    uint8_t* p = gcry_md_read (hd, GCRY_MD_SHA1);
    memcpy(mac, p, SHA1_DIGEST_LENGTH);
    gcry_md_close (hd);
}

void sha1(uint8_t* dataChunks[],
            uint32_t dataChunkLength[],
            uint8_t* mac)
{
    gcry_md_hd_t hd;
    gcry_error_t err = 0;

    err = gcry_md_open(&hd, GCRY_MD_SHA1, 0);
    while (*dataChunks) {
        gcry_md_write (hd, *dataChunks, (uint32_t)(*dataChunkLength));
        dataChunks++;
        dataChunkLength++;
    }
    uint8_t* p = gcry_md_read (hd, GCRY_MD_SHA1);
    memcpy(mac, p, SHA1_DIGEST_LENGTH);
    gcry_md_close (hd);
}
