/*
 *  This file is part of Dune Legacy.
 *
 *  Dune Legacy is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Dune Legacy is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Dune Legacy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <FileClasses/FileManager.h>

#include <globals.h>

#include <FileClasses/TextManager.h>

#include <misc/FileSystem.h>

#include <config.h>
#include <misc/fnkdat.h>
#include <misc/md5.h>
#include <misc/string_util.h>

#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iomanip>

FileManager::FileManager(bool saveMode) {
    fprintf(stderr,"\n");
    fprintf(stderr,"FileManager is loading PAK-Files...\n\n");
    fprintf(stderr,"MD5-Checksum                      Filename\n");

    std::vector<std::string> searchPath = getSearchPath();
    std::vector<std::string> FileList = getNeededFiles();

    std::vector<std::string>::const_iterator filenameIter;
    for(filenameIter = FileList.begin(); filenameIter != FileList.end(); ++filenameIter) {

        std::vector<std::string>::const_iterator searchPathIter;
        for(searchPathIter = searchPath.begin(); searchPathIter != searchPath.end(); ++searchPathIter) {
            std::string filepath = *searchPathIter + "/" + *filenameIter;
            if(getCaseInsensitiveFilename(filepath) == true) {
                try {
                    fprintf(stderr,"%s  %s\n", md5FromFilename(filepath).c_str(), filepath.c_str());
                    pakFiles.push_back(new Pakfile(filepath));
                } catch (std::exception &e) {
                    if(saveMode == false) {
                        while(pakFiles.empty()) {
                            delete pakFiles.back();
                            pakFiles.pop_back();
                        }

                        throw std::runtime_error("FileManager::FileManager(): Error while opening " + filepath + ": " + e.what());
                    }
                }

                // break out of searchPath-loop because we have opened the file in one directory
                break;
            }
        }

    }

    fprintf(stderr,"\n");
}

FileManager::~FileManager() {
    std::vector<Pakfile*>::const_iterator iter;
    for(iter = pakFiles.begin(); iter != pakFiles.end(); ++iter) {
        delete *iter;
    }
}

std::vector<std::string> FileManager::getSearchPath() {
    std::vector<std::string> searchPath;

    searchPath.push_back(DUNELEGACY_DATADIR);
    char tmp[FILENAME_MAX];
	fnkdat("data", tmp, FILENAME_MAX, FNKDAT_USER | FNKDAT_CREAT);
    searchPath.push_back(tmp);

    return searchPath;
}

std::vector<std::string> FileManager::getNeededFiles() {
    std::vector<std::string> fileList;

    fileList.push_back("LEGACY.PAK");
    fileList.push_back("OPENSD2.PAK");
    fileList.push_back("DUNE.PAK");
    fileList.push_back("SCENARIO.PAK");
    fileList.push_back("MENTAT.PAK");
    fileList.push_back("VOC.PAK");
    fileList.push_back("MERC.PAK");
    fileList.push_back("FINALE.PAK");
    fileList.push_back("INTRO.PAK");
    fileList.push_back("INTROVOC.PAK");
    fileList.push_back("SOUND.PAK");

    std::string LanguagePakFiles = (pTextManager != NULL) ? _("LanguagePakFiles") : "";

    if(LanguagePakFiles.empty()) {
        LanguagePakFiles = "ENGLISH.PAK,HARK.PAK,ATRE.PAK,ORDOS.PAK";
    }

    std::vector<std::string> additionalPakFiles = splitString(LanguagePakFiles);
    std::vector<std::string>::iterator iter;
    for(iter = additionalPakFiles.begin(); iter != additionalPakFiles.end(); ++iter) {
        fileList.push_back(*iter);
    }

    std::sort(fileList.begin(), fileList.end());

    return fileList;
}

std::vector<std::string> FileManager::getMissingFiles() {
    std::vector<std::string> MissingFiles;
    std::vector<std::string> searchPath = getSearchPath();
    std::vector<std::string> FileList = getNeededFiles();

    std::vector<std::string>::const_iterator filenameIter;
    for(filenameIter = FileList.begin(); filenameIter != FileList.end(); ++filenameIter) {
        bool bFound = false;

        std::vector<std::string>::const_iterator searchPathIter;
        for(searchPathIter = searchPath.begin(); searchPathIter != searchPath.end(); ++searchPathIter) {
            std::string filepath = *searchPathIter + "/" + *filenameIter;
            if(getCaseInsensitiveFilename(filepath) == true) {
                bFound = true;
                break;
            }
        }

        if(bFound == false) {
            MissingFiles.push_back(*filenameIter);
        }
    }

    return MissingFiles;
}

SDL_RWops* FileManager::openFile(std::string filename) {
	SDL_RWops* ret;

    // try loading external file
    std::vector<std::string> searchPath = getSearchPath();
    std::vector<std::string>::const_iterator searchPathIter;
    for(searchPathIter = searchPath.begin(); searchPathIter != searchPath.end(); ++searchPathIter) {

        std::string externalFilename = *searchPathIter + "/" + filename;
        if(getCaseInsensitiveFilename(externalFilename) == true) {
            if((ret = SDL_RWFromFile(externalFilename.c_str(), "rb")) != NULL) {
                return ret;
            }
        }
    }

    // now try loading from pak file
    std::vector<Pakfile*>::const_iterator iter;
    for(iter = pakFiles.begin(); iter != pakFiles.end(); ++iter) {
        ret = (*iter)->openFile(filename);
		if(ret != NULL) {
			return ret;
		}
    }

    throw std::runtime_error("FileManager::OpenFile(): Cannot find " + filename + "!");
}

bool FileManager::exists(std::string filename) const {

    // try finding external file
    std::vector<std::string> searchPath = getSearchPath();
    std::vector<std::string>::const_iterator searchPathIter;
    for(searchPathIter = searchPath.begin(); searchPathIter != searchPath.end(); ++searchPathIter) {

        std::string externalFilename = *searchPathIter + "/" + filename;
        if(getCaseInsensitiveFilename(externalFilename) == true) {
            return true;
        }
    }

    // now try finding in one pak file
    std::vector<Pakfile*>::const_iterator iter;
    for(iter = pakFiles.begin(); iter != pakFiles.end(); ++iter) {
        if((*iter)->exists(filename) == true) {
            return true;
        }
    }

    return false;
}


std::string FileManager::md5FromFilename(std::string filename) {
	unsigned char md5sum[16];

	if(md5_file(filename.c_str(), md5sum) != 0) {
		throw std::runtime_error("Cannot open or read " + filename + "!");
	} else {

		std::stringstream stream;
		stream << std::setfill('0') << std::hex;
		for(int i=0;i<16;i++) {
			stream << std::setw(2) << (int) md5sum[i];
		}
		return stream.str();
	}
}
