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

#include "text_dumper.hpp"
#include "data_structure.hpp"

#include <map>
#include <sstream>
#include <algorithm>
#include <exception>

using namespace std;

/**
* Manages the dump process, forwarding the data to the apropriate dump method.
* @param result String where the text script will be written to.
* @param type The type of file to dump from.
* @param ptrOff Offset of the pointers table (optional).
* @param txtOff Offset of the text data (optional).
*/
void TextDumper::dump(string &result, int type, u32 ptrOff, u32 txtOff)
{
   switch (type)
   {
      case Battle:
         dumpFromBattleScene(result);
         break;

      case Field:
         dumpFromFieldDialogs(result);
         break;

      case Linear:
         dumpFromLinearData(result, ptrOff);
         break;

      case Packed:
         dumpFromPackedData(result, ptrOff);
         break;

      case SeedTest:
         dumpFromLinearData(result, ptrOff, true);
         break;

      case Refines:
         dumpFromRefinesData(result, ptrOff, txtOff);
         break;

      case MenuHelp:
         dumpFromMenuHelpData(result, ptrOff, txtOff);
         break;

      case MenuBattle:
         if (!m_ptrdesc.isLoaded()) m_ptrdesc.loadFromFile("battle-module.xml");
         dumpFromMenuBattleData(result, ptrOff, txtOff);
         break;

      case MainMenu:
         dumpFromMainMenuData(result, ptrOff);
         break;

      default:
         throw exception("TextDumper is unable to dump from the specified file type.");
   }
}

/**
* Dumps text data in a readable format from .dat files, which
* includes the dialogues found inside battle scenes.
* @param result String where the text script will be written to.
*/
void TextDumper::dumpFromBattleScene (string &result)
{
   typedef FieldBattleTextSectionHeader TextSectionHeader;
   u8 *dataPtr = m_data.first.get();
   u32 dataLen = m_data.second;

   const u32 *num_sections = (u32 *)dataPtr;

   if (*num_sections != 0x0b)
      throw exception("There is no battlescript section in the specified file.");

   const FieldBattleHeader *header = (FieldBattleHeader *)(dataPtr + 4);

   if (header->ptr_battlescript >= dataLen)
      throw exception("Reached EOF before dumping any text data.");

   if (header->ptr_eof != dataLen)
      throw exception("Malformed .dat file.");

   const TextSectionHeader *txtheader = (TextSectionHeader *)(dataPtr + header->ptr_battlescript);

   if (txtheader->text_ptrtbl == txtheader->text_data)
      throw exception("There is no text data to be dumped.");

   const u32 off_ptrtbl = header->ptr_battlescript + txtheader->text_ptrtbl;
   const u32 off_txtdata = header->ptr_battlescript + txtheader->text_data;
   const u32 num_ptr = (off_txtdata - off_ptrtbl) / 2;

   for (u32 i = 0; i < num_ptr; i++)
   {
      u16 *cur = (u16 *)(dataPtr + off_ptrtbl + i * 2);
      const u8 *block_start = dataPtr + off_txtdata + *cur;

      string encBlock = translateBlock(block_start);
      result.append(encBlock);
   }
}

/**
* Dumps text data in a readable format from .msd files, which
* includes the dialogues found outside battles thorugh all the game.
* @param result String where the text script will be written to.
*/
void TextDumper::dumpFromFieldDialogs (string &result)
{
   u8 *dataPtr = m_data.first.get();
   FieldDialogsHeader header;

   // subtracts 0x800e1000 from each pointer to obtain the correct offset.
   u32 *first = (u32 *)dataPtr, *last = (u32 *)dataPtr + 12, *d_first = (u32 *)&header;
   transform(first, last, d_first, bind2nd(minus<u32>(), 0x800e1000));

   if (header.ptr_textdata == header.ptr_section9)
      throw exception("There is no text data to be dumped.");

   u32 num_ptr = *((u32 *)(dataPtr + header.ptr_textdata)) / 4;

   for (u32 i = 0; i < num_ptr; i++)
   {
      u32 *cur = (u32 *)(dataPtr + header.ptr_textdata + i * 4);
      const u8 *block_start = dataPtr + header.ptr_textdata + *cur;

      string encBlock = translateBlock(block_start);
      result.append(encBlock);
   }
}

/**
* Dumps text data in a readable format from linear data files, which
* includes files from various modules and various purposes.
* @param result String where the text script will be written to.
* @param ptrOff Offset of the pointers table.
* @param seedTest Flag to make it work with SeeD Test files.
*/
void TextDumper::dumpFromLinearData (string &result, u32 ptrOff, bool seedTest)
{
   u8 *dataPtr = m_data.first.get() + ptrOff;
   u16 num_ptr = *((u16 *)dataPtr);

   for (u32 i = 0; i < num_ptr; i++)
   {
      u16 *cur = (u16 *)(dataPtr + 2 + i * 2);
      const u8 *block_start = dataPtr + *cur;
      
      // in seedtest files, encodes the answer to one of the questions
      if (seedTest)
      {
         result.append("[" + hexEncode<u8>(*block_start) + "]");
         block_start++;
      }

      string encBlock = translateBlock(block_start);
      result.append(encBlock);
   }
}

