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

#ifndef FONTMANAGER_H
#define FONTMANAGER_H

#include <SDL.h>
#include "Font.h"

#include <misc/memory.h>
#include <string>

typedef enum {
	FONT_STD10,
	FONT_STD12,
	FONT_STD24,
	NUM_FONTS
} Fonts_enum;

/// A class for managing fonts.
/**
	This class manages all fonts used in Dune Legacy and provides methods for rendering texts with a specific font.
*/
class FontManager
{
public:
	FontManager();
	~FontManager();

	void drawTextOnSurface(SDL_Surface* pSurface, std::string text, unsigned char color, unsigned int fontNum);
	int	getTextWidth(std::string text, unsigned int fontNum);
	int getTextHeight(unsigned int fontNum);
	SDL_Surface* createSurfaceWithText(std::string text, unsigned char color, unsigned int fontNum);
	SDL_Surface* createSurfaceWithMultilineText(std::string text, unsigned char color, unsigned int fontNum, bool bCentered = false);
private:
	std::shared_ptr<Font> fonts[NUM_FONTS];

};

#endif // FONTMANAGER_H
