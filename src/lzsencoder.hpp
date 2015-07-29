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

#ifndef LZSENCODER_HPP
#define LZSENCODER_HPP

#include "common.hpp"
#include <map>
#include <utility>
#include <boost/shared_array.hpp>


#include <iostream>

class LZSEncoder
{
public:
   typedef std::basic_string<u8, std::char_traits<u8>> matchstr_type;  /**< Used to represent an LZSS match in the dictionary */
   typedef std::pair<boost::shared_array<u8>, u32> filedata_type;      /**< Used to represent a binary data block */

   LZSEncoder (const filedata_type &data) :
      m_data(data) {
      preprocess();
   }

   /**
   * Encodes the given byte stream into an internal buffer.
   * @return True on success, false otherwise.
   */
   filedata_type encode ()
   {
      boost::shared_array<u8> buffer(new u8[m_data.second]);

      u8 *bufferPtr = buffer.get();
      u8 *dataPtr = m_data.first.get();

      int dataPos = 0, bufferTail = 0;
      const u32 &dataLen = m_data.second;

      while (dataPos < dataLen)
      {
         bufferPtr[bufferTail] = 0xFF;
         u8 &controlByte = bufferPtr[bufferTail++];

         // find values to each of the 8-bits in the control byte
         for (int i = 0; i < 8 && dataPos < dataLen; i++)
         {
            // compute sliding window range
            int winRange = dataPos > LzWinSize ? dataPos - LzWinSize : 0;
            LZMatch bestMatch;
            
            // finds the best possible match for this position
            for (int j = LzMaxSize; j >= LzMinSize; j--)
            {
               matchstr_type cur(dataPtr + dataPos, dataPtr + dataPos + j);
               std::pair<refiter_type, refiter_type> result = m_refs.equal_range(cur);

               for (refiter_type k = result.first; k != result.second; ++k)
                  if (/*bestMatch.size < k->second.size
                     && */((k->second.pos >= winRange && winRange + k->second.pos + k->second.size <= dataPos) || (k->second.pos < 0)))
                     bestMatch = k->second;

               // the first match found comprises automatically the best one in terms of size
               if (result.first != result.second && !bestMatch.empty()) break;
            }

            if (bestMatch.empty())
               bufferPtr[bufferTail++] = dataPtr[dataPos++];
            else
            {
               u16 *pair = (u16 *)&bufferPtr[bufferTail];
               u16 offset = (bestMatch.pos - LzMaxSize) & LzWinSize;

               *pair = (bestMatch.size - LzMinSize) & 0xf;
               *pair |= ((offset & 0xff) << 8) | (offset & 0xf00) >> 4;
               *pair = (*pair >> 8) | ((*pair << 8) & 0xff00);

               bufferTail += 2;
               dataPos += bestMatch.size;
               controlByte ^= 0x01 << i;

               bestMatch.reset();
            }
         }
      }

      return std::make_pair(buffer, bufferTail);
   }

private:
   typedef struct tagLZMatch {
      tagLZMatch (int mSize = 0, int mPos = 0) :
         size(mSize), pos(mPos) { }

      void reset () {
         size = pos = 0;
      }

      bool empty () {
         return size == 0 && pos == 0;
      }
      
      int size;
      int pos;
   } LZMatch;

   enum {
      LzMinSize = 0x03,  /**< matches should be at least 3-bytes long         */
      LzMaxSize = 0x12,  /**< matches can be up to 18-bytes long              */
      LzWinSize = 0x0fff /**< sliding window comprised of a 4096-bytes buffer */
   };

   void preprocess ()
   {
      const u8 *dataPtr = m_data.first.get();

      for (long dataPos = 0; dataPos < m_data.second; dataPos++)
         for (int i = LzMinSize - 1; i < LzMaxSize && dataPos - i >= 0; i++)
            m_refs.insert(std::make_pair(matchstr_type(dataPtr + dataPos - i, i + 1), LZMatch(i + 1, dataPos - i)));
      
      for (int i = LzMinSize; i <= LzMaxSize; i++)
         m_refs.insert(std::make_pair(matchstr_type(i, 0x00), LZMatch(i, -i)));
   }

   typedef std::multimap<matchstr_type, LZMatch> lzref_type;
   typedef lzref_type::iterator refiter_type;

   lzref_type m_refs;             /**< mapping beetwen all possible matches and its position and size */
   const filedata_type &m_data;   /**< binary data to be compressed */
};

#endif //~LZSENCODER_HPP