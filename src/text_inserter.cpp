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

#include "text_inserter.hpp"
#include <exception>
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string_regex.hpp>

using namespace std;
using boost::shared_array;

/**
* Manages the insert process, forwarding the data to the apropriate insert method.
* An entire new file is created preserving the original structure.
* @param script Text script to be inserted.
* @param type Format of the target file.
* @param ptrOff Offset of the pointers table (optional).
* @param txtOff Offset of the texta data (optional).
*/
void TextInserter::insert (const string &script, int type, u32 ptrOff, u32 txtOff)
{
   boost::regex re("\\[([\\d\\l\\u\\s_?]+)\\]");
   boost::match_results<string::const_iterator> what;

   // replaces any code inside brackets with an hexstring representation
   string temp = script;
   while (boost::regex_search(temp.cbegin(), temp.cend(), what, re))
   {
      string code = what[1];
      if (m_tbl.exists<u16>(code))
      {
         u16 value = m_tbl.find<u16>(code);
         string enc = hexEncode<u16>(value);
         temp.replace(what[1].first, what[1].second, enc);
      }
      else if (m_tbl.exists<u8>(code))
      {
         string enc = hexEncode<u8>(m_tbl.find<u8>(code));
         temp.replace(what[1].first, what[1].second, enc);
      }
      else throw exception(("Invalid code found: " + code).c_str());
   }

   // splits the script into sections delimited by the endstrings
   vector<string> lines;
   boost::split_regex(lines, temp, boost::regex("\\n-{33}\\n")).pop_back();
      
   switch (type)
   {
      case Battle: {
         shared_array<u8> buffer(new u8[script.size()]);
         vector<u32> block_offsets;

         // encodes the entire script
         u32 bufferLen = encodeScript(lines, buffer.get(), block_offsets);

         // field battle pointers are 16-bit wide
         vector<u16> pointers(block_offsets.size());
         transform(block_offsets.begin(), block_offsets.end(), pointers.begin(), s_cast<u32, u16>());

         insertIntoBattleScene(make_pair(buffer, bufferLen), pointers);
      }
      break;

      case Field: {
         shared_array<u8> buffer(new u8[script.size()]);
         vector<u32> block_offsets;

         u32 bufferLen = encodeScript(lines, buffer.get(), block_offsets);
         insertIntoFieldDialogs(make_pair(buffer, bufferLen), block_offsets);
      }
      break;

      /* case SeedTest:  { 
         vector<u16> pointers(block_offsets.size());
         copy(block_offsets.begin(), block_offsets.end(), pointers.begin());

         ignoreNewsessions = false;

         return insertIntoSeedTest(make_pair(buffer, bufferPos), pointers, make_pair(fileData, fileLen)); 
      }
      break; */

      case Packed: {
         shared_array<u8> buffer(new u8[script.size()]);
         vector<u32> block_offsets;

         u32 bufferLen = encodeScript(lines, buffer.get(), block_offsets, false, false);

         vector<u16> pointers(block_offsets.size());
         transform(block_offsets.begin(), block_offsets.end(), pointers.begin(), s_cast<u32, u16>());

         insertIntoPackedData(make_pair(buffer, bufferLen), pointers, ptrOff);
      }
      break;

      case Refines: {
         shared_array<u8> buffer(new u8[script.size()]);
         vector<u32> block_offsets;

         u32 bufferLen = encodeScript(lines, buffer.get(), block_offsets, false, /* TODO check if refines can use DTEs */false);

         vector<u16> pointers(block_offsets.size());
         transform(block_offsets.begin(), block_offsets.end(), pointers.begin(), s_cast<u32, u16>());

         insertIntoRefinesData(make_pair(buffer, bufferLen), pointers, ptrOff, txtOff);
      }
      break;

      case MenuHelp: {
         vector<filedata_type> blocks;

         for (vector<string>::iterator i = lines.begin(); i != lines.end(); i += 2)
         {
            shared_array<u8> buffer(new u8[i->size() + (i + 1)->size() + 32]);
            
            u32 bufferLen = translateBlock(*i, buffer.get(), false, /* TODO check if DTEs can REALLY be used */true);
            buffer.get()[bufferLen++] = 0x00;

            bufferLen += translateBlock(*(i + 1), buffer.get() + bufferLen, false, /* TODO check if DTEs can REALLY be used */true);
            buffer.get()[bufferLen++] = 0x00;

            blocks.push_back(make_pair(buffer, bufferLen));
         }

         insertIntoMenuHelpData(blocks, ptrOff, txtOff);
      }
      break;

      case MenuBattle: {
         if (!m_ptrdesc.isLoaded()) m_ptrdesc.loadFromFile("battle-module.xml");

         shared_array<u8> buffer(new u8[script.size()]);
         vector<u32> block_offsets;

         u32 bufferLen = encodeScript(lines, buffer.get(), block_offsets, false, false);

         vector<u16> pointers(block_offsets.size());
         transform(block_offsets.begin(), block_offsets.end(), pointers.begin(), s_cast<u32, u16>());

         insertIntoMenuBattleData(make_pair(buffer, bufferLen), pointers, ptrOff, txtOff);
      }
      break;

      case MainMenu: {
         shared_array<u8> buffer(new u8[script.size()]);
         vector<u32> block_offsets;

         u32 bufferLen = encodeScript(lines, buffer.get(), block_offsets, false, false);

         vector<u16> pointers(block_offsets.size());
         transform(block_offsets.begin(), block_offsets.end(), pointers.begin(), s_cast<u32, u16>());

         insertIntoMainMenuData(make_pair(buffer, bufferLen), pointers, ptrOff);
      }
      break;

      default:
         throw exception("TextInserter is unable to insert into the specified file type.");
   }
}

