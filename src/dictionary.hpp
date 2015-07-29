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

#ifndef DICTIONARY_HPP
#define DICTIONARY_HPP

#include <string>
#include <boost/bimap.hpp>

#include "common.hpp"

/**
* Implements a dictionary-like functionality matching 8 and 16-bit data
* to textual values.
*/
class Dictionary
{
public:
   Dictionary() { }

   typedef boost::bimap <u8, std::string> dic_type8b;
   typedef boost::bimap <u16, std::string> dic_type16b;
   typedef boost::bimap <u8, std::string>::key_type dic_entry8b;
   typedef boost::bimap <u16, std::string>::key_type dic_entry16b;

   /**
   * Loads the dictionary data from a Thingy table file.
   * @param file Path to the table file
   */
   void loadFromFile (const std::string &file);

   /**
   * Stores the dictionary data into a Thingy table file.
   * @param file Path to the table file
   */
   void saveToFile (const std::string &file);

   /**
   * Inserts a new entry in the dictionary.
   * @param key New entry's key
   * @param value New entry's matching value
   */
   template <typename T> void insert (const T key, const std::string value);

   /**
   * Finds the value of a given key in the dictionary.
   * @param key The target key
   * @return If found, the matching value. Otherwise, an empty string.
   */
   template <typename T> std::string find (const T key) const;

   /**
   * Finds the key matching a given value.
   * @param value The target value
   * @return If found, the matching value. Otherwise, returns 0.
   */
   template <typename T> T find (const std::string &value) const;

   /**
   * Checks wether a given value existis in the dictionary.
   * @param value The value to be checked
   * @return True if it exists, false otherwise.
   */
   template <typename T> bool exists (const std::string &value) const;

private:
   dic_type8b m_8b;   /**< Bimap containing 8-bit entries */
   dic_type16b m_16b; /**< Bimap containing 16-bit entries */
};

#endif //~DICTIONARY_HPP