/**
* Dumps text data in a readable format from packed files, which are
* made of one or more subsessions spread through all the file.
* @param result String where the text script will be written to.
* @param ptrOff Offset of the pointers table.
*/
void TextDumper::dumpFromPackedData (string &result, u32 ptrOff)
{
   u8 *dataPtr = m_data.first.get() + ptrOff;
   PackedMenuHeader *header = (PackedMenuHeader *)dataPtr;

   // dumps text data from valid blocks
   for (u16 *curBlock = header->blocks; curBlock != header->blocks + 17; curBlock++)
   {
      if (*curBlock == 0x0000) continue;
      u16 num_ptr = *(dataPtr + *curBlock);

      for (u32 i = 0; i < num_ptr; i++)
      {
         u16 *cur = (u16 *)(dataPtr + *curBlock + 2 + i * 2);
        
         if (*cur != 0x0000)
         {
            const u8 *block_start = dataPtr + *curBlock + *cur;

            string encBlock = translateBlock(block_start);
            result.append(encBlock);
         }
      }
   }
}

/**
* Dumps text data in a readable format from files with recipes for Refines.
* These files store the pointers table apart from the text data.
* @param result String where the text script will be written to.
* @param ptrOff Offset of the pointers table.
* @param txtOff Offset of the text data.
*/
void TextDumper::dumpFromRefinesData (string &result, u32 ptrOff, u32 txtOff)
{
   u8 *pointersPtr = m_data.first.get() + ptrOff;
   u8 *textPtr = m_data.first.get() + txtOff;

   RefinesPointerBlock *ptr = (RefinesPointerBlock *)pointersPtr;

   for (; ptr->unknown1 && ptr->unknown2 && ptr->unknown3; ptr++)
   {
      string encBlock = translateBlock(textPtr + ptr->text_start);
      result.append(encBlock);
   }
}

/**
* Dumps text data in a readable format from files containing the Tutorial
* section of the main menu. These files share the same pointers table, but
* store the text data in separate blocks.
* @param result String where the text script will be written to.
* @param ptrOff Offset of the pointers table.
* @param txtOff Offset of the text data.
*/
void TextDumper::dumpFromMenuHelpData (string &result, u32 ptrOff, u32 txtOff)
{
   u8 *pointersPtr = m_data.first.get() + ptrOff;
   u8 *textPtr = m_data.first.get() + txtOff;

   MenuHelpPointer *ptr = (MenuHelpPointer *)pointersPtr;
   u16 block = ptr->num_block;

   for (; ptr->num_block == block; ptr++)
   {
      const u8 *curBlock = textPtr + ptr->ptr_text;
      MenuHelpTextFlags *flags = (MenuHelpTextFlags *)curBlock;

      // block starts after the title (which ends in 0x00, but didn't get its own pointer)
      const u8 *block_start = find(curBlock + sizeof(*flags), curBlock + flags->block_size, 0x00) + 1;

      string encTitle = translateBlock(curBlock + sizeof(*flags));
      string encBlock = translateBlock(block_start);

      result.append(encTitle);
      result.append(encBlock);
   }

}

/**
* Dumps text data in a readable format from the battle module file, which
* uses different structures to represent the pointers of each one of its
* 25 blocks. The dumping relies on data from a XML files.
* @param result String where the text script will be written to.
* @param ptrOff Offset of the pointers table.
* @param txtOff Offset of the text data.
*/
void TextDumper::dumpFromMenuBattleData (string &result, u32 ptrOff, u32 txtOff)
{
   u8 *dataPtr = m_data.first.get();
   u8 *pointersPtr = m_data.first.get() + ptrOff;
   u8 *textPtr = m_data.first.get() + txtOff;

   // find out where the text offset of this block is located inside the pointers table
   u32 *txttbl = (u32 *)(dataPtr + 0x80);
   u32 *curtbl = find(txttbl, txttbl + 25, txtOff);

   // figure out which pointer description should be used
   int n = static_cast<int> (distance(txttbl, curtbl) + 1);

   // iterate through the pointer blocks and extract the text pointed by valid ones
   for (int i = 0; i < m_ptrdesc[n].m_count; i++)
   {
      u16 *cur = (u16 *)(pointersPtr + i * m_ptrdesc[n].m_width);

      for (int p = 0; p < m_ptrdesc[n].m_group && *(cur + p) != 0xffff; p++)
      {
         string encBlock = translateBlock(textPtr + *(cur + p));
         result.append(encBlock);
      }
   }
}

/**
* Dumps text data in a readable format from the main menu entries data.
* @param result String where the text script will be written to.
* @param ptrOff Offset of the pointers table.
*/
void TextDumper::dumpFromMainMenuData (string &result, u32 ptrOff)
{
   u8 *dataPtr = m_data.first.get() + ptrOff;
   u16 num_ptr = *((u16 *)dataPtr);

   for (u16 *cur = (u16 *)dataPtr + 1; cur != (u16 *)dataPtr + 1 + num_ptr; cur++)
      if (*cur) result.append(translateBlock(dataPtr + *cur));
}

/**
* Translates a binary block from FF8 files into a readable text script.
* @param data A pointer into the location of the given block.
* @return The readable text script generated.
*/
string TextDumper::translateBlock (const u8 *data)
{
   ostringstream result;

   for (u32 i=0; ; i++)
   {
      if (data[i] == 0x00) break;
      else if (data[i] == 0x01) result << endl << endl << endl;
      else if (data[i] == 0x02) result << endl;
      else if (data[i] >= 0x03 && data[i] <= 0x0e)
      {
         u16 mte = (data[i++] << 8) | (data[i + 1] & 0xff);
         string r = m_tbl.find<u16>(mte);

         result << '[' << (r.empty() ? hexEncode<u16>(mte) : r) << ']';
      }
      else
      {
         string r = m_tbl.find<u8>(data[i]);
         result << (r.empty() ? "[" + hexEncode<u8>(data[i]) + "]" : r);
      }
   }

   result << endl << string(33, '-') << endl;
   return result.str();
}