/**
* Inserts a binary block of encoded text data into FFVIII Field Battle files (.dat).
* The file gets reconstructed and the original data is preserved.
* @param scriptData Encoded text data to be inserted.
* @param pointers Pointers to each text block inside the encoded text data.
*/
void TextInserter::insertIntoBattleScene (const filedata_type &scriptData, vector<u16> &pointers)
{
   typedef FieldBattleTextSectionHeader TextSectionHeader;
   u8 *originalPtr = m_data.first.get(), *newDataPtr = scriptData.first.get();
   u32 originalLen = m_data.second, newDataLen = scriptData.second;

   // get the number of sections, just to be sure the file has text data
   const u32 *num_sections = (u32 *)originalPtr;
   if (*num_sections != 0x0b) throw exception("Incompatible file format");

   // copy the header info for later use
   FieldBattleHeader header;
   copy((u32 *)(originalPtr + 4), (u32 *)(originalPtr + 4) + *num_sections + 1, (u32 *)&header);

   const TextSectionHeader *txtheader = (TextSectionHeader *)(originalPtr + header.ptr_battlescript);

   // useful pointers
   u8 *textPointersPtr = originalPtr + header.ptr_battlescript + txtheader->text_ptrtbl;
   u8 *textDataPtr = originalPtr + header.ptr_battlescript + txtheader->text_data;
   u16 *first_ptr = (u16 *)textPointersPtr, *last_ptr = (u16 *)(textDataPtr - 2);

   // find out where the last text block from the old data ends
   u8 *lastBlockEnd = find_if(textDataPtr + *last_ptr, originalPtr + originalLen, bind2nd(equal_to<u8>(), 0x00));
   u32 extraLen = lastBlockEnd - textDataPtr - *last_ptr + 1;

   u32 oldDataLen = *last_ptr + extraLen - *first_ptr;

   // ------------------------------------------
   // rebuild the entire file using the new data
   shared_array<u8> buffer(new u8[originalLen + newDataLen - oldDataLen]);
   u8 *bufferPtr = buffer.get();
   u32 bufferTail = 0;

   // copy the data prior to the text section pointers
   copy(originalPtr, textPointersPtr, bufferPtr);
   bufferTail += textPointersPtr - originalPtr;

   // update the pointers table
   copy(pointers.begin(), pointers.end(), (u16 *)(bufferPtr + bufferTail));
   bufferTail += pointers.size() * 2;

   // copy the new encoded text data
   copy(newDataPtr, newDataPtr + newDataLen, bufferPtr + bufferTail);
   bufferTail += newDataLen;

   // copy the remaining data from the file
   copy(textDataPtr + oldDataLen, originalPtr + originalLen, bufferPtr + bufferTail);
   bufferTail += originalLen - (textDataPtr + oldDataLen - originalPtr);

   // header needs to be updated only if the new data has a different size
   if (newDataLen != oldDataLen)
   {
      // update the file header
      u32 *hfirst = (u32 *)&header, *hlast = (u32 *)&header + sizeof(header) / 4;
      transform(hfirst + 9, hlast, hfirst + 9, bind2nd(plus<u32>(), newDataLen - oldDataLen));

      // copy updated header into proper position
      copy((u8 *)&header, (u8 *)&header + sizeof(header), bufferPtr + 4);
   }

   m_data = make_pair(buffer, bufferTail);
}

