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

#ifndef TEXTDUMPER_HPP
#define TEXTDUMPER_HPP

#include "common.hpp"
#include "dictionary.hpp"
#include "pointerdesc.hpp"

#include <map>
#include <string>
#include <utility>
#include <boost/format.hpp>
#include <boost/shared_array.hpp>

/**
* Dumps text data into a readable format from all the
* major file types found in the Final Fantasy 8 game.
*/
class TextDumper
{
public:
   /** Used to represent a binary data block */
   typedef std::pair<boost::shared_array<u8>, u32> filedata_type;

   TextDumper (const filedata_type &data, const Dictionary &dic) :
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

   void dump(std::string &result, int type, u32 ptrOff = 0, u32 txtOff = 0);

private:
   void dumpFromBattleScene (std::string &result);
   void dumpFromFieldDialogs (std::string &result);
   void dumpFromLinearData (std::string &result, u32 ptrOff, bool seedTest = false);
   void dumpFromPackedData (std::string &result, u32 ptrOff);
   void dumpFromRefinesData (std::string &result, u32 ptrOff, u32 txtOff);
   void dumpFromMenuHelpData (std::string &result, u32 ptrOff, u32 txtOff);
   void dumpFromMenuBattleData (std::string &result, u32 ptrOff, u32 txtOff);
   void dumpFromMainMenuData (std::string &result, u32 ptrOff);
   std::string translateBlock (const u8 *data);

   const filedata_type m_data;    /**< Data containing text data to be dumped from.                 */
   const Dictionary &m_tbl;       /**< Dictionary used to translate binary data into readable text. */
   PointerDescription m_ptrdesc;  /**< Description of all pointer blocks in the battle module.      */
};

#endif //~TEXTDUMPER_HPP