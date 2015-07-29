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

#ifndef POINTERDESC_HPP
#define POINTERDESC_HPP

#include <map>
#include <string>
#include <exception>
#include <xmlparser.h>
#include <boost/lexical_cast.hpp>

class PointerDescription
{
public:
   PointerDescription () { }

   /**
   * Represents the structure of a pointer found in the battle module.
   * The data is loaded from a previously hand made XML file.
   **/
   typedef struct tagFF8BattleModulePointerDescription {
      tagFF8BattleModulePointerDescription (int id = -1, int count = 0, int width = 0, int group = 0) :
         m_id(id), m_count(count), m_width(width), m_group(group) { }

      int m_id;    /**< Block number.                              */
      int m_count; /**< Number of pointer blocks.                  */
      int m_width; /**< Number of bytes per pointer block.         */
      int m_group; /**< Number of text pointers per pointer block. */
   } BattleModulePtrDesc;

   BattleModulePtrDesc &operator[] (int n) { return m_blocks.at(n); }
   const BattleModulePtrDesc &operator[] (int n) const { return m_blocks.at(n); }

   /**
   * Tells whether XML file was loaded or not.
   * @return True if it's already loaded, false otherwise.
   */
   bool isLoaded () const { return m_blocks.size() > 0; }

   /**
   * Loads the hand made XML files containing the description of the
   * pointer structures found in the battle module file.
   * @param xmlFile Path to the XML file.
   */
   void loadFromFile (const std::string &xmlFile)
   {
      XMLResults pResults;
      XMLNode root = XMLNode::parseFile(xmlFile.c_str(), "BattleModule", &pResults);

      if (pResults.error) throw std::exception(("Error parsing " + xmlFile + " file.").c_str());

      for (int i=0; i<root.nChildNode("Block"); i++)
      {
         XMLNode blockNd = root.getChildNode("Block", i);
         int n = boost::lexical_cast<int>(blockNd.getAttribute("n"));
         int count = boost::lexical_cast<int>(blockNd.getAttribute("count"));
         int width = boost::lexical_cast<int>(blockNd.getAttribute("width"));
         int group = boost::lexical_cast<int>(blockNd.getAttribute("group"));

         m_blocks[n] = BattleModulePtrDesc(n, count, width, group);
      }
   }

private:
   std::map<int, BattleModulePtrDesc> m_blocks; /** Description of all the pointer blocks from the battle module. */
};

#endif //~POINTERDESC_HPP