void TextInserter::insertIntoFieldDialogs (const filedata_type &scriptData, vector<u32> &pointers)
{
   u8 *originalPtr = m_data.first.get(), *newDataPtr = scriptData.first.get();
   u32 originalLen = m_data.second, newDataLen = scriptData.second;

   FieldDialogsHeader header;
   u32 *first = (u32 *)originalPtr, *last = (u32 *)originalPtr + 12, *d_first = (u32 *)&header;

   // subtracts 0x800e1000 from each pointer to obtain the correct offset.
   transform(first, last, d_first, bind2nd(minus<u32>(), 0x800e1000));

   u32 *first_ptr = (u32 *)(originalPtr + header.ptr_textdata);
   u32 *last_ptr = (u32 *)(originalPtr + header.ptr_textdata + *first_ptr - 4);

   // find out where the last text block from the old data ends
   u8 *sstart = originalPtr + header.ptr_textdata + *last_ptr;
   u8 *lastBlockEnd = find_if(sstart, originalPtr + header.ptr_eof, bind2nd(equal_to<u8>(), 0x00));
   u32 extraLen = lastBlockEnd - originalPtr - header.ptr_textdata - *last_ptr + 1;

   u32 oldDataLen = *last_ptr + extraLen - *first_ptr;

   // ------------------------------------------
   // rebuild the entire file using the new data
   shared_array<u8> buffer(new u8[originalLen + newDataLen - oldDataLen]);
   u8 *bufferPtr = buffer.get();
   u32 bufferTail = 0;

   // copy the data prior to the text section
   copy(originalPtr, originalPtr + header.ptr_textdata, bufferPtr);
   bufferTail += header.ptr_textdata;

   // update the pointers table
   transform(pointers.begin(), pointers.end(), pointers.begin(), bind2nd(plus<u32>(), pointers.size() * 4));
   copy(pointers.begin(), pointers.end(), (u32 *)(bufferPtr + bufferTail));
   bufferTail += pointers.size() * 4;

   // copy the new encoded text data
   copy(newDataPtr, newDataPtr + newDataLen, bufferPtr + bufferTail);
   bufferTail += newDataLen;

   // copy the remaining data from the file
   u32 cstart = header.ptr_textdata + *first_ptr + oldDataLen;
   copy(originalPtr + cstart, originalPtr + header.ptr_eof, bufferPtr + bufferTail);
   bufferTail += header.ptr_eof - cstart;

   // header needs to be updated only if the new data has a different size
   if (newDataLen != oldDataLen)
   {
      // update the file header
      u32 *hfirst = (u32 *)&header, *hlast = (u32 *)&header + sizeof(header) / 4;
      transform(hfirst + 9, hlast, hfirst + 9, bind2nd(plus<u32>(), newDataLen - oldDataLen));
      transform(hfirst, hlast, hfirst, bind2nd(plus<u32>(), 0x800e1000));

      // copy updared header into proper position
      copy((u8 *)&header, (u8 *)&header + sizeof(header), bufferPtr);
   }

   m_data = make_pair(buffer, bufferTail);
}

void TextInserter::insertIntoLinearData (const filedata_type &scriptData, vector<u16> &pointers, u32 ptrOff)
{
   u8 *originalPtr = m_data.first.get(), *newDataPtr = scriptData.first.get();
   u32 originalLen = m_data.second, newDataLen = scriptData.second;

   boost::shared_array<u8> buffer(new u8[originalLen]);
   u8 *bufferPtr = buffer.get();
   u32 bufferTail = 0;

   // update pointers table
   *((u16 *)bufferPtr) = static_cast<u16>(pointers.size());
   transform(pointers.begin(), pointers.end(), pointers.begin(), bind2nd(plus<u16>(), pointers.size() * 2 + 2));
   copy(pointers.begin(), pointers.end(), (u16 *)(bufferPtr + 2));
   bufferTail += 2 + pointers.size() * 2;

   // copy modified data into place
   copy(newDataPtr, newDataPtr + newDataLen, bufferPtr + bufferTail);
   bufferTail += newDataLen;

   // fill the rest of the file with 0x00
   fill(bufferPtr + bufferTail, bufferPtr + originalLen, 0x00);
   bufferTail = originalLen;
   
   make_pair(buffer, bufferTail);
}

