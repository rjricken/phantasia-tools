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

#ifndef DATASTRUCTURE_HPP
#define DATASTRUCTURE_HPP

#include "common.hpp"

/**
* Defines the data found in the Field Battle (.dat) files.
*/
typedef struct tagFF8FieldBattleHeader
{
   u32 ptr_skeleton;     /**< Skeleton data                 */
   u32 ptr_geometry;     /**< 3d geometry                   */
   u32 ptr_animation;    /**< Animations data               */
   u32 ptr_section3;     /**< Unknown data                  */
   u32 ptr_section4;     /**< Unknown data                  */
   u32 ptr_section5;     /**< unknown data                  */
   u32 ptr_monterinfo;   /**< Information about the monster */
   u32 ptr_battlescript; /**< Contains the dialogues        */
   u32 ptr_soundsec1;    /**< Appears to be sound data      */
   u32 ptr_soundsec2;    /**< Appears to be sound data      */
   u32 ptr_texture;      /**< Textures                      */
   u32 ptr_eof;          /**< End of file                   */
}
FieldBattleHeader;

typedef struct tagFF8FieldBattleTextSectionHeader
{
   u32 num_pointers; // always 0x03
   u32 unknown_data; // unknown, always 0x10 (?)
   u32 text_ptrtbl;  // Address of text pointers table
   u32 text_data;    // text data
}
FieldBattleTextSectionHeader;

typedef struct tagFF8FieldDialogsHeader
{
   u32 ptr_section0;
   u32 ptr_section1;
   u32 ptr_section2;
   u32 ptr_section3;
   u32 ptr_section4;
   u32 ptr_section5;
   u32 ptr_section6;
   u32 ptr_section7;
   u32 ptr_textdata;
   u32 ptr_section9;
   u32 ptr_sectiona;
   u32 ptr_eof;
}
FieldDialogsHeader;

typedef struct tagFF8RefinesPointerBlock
{
   bool operator! () const {
      return (!unknown1 && !unknown2 && !unknown3);
   }

   u16 text_start; // points to a text block
   u16 unknown1;   // maybe some type flag
   u16 unknown2;   // unknown
   u16 unknown3;   // unknown
}
RefinesPointerBlock;

typedef struct tagFF8PackedMenuHeader
{
   u16 id;           // always 0x0010
   u16 blocks[17];   // pointer to each sub block
}
PackedMenuHeader;

typedef struct tagFF8MenuHelpPointer
{
   tagFF8MenuHelpPointer(u16 txtptr = 0, u16 blockptr = 0) :
      ptr_text(txtptr), num_block(blockptr) { }

   u16 ptr_text;
   u16 num_block;
}
MenuHelpPointer;

typedef struct FF8MenuHelpTextFlags
{
   u16 level;
   u16 pagination1;
   u16 pagination2;
   u16 block_size;
}
MenuHelpTextFlags;

#endif //~DATASTRUCTURE_HPP