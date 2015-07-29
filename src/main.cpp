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

#define BOOST_FILESYSTEM_VERSION 3

#include <iostream>
#include <fstream>
#include <iomanip>
#include <exception>
#include <boost/regex.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time.hpp>
#include "common.hpp"
#include "lzsdecoder.hpp"
#include "dictionary.hpp"
#include "extractinfo.hpp"
#include "insertinfo.hpp"
#include "file_extractor.hpp"
#include "text_dumper.hpp"
#include "text_inserter.hpp"

using namespace std;
using namespace boost::filesystem;
using namespace boost::posix_time;
using boost::format;
using boost::lexical_cast;

int main ()
{
   cout <<                                                endl
        << "Phantasia Tools                    v0.8"   << endl
        <<                                                endl
        << "Final Fantasy VIII (R) Datahacking Tools"  << endl
        << "2005-2011 by Ricardo J. Ricken (Darkl0rd)" << endl
        <<                                                endl;

   try
   {
      string userInput;
      cout << "1. Final Fantasy VIII Disc 1"         << endl
           << "2. Final Fantasy VIII Disc 2"         << endl
           << "3. Final Fantasy VIII Disc 3"         << endl
           << "4. Final Fantasy VIII Disc 4"         << endl
           << "   Choose: ";

      getline(cin, userInput), cout << endl;
      int discNum = lexical_cast<int>(userInput);

      if (!(discNum >= 1 && discNum <= 4))
         throw exception("Final Fantasy 8 has no such disc.");

      cout << "1. Extract from disc and dump into script files" << endl
           << "2. Rebuild files using modified script files"    << endl
           << "3. Insert modified files back into IMG file"     << endl
           << "   Pick one: ";

      getline(cin, userInput), cout << endl;
      int option = lexical_cast<int>(userInput);

      if (!(option >= 1 && option <= 3))
         throw exception("There's no such option.");

      Dictionary dic;
      dic.loadFromFile("ff8complete.tbl");

      switch (option)
      {
         enum { Extract = 1, Rebuild, Insert };

         //============================================================================================
         // Extract from disc and dump into script files
         //============================================================================================
         case Extract:
         {
            FF8ExtractInfo extInfo;
            extInfo.loadFromFile("extractinfo.xml", discNum);

            FileExtractor extractor(extInfo, "fieldbattle.tbl");
            u32 idxEnd = extractor.loadMainIndex();

            path folder = "Disc" + lexical_cast<string>(discNum);
            create_directories(folder / "Other" / "Original");
            create_directories(folder / "Other" / "Modified");
            create_directories(folder / "Other" / "Original" / "Script");
            create_directories(folder / "Other" / "Modified" / "Script");

            // this will collect information to be used to insert those files back
            FF8InserterInfo insInfo(discNum, extInfo.imgName());
            insInfo.setIndexInfo(extInfo.indexOffset(), idxEnd);

            for (FF8ExtractInfo::iterator i = extInfo.begin(); i != extInfo.end(); ++i)
            {
               FF8FileInfo &cur = i->second;

               // for now, only some files will be extracted
               if (!cur.hasSubIndex() && !cur.hasTextData() && cur.nameHint().empty()) continue;

               format fmt("%1$03d%2%");
               fmt % cur.id() % (cur.nameHint().empty() ? "" : "-" + cur.nameHint());

               path filePath;
               filePath /= folder / "Other" / "Original" / fmt.str();
               filePath.replace_extension("." + cur.ext());

               cout << "Extracting " << filePath.filename() << endl;
               cout << "Info: " << cur.comment() << endl << endl;

               FileExtractor::filedata_type fileData = extractor.extractFile(cur.id());

               ofstream file(filePath.native(), ios::binary);
               if (!file) throw exception(("Unable to create " + filePath.filename().string()).c_str());

               file.exceptions(ios_base::badbit);
               file.write((char *)fileData.first.get(), fileData.second);

               // collect information about the file which just got extracted
               insInfo["Other"].add(cur.id(), filePath.filename().string(), cur.offset(), cur.length());

               // ------------------------------------------------------------------------------
               // dump all the text data in the current file into scripts
               if (cur.hasTextData())
               {
                  TextDumper dumper(fileData, dic);

                  for (FF8FileInfo::txtdata_iterator j = cur.begin(); j != cur.end(); ++j)
                  {
                     FF8FileInfo::txtdata_iterator::difference_type n = distance(cur.begin(), j);
                    
                     format fmt("%1$03d_%2$02d-%3%");
                     fmt % cur.id() % n % j->m_nameHint;

                     string nameHint = fmt.str();
                     boost::to_lower(nameHint);
                     boost::erase_all(nameHint, " ");

                     path scriptPath;
                     scriptPath /= filePath.parent_path() / "Script" / nameHint;
                     scriptPath.replace_extension(".txt");

                     try 
                     {
                        cout << " Dumping to " << scriptPath.filename() << endl;

                        string script;
                        dumper.dump(script, j->m_format, j->m_ptrOff, j->m_txtOff);

                        ofstream scriptFile(scriptPath.native());
                        if (!scriptFile) throw exception(("Unable to create " + scriptPath.filename().string()).c_str());

                        scriptFile << script;

                        // collect information about the script that was just dumped
                        insInfo["Other"][filePath.filename().string()].add(
                           scriptPath.filename().string(), j->m_format, j->m_ptrOff, j->m_txtOff);
                     }
                     catch (const exception &e) {
                        cout << " Error: " << e.what() << endl;
                     }
                  }

                  cout << endl;
               }

               // ------------------------------------------------------------------------------
               // when a file containing  sub-index is identified
               // this is the case for field and battle files.
               if (cur.hasSubIndex() && false)
               {
                  const string &curFolder = cur.subIndex().m_folder;
                  cout << "Extracting files from sub-index" << endl;

                  // load the subindex into file extractor (field index is lzs-compressed)
                  if (cur.ext() == "lzs")
                  {
                     u8 *lzsDataPtr = fileData.first.get();
                     u32 *lzsLen = (u32 *)lzsDataPtr;

                     LZSDecoder decoder(lzsDataPtr + 4, *lzsLen);
                     LZSDecoder::filedata_type decData = decoder.decode();

                     extractor.loadSubIndex(cur.id(), decData);
                  }
                  else extractor.loadSubIndex(cur.id(), fileData);

                  // collect information about this folder
                  insInfo.add(curFolder, filePath.filename().string(), cur.subIndex().m_startOff, cur.subIndex().m_endOff);

                  // DiscX/Field|Battle/Original|Modified and /Original|Modified/Script
                  create_directories(folder / curFolder / "Original");
                  create_directories(folder / curFolder / "Modified");
                  create_directories(folder / curFolder / "Original" / "Script");
                  create_directories(folder / curFolder / "Modified" / "Script");

                  string nameHint;
                  FileExtractor::filedata_type result;
                  int type;

                  if (curFolder == "Field") type = FileExtractor::Field;
                  else if (curFolder == "Battle") type = FileExtractor::Battle;
                  else throw exception("Invalid folder type found in current sub-index");

                  // list used to filter test files no longer acessible in the game.
                  map<u8, string> ignoreList;
                  ignoreList[1] = "(testno|start0?|gover|test[\\d]*$)";
                  ignoreList[2] = "";
                  ignoreList[3] = "";
                  ignoreList[4] = "";

                  // extract files one by one, till there's no more files left
                  while (extractor.extractNextSubFile(result, nameHint, type))
                  {
                     // test files used during the development of game, no longe acessible during gameplay.
                     // if still there's some way to access them, I really don't care.
                     boost::regex re(ignoreList[extInfo.discNum()]);
                     if (boost::regex_match(nameHint, re)) continue;

                     path subPath;
                     subPath /= folder / curFolder / "Original" / nameHint;
                     subPath.replace_extension("." + cur.subIndex().m_ext);

                     cout << " " << subPath.filename() << endl;

                     ofstream subFile(subPath.native(), ios::binary);
                     if (!subFile) throw exception(("Unable to create " + subPath.filename().string()).c_str());

                     subFile.write((char *)result.first.get(), result.second);

                     // collect information about the file just extracted
                     FileExtractor::fileinfo_type info = extractor.getLasInfo();
                     insInfo[curFolder].add(extractor.getLastId(), subPath.filename().string(), info.first, info.second);

                     // dump text data into script file, when possible.
                     try
                     {
                        path scriptPath = subPath.parent_path() / "Script" / subPath.filename();
                        scriptPath.replace_extension(".txt");

                        if (curFolder == "Field")
                        {
                           u8 *lzsDataPtr = result.first.get();
                           u32 *lzsLen = (u32 *)lzsDataPtr;

                           LZSDecoder decoder(lzsDataPtr + 4, *lzsLen);
                           LZSDecoder::filedata_type decData = decoder.decode();

                           TextDumper dumper(decData, dic);
                           cout << " Dumping to " << scriptPath.filename() << endl;

                           string script;
                           dumper.dump(script, TextDumper::Field);

                           ofstream scriptFile(scriptPath.native());
                           if (!scriptFile) throw exception(("Unable to create " + scriptPath.filename().string()).c_str());

                           scriptFile << script;
                        }
                        else if (curFolder == "Battle")
                        {
                           TextDumper dumper(result, dic);
                           cout << " Dumping to " << scriptPath.filename() << endl;

                           string script;
                           dumper.dump(script, TextDumper::Battle);

                           ofstream scriptFile(scriptPath.native());
                           if (!scriptFile) throw exception(("Unable to create " + scriptPath.filename().string()).c_str());

                           scriptFile << script;
                        }
                        else throw exception("Invalid folder type found in current sub-index");

                        // collect information about the script that was just dumped
                        insInfo[curFolder][subPath.filename().string()].add(scriptPath.filename().string());
                     }
                     catch (const exception &e) {
                        cout << " Error: " << e.what() << endl;
                     }
                  }

                  cout << endl;
               }
            }

            string xmlFile = folder.string() + ".xml";
            boost::to_lower(xmlFile);

            insInfo.saveToFile(xmlFile);
            cout << "Complete. Information saved into " << xmlFile << endl;
         }
         break;

         //============================================================================================
         // Rebuild files using modified script files
         //============================================================================================
         case Rebuild:
         {
            path folder = "Disc" + lexical_cast<string>(discNum);
            
            string xmlFile = folder.string() + ".xml";
            boost::to_lower(xmlFile);

            FF8InserterInfo info;
            info.loadFromFile(xmlFile);

            for (FF8InserterInfo::folder_iterator i = info.begin(); i != info.end(); ++i)
            {
               FF8InserterFolder curFolder = i->second;

               for (FF8InserterFolder::file_iterator j = curFolder.begin(); j != curFolder.end(); ++j)
               {
                  try
                  {
                     FF8InserterFile curFile = j->second;
                     path filePath = folder / curFolder.name() / "Original" / curFile.name();
                  
                     // ------------------------------------------------------------------------------
                     // menu files
                     if (curFolder.name() == "Other")
                     {
                        //vector<FF8InserterScript> scripts;
                        map<path, FF8InserterScript> scripts;

                        // collect the scripts that should be inserted back to its original file
                        for (FF8InserterFile::script_iterator k = curFile.begin(); k != curFile.end(); ++k)
                        {
                           path scriptPath = folder / curFolder.name() / "Modified" / "Script" / k->m_name;
                           if (!exists(scriptPath)) continue;

                           ptime scriptVersion = from_time_t(last_write_time(scriptPath));

                           if (k->m_version.is_not_a_date_time() || scriptVersion > k->m_version)
                              scripts[scriptPath] = *k;
                        }

                        // none of the scripts need to be inserted
                        if (scripts.empty()) continue;
                     
                        ifstream originalFile(filePath.native(), ios::binary);
                        if (!originalFile) throw exception(("Unable to open " + filePath.filename().string()).c_str());

                        cout << "Rebuilding " << filePath.filename() << endl;

                        uintmax_t originalLen = file_size(filePath);

                        // buffer where each script will be inserted to
                        boost::shared_array<u8> buffer(new u8[originalLen]);
                        u32 bufferLen = static_cast<u32>(originalLen);

                        // read original file data into buffer
                        originalFile.read((char *)buffer.get(), originalLen);
                        TextInserter inserter(make_pair(buffer, bufferLen), dic);

                        // used to correct text offsets that were based on original file data
                        int delta = 0;
                     
                        for (map<path, FF8InserterScript>::iterator s = scripts.begin(); s != scripts.end(); ++s)
                        {
                           try
                           {
                              cout << " Inserting " << s->first.filename() << endl;
                              FF8InserterScript cur = s->second;

                              ifstream scriptFile(s->first.native());
                              if (!scriptFile) throw exception(("Unable to open " + s->first.filename().string()).c_str());

                              // read script content into string
                              string script((istreambuf_iterator<char>(scriptFile)), istreambuf_iterator<char>());
                              inserter.insert(script, cur.m_format, cur.m_ptrOffset, cur.m_textOffset + delta);

                              delta = inserter.getModifiedFile().second - originalLen;
                           }
                           catch (const exception &e) {
                              cout << " Error: " << e.what() << endl;
                           }
                        }

                        // get the modified file
                        TextInserter::filedata_type result = inserter.getModifiedFile();
          
                        path modifiedPath = folder / curFolder.name() / "Modified" / filePath.filename();

                        ofstream modifiedFile(modifiedPath.native(), ios::binary);
                        if (!modifiedFile) throw exception(("Unable to create " + modifiedPath.filename().string()).c_str());

                        modifiedFile.write((char *)result.first.get(), result.second);
                        cout << endl;
                     }

                     // ------------------------------------------------------------------------------
                     // field and battle files
                     else /* TODO DELETE ME */if (false)
                     {
                        FF8InserterFile::script_iterator curScript = curFile.begin();
                        if (curScript == curFile.end()) continue;

                        path scriptPath = folder / curFolder.name() / "Modified" / "Script" / curScript->m_name;
                        if (!exists(scriptPath)) continue;

                        ptime scriptVersion = from_time_t(last_write_time(scriptPath));

                        // only insert the script back to its original file if it have not been
                        // inserted yet or if it's different from the last one inserted.
                        if (curScript->m_version.is_not_a_date_time() || scriptVersion > curScript->m_version)
                        {
                           try
                           {
                              ifstream originalFile(filePath.native(), ios::binary);
                              if (!originalFile) throw exception(("Unable to open " + filePath.filename().string()).c_str());

                              // read original file data into buffer
                              uintmax_t originalLen = file_size(filePath);
                              boost::shared_array<u8> originalData(new u8[originalLen]);
                              originalFile.read((char *)originalData.get(), originalLen);

                              cout << "Inserting " << scriptPath.filename() << " back into " << filePath.filename() << endl;

                              ifstream scriptFile(scriptPath.native());
                              if (!scriptFile) throw exception(("Unable to open " + scriptPath.filename().string()).c_str());

                              // read script content into string
                              string script((istreambuf_iterator<char>(scriptFile)), istreambuf_iterator<char>());

                              if (curFolder.name() == "Battle")
                              {
                                 TextInserter inserter(make_pair(originalData, originalLen), dic);

                                 inserter.insert(script, TextInserter::Battle);
                                 TextInserter::filedata_type result = inserter.getModifiedFile();

                                 path modifiedPath = folder / curFolder.name() / "Modified" / filePath.filename();

                                 ofstream modifiedFile(modifiedPath.native(), ios::binary);
                                 if (!modifiedFile) throw exception(("Unable to create " + modifiedPath.filename().string()).c_str());

                                 modifiedFile.write((char *)result.first.get(), result.second);
                              }
                              else if (curFolder.name() == "Field")
                              {
                                 u8 *lzsDataPtr = originalData.get();
                                 u32 *lzsLen = (u32 *)lzsDataPtr;

                                 LZSDecoder decoder(lzsDataPtr + 4, *lzsLen);
                                 LZSDecoder::filedata_type decData = decoder.decode();

                                 TextInserter inserter(decData, dic);

                                 inserter.insert(script, TextInserter::Field);
                                 TextInserter::filedata_type result = inserter.getModifiedFile();

                                 // TODO compress and store
                              }
                              else throw exception("Invalid folder type found.");

                           }
                           catch (const exception &e) {
                              cout << " Error: " << e.what() << endl;
                           }
                        }
                     }
                  }
                  catch (const exception &e) {
                     cout << "Error: " << e.what() << endl;
                  }
               }
            }
         }
         break;

         case Insert:
         {

         }
         break;
      }
   }
   catch (const boost::bad_lexical_cast &e)
   {
      cout << "Error: Failed to convert input string into " << e.target_type().name() << endl;
      cout << "Details: " << e.what() << endl;
   }
   catch (const exception &e) {
      cout << "Error: " << e.what() << endl;
   }
   
   cin.get();
   return 0;
}