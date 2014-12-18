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

#ifndef SHPFILE_H
#define SHPFILE_H

#include <SDL.h>
#include <SDL_rwops.h>
#include <stdarg.h>
#include <vector>

#include "Animation.h"

#define	TILE_NORMAL		0x00010000
#define	TILE_FLIPH		0x00100000
#define	TILE_FLIPV		0x01000000
#define TILE_ROTATE		0x10000000

#define TILE_GETINDEX(x)	(x & 0x0000FFFF)
#define TILE_GETTYPE(x)		(x & 0xFFFF0000)

/// A class for loading a *.SHP-File.
/**
	This class can read shp-Files and return the contained pictures as a SDL_Surface.
*/
class Shpfile
{
private:
	// Internal structure used for an index of contained files
	struct ShpfileEntry
	{
		Uint32 startOffset;
		Uint32 endOffset;
	};

public:
	Shpfile(SDL_RWops* rwop, int freesrc);
	virtual ~Shpfile();

	SDL_Surface* getPicture(Uint32 indexOfFile);
	SDL_Surface* getPictureArray(unsigned int tilesX, unsigned int tilesY, ...);
	Animation* getAnimation(unsigned int startindex, unsigned int endindex, bool bDoublePic=true, bool bSetColorKey=true, bool bLoopRewindBackwards=false);

	/// Returns the number of contained pictures
	/**
		Returns the number of pictures in this SHP-File.
		\return	Number of pictures in this SHP-File.
	*/
	inline int getNumFiles() const { return (int) shpfileEntries.size(); };

private:
	void readIndex();
	void shpCorrectLF(unsigned char *in, unsigned char *out, int size);
	void applyPalOffsets(unsigned char *offsets, unsigned char *data,unsigned int length);

	std::vector<ShpfileEntry> shpfileEntries;
	unsigned char* pFiledata;
	Uint32 shpFilesize;
};


#endif //SHPFILE_H
