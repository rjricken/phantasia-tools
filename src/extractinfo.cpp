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

#include "extractinfo.hpp"

#include <exception>
#include <xmlparser.h>
#include <boost/lexical_cast.hpp>

using namespace std;
using boost::lexical_cast;

void FF8ExtractInfo::loadFromFile(string xmlName, int discNum)
{
   XMLResults pResults;

   XMLNode root = XMLNode::parseFile(xmlName.c_str(), "ExtractInfo", &pResults);
   if (pResults.error) throw exception("XML parse error.");

   XMLNode discInfoNd = root.getChildNodeWithAttribute("DiscInfo", "number", lexical_cast<string>(discNum).c_str());
   if (discInfoNd.isEmpty()) throw exception("Information about the specified FF8 disc was not found");

   // Collects information about the current disc.
   u8 number = lexical_cast<int>(discInfoNd.getAttribute("number"));
   string img = discInfoNd.getAttribute("img");
   u16 secsize = static_cast<u16>(strtol(discInfoNd.getAttribute("secsize"), 0, 16));

   XMLNode mainIdxNd = discInfoNd.getChildNode("MainIndex");
   u32 off = strtol(mainIdxNd.getAttribute("off"), 0, 16);
   u32 discpos = strtol(mainIdxNd.getAttribute("discpos"), 0, 16);

   this->setInfo(img, number, secsize);
   this->setIndexInfo(off, discpos);

   // Collects information about files from EntryInfo node.
   for (int i = 0; i < mainIdxNd.nChildNode("EntryInfo"); i++)
   {
      XMLNode entryInfoNd = mainIdxNd.getChildNode("EntryInfo", i);
      u16 n = lexical_cast<int>(entryInfoNd.getAttribute("n"));
      string type = entryInfoNd.getAttribute("type");
      string comment = entryInfoNd.getAttribute("comment");
      string name = entryInfoNd.isAttributeSet("name") ?
         entryInfoNd.getAttribute("name") : "";
      
      this->add(n, type, name, comment);

      if (entryInfoNd.nChildNode("SubIndex"))
      {
         XMLNode subIndexNd = entryInfoNd.getChildNode("SubIndex");

         u32 begin = strtol(subIndexNd.getAttribute("begin"), 0, 16);
         u32 end = strtol(subIndexNd.getAttribute("end"), 0, 16);
         string folder = subIndexNd.getAttribute("folder");
         string ext = subIndexNd.getAttribute("ext");

         (*this)[n].addIndex(begin, end, folder, ext);
      }

      // Collects information about text data from Data node.
      for (int j = 0; j < entryInfoNd.nChildNode("Data"); j++)
      {
         XMLNode dataNd = entryInfoNd.getChildNode("Data", j);
         u32 ptroff = strtol(dataNd.getAttribute("ptroff"), 0, 16);
         u32 txtoff = strtol(dataNd.getAttribute("txtoff"), 0, 16);
         int format = lexical_cast<int>(dataNd.getAttribute("format"));
         string cmm = dataNd.getAttribute("comment");

         (*this)[n].add(ptroff, txtoff, format, cmm);
      }
   }
}