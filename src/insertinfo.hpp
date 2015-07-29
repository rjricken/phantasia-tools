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

#ifndef INSERTINFO_HPP
#define INSERTINGO_HPP

#include <string>
#include <vector>
#include <map>
#include <boost/date_time.hpp>

#include "common.hpp"

struct FF8InserterScript
{
   FF8InserterScript (std::string name = "", int format = -1, u32 ptroff = 0, u32 txtoff = 0) :
      m_name(name), m_format(format), m_ptrOffset(ptroff), m_textOffset(txtoff) {
      m_version = boost::posix_time::not_a_date_time;
   }

   std::string m_name;
   boost::posix_time::ptime m_version;

   int m_format;
   u32 m_ptrOffset;
   u32 m_textOffset;
};

//============================================================================================
// FF8INSERTERFILE
//============================================================================================

class FF8InserterFile
{
public:
   typedef std::vector<FF8InserterScript>::iterator script_iterator;

   FF8InserterFile (int id = -1, const std::string &name = "", u32 offset = 0, u32 len = 0):
      m_id(id), m_fileName(name), m_offset(offset), m_length(len) { }

   void add (std::string name, int format = -1, u32 ptroff = 0, u32 txtoff = 0) {
      FF8InserterScript temp(name, format, ptroff, txtoff);
      m_scripts.push_back(temp);
   }

   void update (boost::posix_time::ptime newVersion) { m_version = newVersion; }

   int id () const { return m_id; }
   const std::string &name () const { return m_fileName; }
   u32 offset () const { return m_offset; }
   u32 length () const { return m_length; }
   const boost::posix_time::ptime &version () const { return m_version; }
   std::vector<FF8InserterScript>::size_type numScripts () const { return m_scripts.size(); }

   bool hasVersion () const { return !m_version.is_not_a_date_time(); }

   script_iterator begin () { return m_scripts.begin(); }
   script_iterator end () { return m_scripts.end(); }
   FF8InserterScript &last () { return m_scripts.back(); }

private:
   int m_id;
   std::string m_fileName;
   u32 m_offset;
   u32 m_length;

   boost::posix_time::ptime m_version;
   std::vector<FF8InserterScript> m_scripts;
};

//============================================================================================
// FF8INSERTERFOLDER
//============================================================================================

class FF8InserterFolder
{
public:
   typedef std::map<std::string, FF8InserterFile>::iterator file_iterator;

   FF8InserterFolder (std::string name = "", std::string index = "", u32 start = 0, u32 end = 0) :
      m_name(name), m_indexFile(index), m_indexStart(start), m_indexEnd(end) { }

   FF8InserterFile &operator[] (const std::string &file) { return m_files.at(file); }
   const FF8InserterFile &operator[] (const std::string &file) const { return m_files.at(file); }

   void add (int id, std::string name, u32 offset, u32 len) {
      FF8InserterFile temp(id, name, offset, len);
      m_files[name] = temp;
   }

   void setIndexInfo (const std::string &name, u32 start, u32 end) {
      m_indexFile = name;
      m_indexStart = start;
      m_indexEnd = end;
   }

   const std::string &name () const { return m_name; }
   const std::string &indexFile () const { return m_indexFile; }
   u32 indexStart () const { return m_indexStart; }
   u32 indexEnd () const { return m_indexEnd; }

   file_iterator begin () { return m_files.begin(); }
   file_iterator end () { return m_files.end(); }

   bool hasSubIndex () { return m_indexFile.size() && m_indexStart & m_indexEnd; }

private:
   std::string m_name;
   
   std::string m_indexFile;
   u32 m_indexStart;
   u32 m_indexEnd;

   std::map<std::string, FF8InserterFile> m_files; 
};

//============================================================================================
// FF8INSERTERINFO
//============================================================================================

class FF8InserterInfo
{
public:
   typedef std::map <std::string, FF8InserterFolder>::iterator folder_iterator;

   FF8InserterInfo (int disc = 0, std::string img = "", u32 idxStart = 0, u32 idxEnd = 0) :
      m_disc(disc), m_imgName(img), m_indexStart(idxStart), m_indexEnd(idxEnd) {
      add("Other", "", 0, 0);
   }

   FF8InserterFolder &operator[] (const std::string &folder) { return m_folders.at(folder); }
   const FF8InserterFolder &operator[] (const std::string &folder) const { return m_folders.at(folder); }

   // persistence -------------------------
   void saveToFile (const std::string &fileName);
   void loadFromFile (const std::string &fileName);

   void add (const FF8InserterFolder &folder) {
      m_folders[folder.name()] = folder;
   }

   void add (std::string name, std::string index, u32 start, u32 end) {
      FF8InserterFolder temp(name, index, start, end);
      m_folders[name] = temp;
   }

   void setIndexInfo (u32 start, u32 end) { m_indexStart = start; m_indexEnd = end; }

   int disc () const { return m_disc; }
   const std::string &img () const { return m_imgName; }
   u32 indexStart () const { return m_indexStart; }
   u32 indexEnd () const { return m_indexEnd; }

   folder_iterator begin () { return m_folders.begin(); }
   folder_iterator end () { return m_folders.end(); }

private:
   int m_disc;
   std::string m_imgName;

   u32 m_indexStart;
   u32 m_indexEnd;

   std::map <std::string, FF8InserterFolder> m_folders;
};

#endif //~INSERTINGO_HPP