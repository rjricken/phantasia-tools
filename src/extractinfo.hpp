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

#ifndef EXTRACTINFO_HPP
#define EXTRACTINFO_HPP

#include <string>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include "common.hpp"

struct FF8TxtDataInfo
{
   FF8TxtDataInfo (u32 pointersOffset = 0, u32 textOffset = 0, int format = -1, std::string comment = "") :
      m_ptrOff(pointersOffset), m_txtOff(textOffset), m_format(format), m_nameHint(comment) { }

   u32 m_ptrOff;
   u32 m_txtOff;
   int m_format;
   std::string m_nameHint;
};

struct FF8SubIndexInfo
{
   FF8SubIndexInfo (u32 begin = 0, u32 end = 0, std::string folder = "", std::string ext = "") :
      m_startOff(begin), m_endOff(end), m_folder(folder), m_ext(ext) { }

   u32 m_startOff;
   u32 m_endOff;
   std::string m_folder;
   std::string m_ext;
};

//============================================================================================
// FF8FILEINFO
//============================================================================================

class FF8FileInfo
{
public:
   FF8FileInfo (u16 id = 0, std::string type = "", std::string name= "", std::string comment = "") :
      m_id(id), m_ext(type), m_nameHint(name), m_comment(comment) { }

   typedef std::vector<FF8TxtDataInfo>::iterator txtdata_iterator;

   // utilities -------------------------------
   void add (u32 ptrStart, u32 txtStart, int format, std::string comment)
   {
      FF8TxtDataInfo tdi(ptrStart, txtStart, format, comment);
      m_dataInfo.push_back(tdi);
   }

   void addIndex (u32 begin, u32 end, std::string folder, std::string ext) {
      m_idxInfo.reset(new FF8SubIndexInfo(begin, end, folder, ext));
   }

   // acessors ------------------------------
   u16 id () const { return m_id; }
   const std::string &ext () const { return m_ext; }
   const std::string &nameHint () const { return m_nameHint; }
   const std::string &comment () const { return m_comment; }
   u32 offset () const { return m_offset; }
   u32 length () const { return m_length; }
   bool hasSubIndex () const { return m_idxInfo.get() != 0; }
   bool hasTextData () const { return !m_dataInfo.empty(); }
   FF8SubIndexInfo &subIndex () const { return *m_idxInfo.get(); }
   txtdata_iterator begin () { return m_dataInfo.begin(); }
   txtdata_iterator end () { return m_dataInfo.end(); }

   void offset (u32 value) { m_offset = value; }
   void length (u32 value) { m_length = value; }

private:
   u16 m_id;               /**< File ID                      */
   std::string m_ext;      /**< File extension               */
   std::string m_nameHint; /**< A hint to help into naming the file */
   std::string m_comment;  /**< Comments regarding this file */

   u32 m_offset;           /**< File offset inside .IMG      */
   u32 m_length;           /**< File length (in bytes)       */

   std::vector<FF8TxtDataInfo> m_dataInfo;
   boost::shared_ptr<FF8SubIndexInfo> m_idxInfo;
};

//============================================================================================
// FF8EXTRACTINFO
//============================================================================================

class FF8ExtractInfo
{
public:
   FF8ExtractInfo(const std::string &imgName = "", u8 discNum = 0, u16 secSize = 0) :
      m_imgName(imgName), m_discNum(discNum), m_secSize(secSize) { }

   typedef std::map<u16, FF8FileInfo>::iterator iterator;

   // operators -----------------------------
   FF8FileInfo &operator[] (u16 id) {
      return m_fileInfo.at(id);
   }

   // utilities -------------------------------
   void add (u16 id, std::string ext, std::string name, std::string comment)
   {
      FF8FileInfo fi(id, ext, name, comment);
      m_fileInfo[id] = fi;
   }

   void loadFromFile(std::string xmlName, int discNum);

   // setters -------------------------------
   void setIndexInfo (u32 offset, u32 sector) {
      m_idxOffset = offset;
      m_idxSector = sector;
   }

   void setInfo (std::string &imgName, u8 discNum, u16 secSize) {
      m_imgName = imgName;
      m_discNum = discNum;
      m_secSize = secSize;
   }
   
   // acessors
   const std::string &imgName () const { return m_imgName; }
   u8 discNum () const { return m_discNum; }
   u16 secSize () const { return m_secSize; }
   u32 indexOffset () const { return m_idxOffset; }
   u32 indexSector () const { return m_idxSector; }
   bool exists (u16 index) const { return m_fileInfo.count(index) > 0; }
   std::map<u16, FF8FileInfo>::iterator begin () { return m_fileInfo.begin(); }
   std::map<u16, FF8FileInfo>::iterator end () { return m_fileInfo.end(); }

private:
   std::string m_imgName; /**< .IMG file name                                 */
   u8 m_discNum;          /**< Disc number (1-4)                              */
   u16 m_secSize;         /**< Size of each sector (in bytes)                 */

   u32 m_idxOffset;       /**< .IMG index offset                              */
   u32 m_idxSector;       /**< Sector position of .IMG file inside disc image */

   std::map<u16, FF8FileInfo> m_fileInfo;
};

#endif //~FF8EXTRACTINFO_HPP