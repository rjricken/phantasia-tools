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

#include "insertinfo.hpp"

#include <string>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time.hpp>
#include <xmlparser.h>

using namespace std;
using namespace boost::posix_time;
using boost::lexical_cast;

template <typename T> std::string toHexString (T value) {
   return (boost::format("%1$X") % value).str();
}

void FF8InserterInfo::loadFromFile (const string &fileName)
{
   XMLResults pResults;

   XMLNode root = XMLNode::parseFile(fileName.c_str(), "InserterInfo", &pResults);
   if (pResults.error) throw exception("XML parse error.");

   XMLNode discNd = root.getChildNode("Disc");
   if (discNd.isEmpty()) throw exception("Information about the specified FF8 disc was not found");

   int n = lexical_cast <int>(discNd.getAttribute("n"));
   string img = discNd.getAttribute("img");
   this->m_disc = n;
   this->m_imgName = img;

   XMLNode indexNd = discNd.getChildNode("Index");
   u32 start = strtol(indexNd.getAttribute("start"), 0, 16);
   u32 end = strtol(indexNd.getAttribute("end"), 0, 16);
   
   this->setIndexInfo(start, end);
   FF8InserterInfo &info = *this;

   for (int i = 0; i < discNd.nChildNode("Folder"); i++)
   {
      XMLNode curFolderNd = discNd.getChildNode("Folder", i);
      string folderName = curFolderNd.getAttribute("name");

      if (folderName != "Other")
      {
         XMLNode fIndexNd = curFolderNd.getChildNode("Index");
         u32 idxStart = strtol(fIndexNd.getAttribute("start"), 0, 16);
         u32 idxEnd = strtol(fIndexNd.getAttribute("end"), 0, 16);
         string idxFile = fIndexNd.getAttribute("file");

         info.add(folderName, idxFile, idxStart, idxEnd);
      }

      for (int j = 0; j < curFolderNd.nChildNode("File"); j++)
      {
         XMLNode curFileNd = curFolderNd.getChildNode("File", j);
         int id = lexical_cast<int>(curFileNd.getAttribute("id"));
         string fileName = curFileNd.getAttribute("name");
         u32 offset = strtol(curFileNd.getAttribute("offset"), 0, 16);
         u32 length = lexical_cast<u32>(curFileNd.getAttribute("length"));

         info[folderName].add(id, fileName, offset, length);

         for (int k = 0; k < curFileNd.nChildNode("Script"); k++)
         {
            XMLNode curScriptNd = curFileNd.getChildNode("Script", k);
            string scriptName = curScriptNd.getAttribute("name");

            info[folderName][fileName].add(scriptName);
            FF8InserterScript &last = info[folderName][fileName].last();

            if (folderName == "Other")
            {
               int format = lexical_cast<int>(curScriptNd.getAttribute("format"));
               u32 ptrtbl = strtol(curScriptNd.getAttribute("ptrtbl"), 0, 16);
               u32 txtdata = strtol(curScriptNd.getAttribute("txtdata"), 0, 16);

               last.m_format = format;
               last.m_ptrOffset = ptrtbl;
               last.m_textOffset = txtdata;
            }

            string version = curScriptNd.getAttribute("version");
            if (!version.empty()) last.m_version = from_iso_string(version);
         }

         if (curFileNd.nChildNode("Insert"))
         {
            string version = curFileNd.getChildNode("Insert").getAttribute("version");
            if (!version.empty()) info[folderName][fileName].update(from_iso_string(version));
         }
      }
   }
}

void FF8InserterInfo::saveToFile (const string &fileName)
{
   XMLNode root = XMLNode::createXMLTopNode("InserterInfo");
   root.addAttribute("xmlns", "projeto-ffviii-br");

   XMLNode discNd = root.addChild("Disc");
   discNd.addAttribute("n", lexical_cast<string>(m_disc).c_str());
   discNd.addAttribute("img", m_imgName.c_str());

   XMLNode midxNd = discNd.addChild("Index");
   midxNd.addAttribute("start", toHexString<u32>(m_indexStart).c_str());
   midxNd.addAttribute("end", toHexString<u32>(m_indexEnd).c_str());

   // iterates through all the folders
   for (folder_iterator i = m_folders.begin(); i != m_folders.end(); ++i)
   {
      XMLNode curFolderNd = discNd.addChild("Folder");
      curFolderNd.addAttribute("name", i->second.name().c_str());

      // information about sub-indices
      if (i->second.name() != "Other")
      {
         XMLNode curIdxNd = curFolderNd.addChild("Index");
         curIdxNd.addAttribute("start", toHexString<u32>(i->second.indexStart()).c_str());
         curIdxNd.addAttribute("end", toHexString<u32>(i->second.indexEnd()).c_str());
         curIdxNd.addAttribute("file", i->second.indexFile().c_str());
      }

      // iterates through all the files inside the current folder
      for (FF8InserterFolder::file_iterator j = i->second.begin(); j != i->second.end(); ++j)
      {
         XMLNode curFileNd = curFolderNd.addChild("File");
         curFileNd.addAttribute("id", lexical_cast<string>(j->second.id()).c_str());
         curFileNd.addAttribute("name", j->second.name().c_str());
         curFileNd.addAttribute("offset", toHexString<u32>(j->second.offset()).c_str());
         curFileNd.addAttribute("length", lexical_cast<string>(j->second.length()).c_str());

         // iterates through all the scripts inside the current file
         for (FF8InserterFile::script_iterator k = j->second.begin(); k != j->second.end(); ++k)
         {
            XMLNode curScriptNd = curFileNd.addChild("Script");
            curScriptNd.addAttribute("name", k->m_name.c_str());

            if (i->second.name() == "Other")
            {
               curScriptNd.addAttribute("format", lexical_cast<string>(k->m_format).c_str());
               curScriptNd.addAttribute("ptrtbl", toHexString<u32>(k->m_ptrOffset).c_str());
               curScriptNd.addAttribute("txtdata", toHexString<u32>(k->m_textOffset).c_str());
            }

            ptime &v = k->m_version;
            curScriptNd.addAttribute("version", v.is_not_a_date_time() ? "" : to_iso_string(v).c_str());
         }

         // if this file has a valid version, create the apropriate node
         if (!j->second.version().is_not_a_date_time())
         {
            XMLNode curInsNd = curFileNd.addChild("Insert");
            curInsNd.addAttribute("version", to_iso_string(j->second.version()).c_str());
         }
      }
   }   

   root.writeToFile(fileName.c_str(), "UTF-8", 1);
}