void TextInserter::insertIntoPackedData (const filedata_type &scriptData, vector<u16> &pointers, u32 ptrOff)
{
   u8 *originalPtr = m_data.first.get(), *newDataPtr = scriptData.first.get();
   u32 originalLen = m_data.second, newDataLen = scriptData.second;

   u8 *dataPtr = originalPtr + ptrOff;
   PackedMenuHeader *headerPtr = (PackedMenuHeader *)dataPtr;

   // find out where the original data block ends and its length
   u8 *lastBlockPtr = dataPtr + *max_element(headerPtr->blocks, headerPtr->blocks + 17);
   u8 *lastPtr = lastBlockPtr + *max_element((u16 *)lastBlockPtr + 1, (u16 *)lastBlockPtr + *((u16 *)lastBlockPtr) + 1);
   u8 *lastPtrEnd = find_if(lastPtr, originalPtr + originalLen, bind2nd(equal_to<u8>(), 0x00));
   u32 oldDataLen = distance(dataPtr, lastPtrEnd);

   // each packed block is aligned in a 2048-byte block, therefore there's padding in each one
   int oldDataPadding = calcPadding(oldDataLen, 2048);
   // we are limited to this length, so the new data must fit inside
   u32 maxDataLength = oldDataLen + oldDataPadding;

   // ------------------------------------------
   // rebuild the entire packed block using the modified data
   shared_array<u8> buffer(new u8[maxDataLength + 2048]);
   u8 *bufferPtr = buffer.get();
   u32 bufferTail = 0;

   copy(dataPtr, dataPtr + sizeof(PackedMenuHeader), bufferPtr);
   bufferTail += sizeof(PackedMenuHeader);

   headerPtr = (PackedMenuHeader *)bufferPtr;
   vector<u16>::iterator blockPtrs = pointers.begin();
   
   // iterates through every block ptr in the header
   for (u16 *curBlock = headerPtr->blocks; curBlock != headerPtr->blocks + 17; curBlock++)
   {
      if (*curBlock == 0x0000) continue;
      u16 num_ptr = *(dataPtr + *curBlock);
      u16 newBlockOff = static_cast<u16>(bufferTail);

      // pointers to the current block ptr table
      u16 *beginPtr = (u16 *)(dataPtr + *curBlock);
      u16 *endPtr = (u16 *)(dataPtr + *curBlock) + num_ptr + 1;

      copy(beginPtr, endPtr, (u16 *)(bufferPtr + bufferTail));
      bufferTail += num_ptr * 2 + 2;

      // find out the number of valid pointers
      u16 numValidPtrs = count_if(beginPtr + 1, endPtr, bind2nd(not_equal_to<u16>(), 0x0000));
      u8 *txtBlockBegin = newDataPtr + *blockPtrs;
      u8 *txtBlockEnd = newDataPtr + (blockPtrs + numValidPtrs >= pointers.end() ? newDataLen : *(blockPtrs + numValidPtrs));

      // correct the pointers of this block
      transform(blockPtrs, blockPtrs + numValidPtrs, blockPtrs, bind2nd(plus<u16>(), num_ptr * 2 + 2 - *blockPtrs));
      // correct the pointer to this block in the header
      *curBlock = newBlockOff;

      // update the pointers table, ignoring all the annoying invalid entries
      for (int p = 0, validCount = 0; p < num_ptr; p++)
      {
         u16 *cur = (u16 *)(bufferPtr + *curBlock) + 1 + p;
         if (*cur) *cur = *(blockPtrs + validCount++);
      }

      // copy the modified text data of this block into place
      copy(txtBlockBegin, txtBlockEnd, bufferPtr + bufferTail);
      bufferTail += distance(txtBlockBegin, txtBlockEnd);

      // next block should start in a 4-byte boundary alignment
      int padding = calcPadding(bufferTail, 4);
      fill(bufferPtr + bufferTail, bufferPtr + bufferTail + padding, 0x00);
      bufferTail += padding;
      
      blockPtrs += numValidPtrs;
   }

   if (bufferTail > maxDataLength)
      throw exception("Modified data exceeds maximum block length.");

   // calculates the padding necessary to fill the 2048-byte block
   int newDataPadding = calcPadding(bufferTail, 2048);

   // copy the entire rebuild data into its original place
   copy(bufferPtr, bufferPtr + bufferTail, dataPtr);
   fill(dataPtr + bufferTail, dataPtr + bufferTail + newDataPadding, 0x00);
}

