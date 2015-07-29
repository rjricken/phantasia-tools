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

#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>
#include <functional>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

typedef unsigned char  u8;  /**< 8-bit unsigned value  */
typedef unsigned short u16; /**< 16-bit unsigned value */
typedef unsigned long  u32; /**< 32-bit unsigned value */

/**
* Encodes a specified value into a readable text string in hexadecimal notation.
* @param value The value to be encoded.
* @return An hexstring containing the encoded value.
*/
template <class T> std::string hexEncode (T value) {
   std::string fmt = "$%1$0" + boost::lexical_cast<string>(sizeof(T) * 2) + "X";
   return (boost::format(fmt) % int(value)).str();
}

/**
* Decodes a specified hexstring into its equivalent numeric representation.
* @param value Value to be decoded.
* @return The decoded value.
*/
template <class T> T hexDecode (std::string value) {
   return static_cast<T> (std::strtoul(value.c_str(), 0, 16));
}

/**
* Function object that applies the static_cast C++ casting operator.
*/
template <class Source, class Result>
struct s_cast : public std::unary_function<Source, Result>
{
   Result operator() (const Source &arg) const {
      return static_cast<Result>(arg);
   }
};

#endif //~COMMON_HPP