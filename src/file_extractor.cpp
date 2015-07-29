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

#include "file_extractor.hpp"
#include "lzsdecoder.hpp"

#include <algorithm>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;

/**
* Extract a field (.msd) file from sub-index in an iterative way.
* @param result Reference where the extracted file data will be saved.
* @param hint Reference where a hint to help naming the extracted file will be stored.
* @return True if the there's a next file, false if we reached the last one.
*/
bool FileExtractor::extractNextFieldFile (filedata_type &result, string &hint)
{
   for (; m_last != m_subRecs.end(); m_last++)
   {
      if (m_last->isInvalid()) continue;

      boost::shared_array<u8> buffer(new u8[m_last->length]);
      u8 *bufferPtr = buffer.get();
               
      m_img.seekg(m_last->offset);
      m_img.read((char *)bufferPtr, m_last->length);

      u32 *lzLen = (u32 *)bufferPtr;

      // unknown files
      if (*lzLen == 0x00000100 || *lzLen == 0x10000100 || *lzLen == 0x20000100) continue;
      // possibly .map files
      if (*lzLen == 0x00000800) continue;
      // dummy text files
      if (m_last->length == 33) continue;
      // decompressed files
      if (*lzLen != m_last->length - 4) continue;

      // now the ones left should be proper lzs files
      LZSDecoder decoder(bufferPtr + 4, *lzLen);
            
      LZSDecoder::filedata_type decBuffer = decoder.decode();
      const u8 *decBufferPtr = decBuffer.first.get();

      // get rid of .mim files
      if (*((u32 *)decBufferPtr) != 0x800e1030) continue;

      const u8 *endPos = find(decBufferPtr + 0x30, decBufferPtr + 0x40, 0x00);
      string name;

      for (const u8 *i = decBufferPtr + 0x30; i < endPos; i++)
         name += *i;

      // if we got this far, it should be a valid field dialog lzs-file.
      result = make_pair(buffer, m_last->length);
      hint = name;

      // stores info that the caller can query
      m_lastId = distance(m_subRecs.begin(), m_last);
      m_lastEntry = make_pair(m_last->offset, m_last->length);

      m_last++;
      return true;
   }

   // no more field files
   return false;
}

/**
* Extract a battle (.dat) file from sub-index in an iterative way.
* @param result Reference where the extracted file data will be saved.
* @param hint Reference where a hint to help naming the extracted file will be stored.
* @return True if the there's a next file, false if we reached the last one.
*/
bool FileExtractor::extractNextBattleFile (filedata_type &result, string &hint)
{
   for (; m_last != m_subRecs.end(); m_last++)
   {
      if (m_last->isInvalid()) continue;

      boost::shared_array<u8> buffer(new u8[m_last->length]);
      u8 *bufferPtr = buffer.get();
               
      m_img.seekg(m_last->offset);
      m_img.read((char *)bufferPtr, m_last->length);

      // field battle files without any text
      // TODO Extract files without text too, to create the bestiary.xml
      u32 *num_sections = (u32 *)bufferPtr;
      if (*num_sections != 0x0000000b) continue;

      u32 *infoPtr = (u32 *)bufferPtr + 7;
      u8 *nameIdPtr = bufferPtr + *infoPtr;

      int sz = count_if(nameIdPtr, nameIdPtr + 24, bind2nd(not_equal_to<u8>(), 0x00));
      string name;

      // extract a nice name
      for (int i=0; i < sz; i++)
      {
         if (nameIdPtr[i] == 0x03 || nameIdPtr[i] == 0x0c)
            name += tbl.find<u16>((nameIdPtr[i] << 8) | (nameIdPtr[i + 1] & 0xff)), i++;
         else
            name += tbl.find<u8>(nameIdPtr[i]);
      }

      // make it looks even nicer
      boost::to_lower(name);
      boost::trim(name);

      vector<IndexEntry>::difference_type n = distance(m_subRecs.begin(), m_last);

      boost::format fmt("%1$04d%2%");
      fmt % n % (name.empty() ? "" : "-" + name);

      result = make_pair(buffer, m_last->length);
      hint = fmt.str();

      // stores info that the caller can query
      m_lastId = n;
      m_lastEntry = make_pair(m_last->offset, m_last->length);

      m_last++;
      return true;
   }

   return false;
}