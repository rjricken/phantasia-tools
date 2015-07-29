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

#ifndef FILEEXTRACTOR_HPP
#define FILEEXTRACTOR_HPP

#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <exception>
#include <boost/shared_array.hpp>
#include "common.hpp"
#include "extractinfo.hpp"
#include "dictionary.hpp"

/**
* The purpose of this class is to extract data from an IMG file
* based on information described inside a XML file.
*/
class FileExtractor
{
public:
   FileExtractor (FF8ExtractInfo &info, std::string battleDic) : m_info(info)
   {
      m_img.open(m_info.imgName(), std::ios::binary);

      std::string error = "Failed to open " + m_info.imgName() + " file.";
      if (!m_img) throw std::exception(error.c_str());

      tbl.loadFromFile(battleDic);
   }

   typedef std::pair<boost::shared_array<u8>, u32> filedata_type;
   typedef std::pair<u32, u32> fileinfo_type;

   enum {
      Battle,  /**< Field Battle files (.dat) */
      Field    /**< Field Dialog files (.msd) */
   };
      
   /**
   * Loads the main index found in the IMG file.
   * This is a prerequesite to use the extractFile method.
   * @return End offset of the index.
   */
   u32 loadMainIndex ()
   {
      m_img.seekg(m_info.indexOffset());

      for (u16 i=0; ; i++)
      {
         IndexEntry temp;
         m_img.read((char *)&temp, sizeof(temp));

         if (temp.isInvalid())
            break;

         temp.offset -= m_info.indexSector();
         temp.offset *= m_info.secSize();

         m_records.push_back(temp);
      }

      return static_cast<u32>(m_img.tellg()) - sizeof(IndexEntry);
   }

   /**
   * Extract the file corresponding to the specified id.
   * @param id The id of the file to be extracted.
   * @return A pair containing a pointer to the data and its length.
   */
   filedata_type extractFile (u16 id)
   {
      if (m_records.empty())
         throw std::exception("FileExtractor::loadMainIndex should be called before extracting any files.");

      m_img.seekg(m_records[id].offset);
      
      boost::shared_array<u8> data(new u8[m_records[id].length]);
      m_img.read((char *)data.get(), m_records[id].length);

      // updates information about the file
      m_info[id].offset(m_records[id].offset), m_info[id].length(m_records[id].length);

      return std::make_pair(data, m_records[id].length);
   }

   /**
   * Loads a sub-index from the specified file.
   * This is a prerequisite to extract field and battle files.
   * @param id The id of the file to be extracted.
   * @param data Pair with the data and its length.
   */
   void loadSubIndex (u16 id, filedata_type data)
   {
      // clear previous entries
      if (!m_subRecs.empty()) m_subRecs.clear();

      FF8SubIndexInfo subIdx = m_info[id].subIndex();
      u8 *dataPtr = data.first.get();

      IndexEntry *startPtr = (IndexEntry *)(dataPtr + subIdx.m_startOff);
      IndexEntry *endPtr = (IndexEntry *)(dataPtr + subIdx.m_endOff);

      for (IndexEntry *i = startPtr; i != endPtr; ++i)
      {
         IndexEntry temp((i->offset - m_info.indexSector()) * m_info.secSize(), i->length);
         m_subRecs.push_back(temp);
      }

      m_last = m_subRecs.begin();
   }

   /**
   * Extract files from sub-index in an iterative way.
   * Invalid files are ignored based on a fixed criterion.
   * @param result Reference where the extracted file data will be saved.
   * @param hint Reference where a hint to help naming the extracted file will be stored.
   * @param type Type of the file to be extracted, either Field or Battle.
   * @return True if the there's a next file, false if we reached the last one.
   */
   bool extractNextSubFile (filedata_type &result, std::string &hint, int type)
   {
      if (m_subRecs.empty())
         throw std::exception("FileExtractor::loadSubIndex should be called before extracting any files");

      switch (type)
      {
         case Field: return extractNextFieldFile(result, hint);
         case Battle: return extractNextBattleFile(result, hint);
         default: throw std::exception("Can't extract, unknown file type.");
      }

      return false;
   }

   /**
   * Get the id of the file extracted in the last operation.
   * This id is also the position of the file in its respective index.
   * @return Id of the last extracted file.
   */
   int getLastId () const { return static_cast<int>(m_lastId); }

   /**
   * Retrieves information about the file extracted in the last operation.
   * This information is the same found in the index and describe the file
   * offset and its length (in bytes).
   * @return Information about the last extracted file.
   */
   fileinfo_type getLasInfo () const { return m_lastEntry; }


private:
   bool extractNextFieldFile (filedata_type &result, std::string &hint);
   bool extractNextBattleFile (filedata_type &result, std::string &hint);

   /**
   * Holds information about a particular file in the IMG file.
   * This information is used to extract that particular file.
   */
   typedef struct tagFF8IndexEntry {
      tagFF8IndexEntry (u32 off = 0, u32 len = 0) :
         offset(off), length(len) { }

      /**
      * Checks wether this particular file is valid or not.
      * @return True is its a valid file, false otherwise.
      */
      bool isInvalid () const {
         return offset == 0x00000000 || length == 0;
      }

      u32 offset; /**< Offset inside IMG file. */
      u32 length; /**< Length (in bytes).      */
   } IndexEntry;

   std::vector<IndexEntry>::difference_type m_lastId; /**< ID of last extracted file. */
   fileinfo_type m_lastEntry; /**< Info about last extracted file. */

   std::vector<IndexEntry> m_records; /**< Main index records */
   std::vector<IndexEntry> m_subRecs; /**< Sub-index records */
   std::vector<IndexEntry>::iterator m_last; /**< Iterative extraction iterator. */

   Dictionary tbl;         /**< Dictionary to collect field battle info. */
   std::ifstream m_img;    /**< .IMG file */
   FF8ExtractInfo &m_info; /**< Info from extractdata.xml */
};

#endif //~FILEEXTRACTOR_HPP