/**
* Inserts a binary block of encoded text data into Refines data.
* @param scriptData Encoded text data to be inserted.
* @param pointers Pointers to each text block inside the encoded text data.
* @param ptrOff Offset of the pointers table.
* @param txtOff Offset of the text data.
*/
void TextInserter::insertIntoRefinesData (const filedata_type &scriptData, vector<u16> &pointers, u32 ptrOff, u32 txtOff)
{
   u8 *originalPtr = m_data.first.get(), *newDataPtr = scriptData.first.get();
   u32 originalLen = m_data.second, newDataLen = scriptData.second;

   // pointers to useful areas
   u8 *pointersPtr = originalPtr + ptrOff;
   u8 *textPtr = originalPtr + txtOff;

   // finds out the begin and end offset of pointers table
   RefinesPointerBlock
      *ptrBegin = (RefinesPointerBlock *)pointersPtr,
      *ptrEnd = find_if(ptrBegin, ptrBegin + 256, logical_not<RefinesPointerBlock>());

   // finds out the length of the original text block
   u8 *lastTxtBlock = textPtr + (ptrEnd - 1)->text_start;
   u8 *textBlockEnd = find_if(lastTxtBlock, originalPtr + originalLen, bind2nd(equal_to<u8>(), 0x00));
   u32 oldDataLen = distance(textPtr, textBlockEnd);

   // each text block is aligned in a 2048-byte block
   int oldDataPadding = calcPadding(oldDataLen, 2048);
   u32 maxDataLength = oldDataLen + oldDataPadding;

   if (newDataLen > maxDataLength)
      throw exception("Modified data exceeds maximum block length.");

   // updates the pointers table with the new values
   for (RefinesPointerBlock *i = ptrBegin; i != ptrEnd; i++)
      i->text_start = pointers.at(distance(ptrBegin, i));

   // copy the modified text data into place
   copy(newDataPtr, newDataPtr + newDataLen, textPtr);

   // fill the padding area with 0x00
   int newDataPadding = calcPadding(newDataLen, 2048);
   fill(textPtr + newDataLen, textPtr + newDataLen + newDataPadding, 0x00);
}

