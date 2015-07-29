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

#include "dictionary.hpp"

#include <fstream>
#include <iomanip>
#include <exception>
#include <boost/regex.hpp>

using namespace std;

template <> void Dictionary::insert<u8> (const u8 key, const string value) {
   m_8b.insert(dic_entry8b(key, value));
}

template <> void Dictionary::insert<u16> (const u16 key, const string value) {
   m_16b.insert(dic_entry16b(key, value));
}

template <> string Dictionary::find<u8> (const u8 key) const {
   return m_8b.left.count(key) ? m_8b.left.at(key) : "";
}

template <> string Dictionary::find<u16> (const u16 key) const {
    return m_16b.left.count(key) ? m_16b.left.at(key) : "";
}

template <> u8 Dictionary::find<u8> (const string &value) const {
   return m_8b.right.count(value) ? m_8b.right.at(value) : 0;
}

template <> u16 Dictionary::find<u16> (const string &value) const {
   return m_16b.right.count(value) ? m_16b.right.at(value) : 0;
}

template <> bool Dictionary::exists<u8> (const string &value) const {
   return m_8b.right.count(value) ? true : false;
}

template <> bool Dictionary::exists<u16> (const string &value) const {
   return m_16b.right.count(value) ? true : false;
}

void Dictionary::loadFromFile(const string &file)
{
   ifstream tbl(file);
   if (!tbl) throw exception(("Failed to open " + file).c_str());

   while (!tbl.eof())
   {
      string line;
      getline(tbl, line);

      // matches HH=* or HHHH=* pattern
      const boost::regex e("^([\\dABCDEFabcdef]{2}|[\\dABCDEFabcdef]{4})=(.+)");
      boost::smatch what;

      if (boost::regex_match(line, what, e))
      {
         string key = what[1];
         u16 tmp = static_cast<u16> (strtoul(key.c_str(), 0, 16));

         if (key.size() == 2) m_8b.insert(dic_entry8b(static_cast <u8> (tmp & 0xff), what[2]));
         else if (key.size() == 4) m_16b.insert(dic_entry16b(tmp, what[2]));
      }
   }
}

void Dictionary::saveToFile (const string &file)
{
   ofstream tbl(file);
   if (!tbl) throw exception(("Failed to create " + file).c_str());

   for (dic_type8b::iterator i = m_8b.begin(); i != m_8b.end(); ++i)
      tbl << hex << uppercase << setw(2) << setfill('0')
          << (short)i->left << "=" << i->right << endl;

   for (dic_type16b::iterator i = m_16b.begin(); i != m_16b.end(); ++i)
      tbl << hex << uppercase << setw(4) << setfill('0')
          << i->left << "=" << i->right << endl;
}