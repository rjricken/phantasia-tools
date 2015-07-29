/*
 * Phantasia - Final Fantasy VIII Romhacking Tools
 * Copyright (C) 2005 Ricardo J. Ricken (Darkl0rd)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef LZSDECODER_HPP
#define LZSDECODER_HPP

#include <iostream>
#include <iomanip>
#include <utility>
#include <exception>
#include <boost/shared_array.hpp>
#include "common.hpp"

using namespace std;

/**
* This class is capable of decode a LZS data stream encoded in
* the schema used in the PSX game Final Fantasy VIII.
*/
class LZSDecoder
{
public:
   /** Used to represent a binary data block */
   typedef std::pair<boost::shared_array<u8>, u32> filedata_type;

   /**
   * Initializes a new instance with the necessary data for the decoding process.
   * @param data Pointer to lzs data stream.
   * @param len Lzs data stream length.
   */
   LZSDecoder (const u8 *data, int len) :
      lzsData(data), lzsDataLen(len) { }

   /**
   * Decodes the given lzs data stream into an internal buffer.
   * @return True on success, false otherwise.
   */
   filedata_type decode ()
   {
      u8 *decBuffer = new u8[lzsDataLen * 9];
      int lzsPos = 0, decPos = 0;

      while (lzsPos < lzsDataLen)
      {
         const u8 &controlByte = lzsData[lzsPos++];

         for (int i=0; i < 8; i++)
         {
            if (controlByte & (0x1 << i))
               decBuffer[decPos++] = lzsData[lzsPos++];
            else
            {
               // extract info from the OOOOOOOO OOOOLLLL lzpair
               u16 pair = lzsData[lzsPos] << 8 | lzsData[lzsPos + 1] & 0xff;
               int rawOffset = ((pair & 0xff00) >> 8) | ((pair & 0xf0) << 4);
               int length = (pair & 0x000f) + 3;

/*               cout << "rawOffset: " << hex << setfill('0') << setw(3) << rawOffset
                    << " decPos: " << dec << setfill(' ') << decPos
                    << " length: " << dec << length << endl;*/
                              
               // offset inside the decoded data buffer
               int realOffset = decPos + 1 - ((decPos + 1 - 18 - rawOffset) & 0x0fff);
               lzsPos += 2;

//               cout << "realOffset: " << dec << realOffset << endl << endl;

               while (length-- > 0)
               {
                  // handles the special case for repetitions
                  if (realOffset < 0 || realOffset > decPos)
                  {
                     decBuffer[decPos++] = 0x00;
                     realOffset++;
                  }
                  else decBuffer[decPos++] = decBuffer[realOffset++];
               }
            }

            // prevents the lzs data stream to be overrun while
            // the control byte is being processed.
            if (lzsPos >= lzsDataLen) break;
         }
      }

      return std::make_pair(boost::shared_array<u8>(decBuffer), decPos);
   }

private:
   const u8 *lzsData;  /**< Buffer holding the lzs data stream */
   int lzsDataLen;     /**< Lzs data stream length */
};

#endif //~LZDECODER_HPP