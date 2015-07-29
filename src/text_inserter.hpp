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

#ifndef TEXTINSERTER_HPP
#define TEXTINSERTER_HPP

#include "common.hpp"
#include "dictionary.hpp"
#include "data_structure.hpp"
#include "pointerdesc.hpp"

#include <vector>
#include <utility>
#include <boost/shared_array.hpp>

class TextInserter
{
public:
   typedef std::pair<boost::shared_array<u8>, u32> filedata_type;

   TextInserter (const filedata_type &data, const Dictionary &dic) :
      m_data(data), m_tbl(dic) { }

   enum {
      Battle,     /**< Field Battle files (.dat)       */
      Field,      /**< Field Dialogs files (.msd)      */
      Linear,     /**< Misc files with linear pointers */
      Packed,     /**< Main Menu files                 */
      SeedTest,   /**< SeeD Test files                 */
      Refines,    /**< Refine System files             */
      MenuHelp,   /**< Help System files               */
      MenuBattle, /**< Battle Menu System files        */
      MainMenu    /**< Main menu basic entries data    */
   };

   void insert (const std::string &script, int type, u32 ptrOff = 0, u32 txtOff = 0);

   filedata_type getModifiedFile () { return m_data; }

private:
   void insertIntoBattleScene (const filedata_type &scriptData, std::vector<u16> &pointers);
   void insertIntoFieldDialogs (const filedata_type &scriptData, std::vector<u32> &pointers);
   void insertIntoLinearData (const filedata_type &scriptData, std::vector<u16> &pointers, u32 ptrOff);
   void insertIntoPackedData (const filedata_type &scriptData, std::vector<u16> &pointers, u32 ptrOff);
   void insertIntoRefinesData (const filedata_type &scriptData, std::vector<u16> &pointers, u32 ptrOff, u32 txtOff);
   void insertIntoMenuHelpData (std::vector<filedata_type> &encBlocks, u32 ptrOff, u32 txtOff);
   void insertIntoMenuBattleData (const filedata_type &scriptData, std::vector<u16> &pointers, u32 ptrOff, u32 txtOff);
   void insertIntoMainMenuData (const filedata_type &scriptData, std::vector<u16> &pointers, u32 ptrOff);
   
   u32 encodeScript (std::vector<std::string> &lines, u8 *buffer, std::vector<u32> &pointers, bool newsessions = true, bool dtes = true);
   u32 translateBlock (const std::string &block, u8 *buffer, bool newsessions = true, bool dtes = true);

   /**
   * Calculates the padding necessary to align a specified value into a certain boundary.
   * @param value Value to be aligned.
   * @param boundary The boundary to be aligned.
   * @return Padding necessary to align the specified value in the specified boundary.
   */
   int calcPadding (u32 value, u32 boundary) {
      return value % boundary ? boundary - (value % boundary) : 0;
   }

   filedata_type m_data;

   const Dictionary &m_tbl;
   PointerDescription m_ptrdesc;
};

#endif //~TEXTINSERTER_HPP