void TextInserter::insertIntoMenuHelpData (vector<filedata_type> &encBlocks, u32 ptrOff, u32 txtOff)
{
   typedef MenuHelpPointer HelpPtr;
   u8 *originalPtr = m_data.first.get();
   u32 originalLen = m_data.second;

   // pointers to useful areas
   u8 *pointersPtr = originalPtr + ptrOff;
   u8 *textPtr = originalPtr + txtOff;

   HelpPtr
      *ptrBegin = (HelpPtr *)pointersPtr,
      *ptrEnd = find_if(ptrBegin, ptrBegin + 128, boost::bind(&HelpPtr::num_block, _1) != ptrBegin->num_block);

   // finds out the length of the original text block
   u8 *lastTxtBlock = textPtr + (ptrEnd - 1)->ptr_text;
   u8 *textBlockEnd = lastTxtBlock + ((MenuHelpTextFlags *)lastTxtBlock)->block_size;
   u32 oldDataLen = distance(textPtr, textBlockEnd);

   // each text block is aligned in a 2048-byte block
   int oldDataPadding = calcPadding(oldDataLen, 2048);
   u32 maxDataLength = oldDataLen + oldDataPadding;

   // ------------------------------------------
   // rebuild the entire text data block using modified data
   shared_array<u8> buffer(new u8[maxDataLength + 2048]);
   u8 *bufferPtr = buffer.get();
   u32 bufferTail = 0;

   vector<HelpPtr> newPointers;
   HelpPtr *pageIterator = ptrBegin;

   for (vector<filedata_type>::iterator i = encBlocks.begin(); i != encBlocks.end(); ++i, ++pageIterator)
   {
      newPointers.push_back(HelpPtr(static_cast<u16>(bufferTail), pageIterator->num_block));

      MenuHelpTextFlags flags = *((MenuHelpTextFlags *)(textPtr + pageIterator->ptr_text));
      flags.block_size = static_cast<u16>(i->second + sizeof(flags));

      // copy the updated flags into place
      copy((u8 *)&flags, (u8 *)&flags + sizeof(flags), bufferPtr + bufferTail);
      bufferTail += sizeof(flags);

      // copy the modified data into place
      copy(i->first.get(), i->first.get() + i->second, bufferPtr + bufferTail);
      bufferTail += i->second;

      // add 4-byte boundary padding
      int padding = calcPadding(bufferTail, 4);
      fill(bufferPtr + bufferTail, bufferPtr + bufferTail + padding, 0x00);
      bufferTail += padding;
   }

   if (bufferTail > maxDataLength)
      throw exception("Modified data exceeds maximum block length.");

   // add padding to align it to 2048-byte boundary
   int newDataPadding = calcPadding(bufferTail, 2048);
   fill(bufferPtr + bufferTail, bufferPtr + bufferTail + newDataPadding, 0x00);
   bufferTail += newDataPadding;

   // update the pointers table and the text data
   copy(newPointers.begin(), newPointers.end(), ptrBegin);
   copy(bufferPtr, bufferPtr + bufferTail, textPtr);
}

void TextInserter::insertIntoMenuBattleData (const filedata_type &scriptData, vector<u16> &pointers, u32 ptrOff, u32 txtOff)
{
   u8 *originalPtr = m_data.first.get(), *newDataPtr = scriptData.first.get();
   u32 originalLen = m_data.second, newDataLen = scriptData.second;

   // pointers to useful areas
   u8 *pointersPtr = originalPtr + ptrOff;
   u8 *textPtr = originalPtr + txtOff;

   // find out where the text offset of this block is located inside the pointers table
   u32 *txttbl = (u32 *)(originalPtr + 0x80);
   u32 *curtbl = find(txttbl, txttbl + 25, txtOff);

   // figure out which pointer description should be used
   int n = static_cast<int> (distance(txttbl, curtbl) + 1);

   u32 headerPtrs[25];
   copy(txttbl, txttbl + 25, headerPtrs);

   // figure out the length of the original data in the current block
   u32 oldDataLen = n < 25 ?
      headerPtrs[n - 1 + 1] - headerPtrs[n - 1] :
      originalLen - headerPtrs[n - 1];

   // since each block is 4-byte aligned, we must pad with 0x00
   int padding = calcPadding(newDataLen, 4);
      
   // ------------------------------------------
   // rebuild the entire file using the new data
   shared_array<u8> buffer(new u8[originalLen + newDataLen + padding - oldDataLen]);
   u8 *bufferPtr = buffer.get();
   u32 bufferTail = 0;

   copy(originalPtr, textPtr, bufferPtr);
   bufferTail += textPtr - originalPtr;

   // update the pointers table
   for (int i = 0, curNewPtr = 0; i < m_ptrdesc[n].m_count; i++)
   {
      u16 *cur = (u16 *)(bufferPtr + ptrOff + i * m_ptrdesc[n].m_width);

      // TODO check if there's a case when group=2 and the second pointer is valid, although the first one ain't
      for (int p = 0; p < m_ptrdesc[n].m_group && *(cur + p) != 0xffff; p++)
         *(cur + p) = pointers[curNewPtr++];
   }

   // copy the new data into place
   copy(newDataPtr, newDataPtr + newDataLen, bufferPtr + bufferTail);
   bufferTail += newDataLen;

   // add padding
   fill(bufferPtr + bufferTail, bufferPtr + bufferTail + padding, 0x00);
   bufferTail += padding;

   // copy the remaining data back into place
   u8 *endPtr = copy(textPtr + oldDataLen, originalPtr + originalLen, bufferPtr + bufferTail);
   bufferTail = endPtr - bufferPtr;

   if (newDataLen + padding != oldDataLen)
   {
      // update header pointers
      int delta = newDataLen + padding - oldDataLen;
      transform(headerPtrs + n, headerPtrs + 25, headerPtrs + n, bind2nd(plus<u32>(), delta));

      // copy the updated header into place
      copy(headerPtrs, headerPtrs + 25, (u32 *)(bufferPtr + 0x80));
   }

   m_data = make_pair(buffer, bufferTail);
}

void TextInserter::insertIntoMainMenuData (const filedata_type &scriptData, vector<u16> &pointers, u32 ptrOff)
{
   // TODO Test this one

   u8 *originalPtr = m_data.first.get(), *newDataPtr = scriptData.first.get();
   u32 originalLen = m_data.second, newDataLen = scriptData.second;
   
   u8 *pointersPtr = originalPtr + ptrOff;
   u8 *textPtr = pointersPtr + 2 + *((u16 *)pointersPtr) * 2;

   u16 *ptrBegin = (u16 *)pointersPtr + 1;
   u16 *ptrEnd = (u16 *)textPtr;

   // this was defined manually, by looking at the available space
   const u32 maxTxtDataLength = 1686;

   if (newDataLen > maxTxtDataLength)
      throw("Modified data exceeds the maximum length");
   
   // update the pointers table, ignoring all the annoying invalid entries
   vector<u16>::iterator nextPtr = pointers.begin();
   for (u16 *cur = ptrBegin; cur != ptrEnd; cur++)
      if (*cur) *cur = *(nextPtr++);

   // update the text data
   copy(newDataPtr, newDataPtr + newDataLen, textPtr);
   fill(textPtr + newDataLen, textPtr + maxTxtDataLength, 0x00);
}

u32 TextInserter::encodeScript (vector<string> &lines, u8 *buffer, vector<u32> &pointers, bool newsessions, bool dtes)
{
   u32 bufferPos = 0;

   // encodes the script content on a block basis
   for (vector<string>::iterator i = lines.begin(); i != lines.end(); ++i)
   {
      pointers.push_back(bufferPos);
      if (i->size()) bufferPos += translateBlock(*i, buffer + bufferPos, newsessions, dtes);
      
      buffer[bufferPos++] = 0x00;
   }

   return bufferPos;
}

/**
* Encodes a given script block into the binary format used in FF8.
* @param block The script block to be encoded.
* @param buffer Buffer where the resulting binary data will be written to.
* @return Number of bytes written to the buffer.
*/
u32 TextInserter::translateBlock (const string &block, u8 *buffer, bool newsessions, bool dtes)
{
   u32 tail = 0;
   vector<string> sessions;

   // splits the script into sections delimited by newsessions (if specified)
   newsessions ?
      boost::split_regex(sessions, block, boost::regex("\\n{3}")) :
      sessions.push_back(block);

   for (vector<string>::iterator i = sessions.begin(); i != sessions.end(); ++i)
   {
      vector<string> subs;
      boost::split(subs, *i, boost::is_any_of("[]"));

      for (vector<string>::iterator j = subs.begin(); j != subs.end(); ++j)
      {
         if (j->size())
         {
            if (j->at(0) == '$')
            {
               string hex = j->substr(1);
               
               if (hex.size() == 2)
                  buffer[tail++] = hexDecode<u8>(hex);
               else
               {
                  u16 value = hexDecode<u16>(hex);
                  buffer[tail++] = value >> 8;
                  buffer[tail++] = value & 0xff;
               }
            }
            else
            {
               for (string::iterator k = j->begin(); k != j->end(); ++k)
               {
                  if (*k == '\n')
                     buffer[tail++] = 0x02;
                  else if (dtes && k + 1 != j->end() && m_tbl.exists<u8>(string(k, k + 2)))
                     buffer[tail++] = m_tbl.find<u8>(string(k, k + 2)), ++k;
                  else
                  {
                     string token(1, *k);

                     if (m_tbl.exists<u8>(token))
                        buffer[tail++] = m_tbl.find<u8>(token);
                     else
                        throw exception("Translation error!");
                  }
               }
            }
         }
      }

      // newsession
      if (i + 1 != sessions.end()) buffer[tail++] = 0x01;
   }

   return tail;
}