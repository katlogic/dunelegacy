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

#include <GUI/dune/DuneStyle.h>
#include <misc/draw_util.h>
#include <GUI/Widget.h>

#include <FileClasses/GFXManager.h>
#include <FileClasses/FontManager.h>
#include <FileClasses/Palette.h>

#include <iostream>
#include <cmath>
#include <algorithm>

extern Palette      palette;
extern GFXManager*  pGFXManager;
extern FontManager* pFontManager;


SDL_Surface* DuneStyle::createSurfaceWithText(const char* text, unsigned char color, unsigned int fontsize) {
	if(pFontManager != NULL) {
		return pFontManager->createSurfaceWithText(text, color, fontsize);
	} else {
		SDL_Surface* surface;

		// create surfaces
		if((surface = SDL_CreateRGBSurface(SDL_HWSURFACE,strlen(text)*10,12,8,0,0,0,0))== NULL) {
			return NULL;
		}
		palette.applyToSurface(surface);

		return surface;
	}
}

unsigned int DuneStyle::getTextHeight(unsigned int FontNum) {
	if(pFontManager != NULL) {
		return pFontManager->getTextHeight(FontNum);
	} else {
		return 12;
	}
}

unsigned int DuneStyle::getTextWidth(const char* text, unsigned int FontNum)  {
	if(pFontManager != NULL) {
		return pFontManager->getTextWidth(text,FontNum);
	} else {
		return strlen(text)*10;
	}
}


Point DuneStyle::getMinimumLabelSize(std::string text, int fontID) {
	return Point(getTextWidth(text.c_str(),fontID) + 12,getTextHeight(fontID) + 4);
}

SDL_Surface* DuneStyle::createLabelSurface(Uint32 width, Uint32 height, std::list<std::string> textLines, int fontID, Alignment_Enum alignment, int textcolor, int textshadowcolor, int backgroundcolor) {
	SDL_Surface* surface;

	// create surfaces
	if((surface = SDL_CreateRGBSurface(SDL_HWSURFACE,width,height,8,0,0,0,0))== NULL) {
		return NULL;
	}
	palette.applyToSurface(surface);

	SDL_FillRect(surface,NULL, backgroundcolor);

	if(textcolor == -1) textcolor = 147;
	if(textshadowcolor == -1) textshadowcolor = 110;

	std::list<SDL_Surface*> TextSurfaces;
	std::list<std::string>::const_iterator iter;
	for(iter = textLines.begin(); iter != textLines.end() ; ++iter) {
		std::string text = *iter;

		// create text background
		TextSurfaces.push_back(createSurfaceWithText(text.c_str(), textshadowcolor, fontID));
		// create text foreground
		TextSurfaces.push_back(createSurfaceWithText(text.c_str(), textcolor, fontID));
	}

	int fontheight = getTextHeight(fontID);
	int spacing = 2;

	int textpos_y;

	if(alignment & Alignment_VCenter) {
		int textheight = fontheight * textLines.size() + spacing * (textLines.size() - 1);
		textpos_y = (height - textheight) / 2;
	} else if(alignment & Alignment_Bottom) {
		int textheight = fontheight * textLines.size() + spacing * (textLines.size() - 1);
		textpos_y = height - textheight - spacing;
	} else {
		// Alignment_Top
		textpos_y = spacing;
	}

	std::list<SDL_Surface*>::const_iterator surfIter = TextSurfaces.begin();
	while(surfIter != TextSurfaces.end()) {
		SDL_Surface* textSurface1 = *surfIter;
		++surfIter;
        SDL_Surface* textSurface2 = *surfIter;
        ++surfIter;

		SDL_Rect textRect1 = { 0, textpos_y + 3, textSurface1->w, textSurface1->h };
        SDL_Rect textRect2 = { 0, textpos_y + 2, textSurface2->w, textSurface2->h };
		if(alignment & Alignment_Left) {
			textRect1.x = 4;
            textRect2.x = 3;
        } else if(alignment & Alignment_Right) {
			textRect1.x = width - textSurface1->w - 4;
            textRect2.x = width - textSurface2->w - 3;
        } else {
            // Alignment_HCenter
			textRect1.x = ((surface->w - textSurface1->w) / 2)+3;
            textRect2.x = ((surface->w - textSurface2->w) / 2)+2;
		}

		SDL_BlitSurface(textSurface1,NULL,surface,&textRect1);
		SDL_FreeSurface(textSurface1);

        SDL_BlitSurface(textSurface2,NULL,surface,&textRect2);
        SDL_FreeSurface(textSurface2);

		textpos_y += fontheight + spacing;
	}

	SDL_SetColorKey(surface, SDL_SRCCOLORKEY | SDL_RLEACCEL, 0);
	return surface;
}





Point DuneStyle::getMinimumCheckboxSize(std::string text) {
	return Point(getTextWidth(text.c_str(),FONT_STD12) + 20 + 17,getTextHeight(FONT_STD12) + 8);
}

SDL_Surface* DuneStyle::createCheckboxSurface(Uint32 width, Uint32 height, std::string text, bool checked, bool activated, int textcolor, int textshadowcolor, int backgroundcolor) {
	SDL_Surface* surface;

	// create surfaces
	if((surface = SDL_CreateRGBSurface(SDL_HWSURFACE,width,height,8,0,0,0,0))== NULL) {
		return NULL;
	}
	palette.applyToSurface(surface);

	SDL_FillRect(surface,NULL, backgroundcolor);

	if(textcolor == -1) textcolor = 147;
	if(textshadowcolor == -1) textshadowcolor = 110;

    if(activated) {
        textcolor -= 2;
    }

	drawRect(surface, 4, 5, 4 + 17, 5 + 17, textcolor);
	drawRect(surface, 4 + 1, 5 + 1, 4 + 17 - 1, 5 + 17 - 1, textcolor);

	if(checked) {
		int x1 = 4 + 2;
		int y1 = 5 + 2;
		int x2 = 4 + 17 - 2;

		for(int i = 0; i < 15; i++) {
			// North-West to South-East
			putPixel(surface, x1, y1, textcolor);
			putPixel(surface, x1+1, y1, textcolor);
			putPixel(surface, x1-1, y1, textcolor);

			// North-East to South-West
			putPixel(surface, x2, y1, textcolor);
			putPixel(surface, x2+1, y1, textcolor);
			putPixel(surface, x2-1, y1, textcolor);

			x1++;
			y1++;
			x2--;
		}
	}


	SDL_Surface* textSurface1 = createSurfaceWithText(text.c_str(), textshadowcolor, FONT_STD12);
	SDL_Rect textRect1 = {  10+2 + 17, ((surface->h - textSurface1->h) / 2)+3,
                            textSurface1->w, textSurface1->h };
	SDL_BlitSurface(textSurface1,NULL,surface,&textRect1);
	SDL_FreeSurface(textSurface1);

	SDL_Surface* textSurface2 = createSurfaceWithText(text.c_str(), textcolor, FONT_STD12);
	SDL_Rect textRect2 = {  10+1 + 17, ((surface->h - textSurface2->h) / 2)+2,
                            textSurface2->w, textSurface2->h };
	SDL_BlitSurface(textSurface2,NULL,surface,&textRect2);
	SDL_FreeSurface(textSurface2);

	SDL_SetColorKey(surface, SDL_SRCCOLORKEY | SDL_RLEACCEL, 0);
	return surface;
}





Point DuneStyle::getMinimumRadioButtonSize(std::string text) {
	return Point(getTextWidth(text.c_str(),FONT_STD12) + 16 + 15,getTextHeight(FONT_STD12) + 8);
}

SDL_Surface* DuneStyle::createRadioButtonSurface(Uint32 width, Uint32 height, std::string text, bool checked, bool activated, int textcolor, int textshadowcolor, int backgroundcolor) {
	SDL_Surface* surface;

	// create surfaces
	if((surface = SDL_CreateRGBSurface(SDL_HWSURFACE,width,height,8,0,0,0,0))== NULL) {
		return NULL;
	}
	palette.applyToSurface(surface);

	SDL_FillRect(surface,NULL, backgroundcolor);

	if(textcolor == -1) textcolor = 147;
	if(textshadowcolor == -1) textshadowcolor = 110;

    if(activated) {
        textcolor -= 2;
    }

    drawHLineNoLock(surface, 8, 7, 13, textcolor);
    drawHLineNoLock(surface, 7, 8, 14, textcolor);
    drawHLineNoLock(surface, 6, 9, 8, textcolor);
    drawHLineNoLock(surface, 13, 9, 15, textcolor);
    drawHLineNoLock(surface, 5, 10, 6, textcolor);
    drawHLineNoLock(surface, 15, 10, 16, textcolor);
    drawHLineNoLock(surface, 4, 11, 6, textcolor);
    drawHLineNoLock(surface, 15, 11, 17, textcolor);

    drawVLineNoLock(surface, 4, 12, 15, textcolor);
    drawVLineNoLock(surface, 5, 12, 15, textcolor);
    drawVLineNoLock(surface, 16, 12, 15, textcolor);
    drawVLineNoLock(surface, 17, 12, 15, textcolor);

    drawHLineNoLock(surface, 4, 16, 6, textcolor);
    drawHLineNoLock(surface, 15, 16, 17, textcolor);
    drawHLineNoLock(surface, 5, 17, 6, textcolor);
    drawHLineNoLock(surface, 15, 17, 16, textcolor);
    drawHLineNoLock(surface, 6, 18, 8, textcolor);
    drawHLineNoLock(surface, 13, 18, 15, textcolor);
    drawHLineNoLock(surface, 7, 19, 14, textcolor);
    drawHLineNoLock(surface, 8, 20, 13, textcolor);

    if(checked) {
        drawHLineNoLock(surface, 9, 11, 12, textcolor);
        drawHLineNoLock(surface, 8, 12, 13, textcolor);
        drawHLineNoLock(surface, 8, 13, 13, textcolor);
        drawHLineNoLock(surface, 8, 14, 13, textcolor);
        drawHLineNoLock(surface, 8, 15, 13, textcolor);
        drawHLineNoLock(surface, 9, 16, 12, textcolor);
	}


	SDL_Surface* textSurface1 = createSurfaceWithText(text.c_str(), textshadowcolor, FONT_STD12);
	SDL_Rect textRect1 = {  8+2 + 15, ((surface->h - textSurface1->h) / 2)+3,
                            textSurface1->w, textSurface1->h };
	SDL_BlitSurface(textSurface1,NULL,surface,&textRect1);
	SDL_FreeSurface(textSurface1);

	SDL_Surface* textSurface2 = createSurfaceWithText(text.c_str(), textcolor, FONT_STD12);
	SDL_Rect textRect2 = {  8+1 + 15, ((surface->h - textSurface2->h) / 2)+2,
                            textSurface2->w, textSurface2->h };
	SDL_BlitSurface(textSurface2,NULL,surface,&textRect2);
	SDL_FreeSurface(textSurface2);

	SDL_SetColorKey(surface, SDL_SRCCOLORKEY | SDL_RLEACCEL, 0);
	return surface;
}



SDL_Surface* DuneStyle::createDropDownBoxButton(Uint32 size, bool pressed, bool activated, int color) {
    if(color == -1) {
        color = 147;
    }

	// create surfaces
	SDL_Surface* surface;
	if((surface = SDL_CreateRGBSurface(SDL_HWSURFACE,size,size,8,0,0,0,0))== NULL) {
		return NULL;
	}
	palette.applyToSurface(surface);


	// create button background
	if(pressed == false) {
		// normal mode
		SDL_FillRect(surface, NULL, 115);
		drawRect(surface, 0, 0, surface->w-1, surface->h-1, 229);
		drawHLine(surface, 1, 1, surface->w-2, 108);
		drawVLine(surface, 1, 1, surface->h-2, 108);
		drawHLine(surface, 1, surface->h-2, surface->w-2, 226);
		drawVLine(surface, surface->w-2, 1, surface->h-2, 226);
	} else {
		// pressed button mode
		SDL_FillRect(surface, NULL, 116);
		drawRect(surface, 0, 0, surface->w-1, surface->h-1, 229);
		drawRect(surface, 1, 1, surface->w-2, surface->h-2, 226);
	}

	int col = (pressed | activated) ? (color-2) : color;

	int x1 = 3;
	int x2 = size-3-1;
	int y = size/3 - 1;

	// draw separated hline
	drawHLine(surface, x1, y, x2, col);
	y+=2;

	// down arrow
	for(;x1 <= x2; ++x1, --x2, ++y) {
		drawHLine(surface, x1, y, x2, col);
	}

	return surface;
}




Point DuneStyle::getMinimumButtonSize(std::string text) {
	return Point(getTextWidth(text.c_str(),FONT_STD10)+12,getTextHeight(FONT_STD10));
}

SDL_Surface* DuneStyle::createButtonSurface(Uint32 width, Uint32 height, std::string text, bool pressed, bool activated, int textcolor, int textshadowcolor) {

	// create surfaces
    SDL_Surface* surface;
	if((surface = SDL_CreateRGBSurface(SDL_HWSURFACE,width,height,8,0,0,0,0))== NULL) {
		return NULL;
	}
	palette.applyToSurface(surface);


	// create button background
	if(pressed == false) {
		// normal mode
		SDL_FillRect(surface, NULL, 115);
		drawRect(surface, 0, 0, surface->w-1, surface->h-1, 229);
		drawHLine(surface, 1, 1, surface->w-2, 108);
		drawVLine(surface, 1, 1, surface->h-2, 108);
		drawHLine(surface, 1, surface->h-2, surface->w-2, 226);
		drawVLine(surface, surface->w-2, 1, surface->h-2, 226);
	} else {
		// pressed button mode
		SDL_FillRect(surface, NULL, 116);
		drawRect(surface, 0, 0, surface->w-1, surface->h-1, 229);
		drawRect(surface, 1, 1, surface->w-2, surface->h-2, 226);

	}

	// create text on this button
	int fontsize;
	if(	(width < getTextWidth(text.c_str(),FONT_STD12) + 12) ||
		(height < getTextHeight(FONT_STD12) + 2)) {
		fontsize = FONT_STD10;
	} else {
		fontsize = FONT_STD12;
	}

    if(textcolor == -1) textcolor = 147;
	if(textshadowcolor == -1) textshadowcolor = 110;

	SDL_Surface* textSurface1 = createSurfaceWithText(text.c_str(), textshadowcolor, fontsize);
	SDL_Rect textRect1 = {  ((surface->w - textSurface1->w) / 2)+2+(pressed ? 1 : 0),
                            ((surface->h - textSurface1->h) / 2)+3+(pressed ? 1 : 0),
                            textSurface1->w,
                            textSurface1->h };
	SDL_BlitSurface(textSurface1,NULL,surface,&textRect1);
	SDL_FreeSurface(textSurface1);

	SDL_Surface* textSurface2 = createSurfaceWithText(text.c_str(), (activated == true) ? (textcolor-2) : textcolor, fontsize);
	SDL_Rect textRect2 = {  ((surface->w - textSurface2->w) / 2)+1+(pressed ? 1 : 0),
                            ((surface->h - textSurface2->h) / 2)+2+(pressed ? 1 : 0),
                            textSurface2->w,
                            textSurface2->h };
	SDL_BlitSurface(textSurface2,NULL,surface,&textRect2);
	SDL_FreeSurface(textSurface2);

	return surface;
}




Point DuneStyle::getMinimumTextBoxSize(int fontID) {
	return Point(10,getTextHeight(fontID) + 6);
}

SDL_Surface* DuneStyle::createTextBoxSurface(Uint32 width, Uint32 height, std::string text, bool carret, int fontID, Alignment_Enum alignment, int textcolor, int textshadowcolor) {

	// create surfaces
	SDL_Surface* surface;
	if((surface = SDL_CreateRGBSurface(SDL_HWSURFACE,width,height,8,0,0,0,0))== NULL) {
		return NULL;
	}
	palette.applyToSurface(surface);

	SDL_FillRect(surface, NULL, 115);
	drawRect(surface,0,0,surface->w-1,surface->h-1,229);

	drawHLine(surface,1,1,surface->w-2,226);
	drawHLine(surface,1,2,surface->w-2,226);
	drawVLine(surface,1,1,surface->h-2,226);
	drawVLine(surface,2,1,surface->h-2,226);
	drawHLine(surface,1,surface->h-2,surface->w-2,108);
	drawVLine(surface,surface->w-2,1,surface->h-2,108);

    if(textcolor == -1) textcolor = 147;
	if(textshadowcolor == -1) textshadowcolor = 110;

	SDL_Rect cursorPos;

	// create text in this text box
	if(text.size() != 0) {
		SDL_Surface* textSurface1 = createSurfaceWithText(text.c_str(), textshadowcolor, fontID);
		SDL_Surface* textSurface2 = createSurfaceWithText(text.c_str(), textcolor, fontID);
		SDL_Rect textRect1 = { 0, ((surface->h - textSurface1->h) / 2)+3, textSurface1->w, textSurface1->h };
		SDL_Rect textRect2 = { 0, ((surface->h - textSurface2->h) / 2)+2, textSurface2->w, textSurface2->h };

        if(alignment & Alignment_Left) {
            textRect1.x = 6;
            textRect2.x = 5;
        } else if(alignment & Alignment_Right) {
            textRect1.x = width - textSurface1->w - 5;
            textRect2.x = width - textSurface2->w - 4;
        } else {
            // Alignment_HCenter
            textRect1.x = ((surface->w - textSurface1->w) / 2)+3;
            textRect2.x = ((surface->w - textSurface2->w) / 2)+2;
        }

		if(textRect1.w > surface->w - 10) {
			textRect1.x -= (textSurface1->w - (surface->w - 10));
			textRect2.x -= (textSurface2->w - (surface->w - 10));
		}

		cursorPos.x = textRect2.x + textSurface2->w + 2;

		SDL_BlitSurface(textSurface1,NULL,surface,&textRect1);
		SDL_FreeSurface(textSurface1);

		SDL_BlitSurface(textSurface2,NULL,surface,&textRect2);
		SDL_FreeSurface(textSurface2);

		cursorPos.w = 1;
	} else {
		if(alignment & Alignment_Left) {
            cursorPos.x = 6;
        } else if(alignment & Alignment_Right) {
            cursorPos.x = width - 5;
        } else {
            // Alignment_HCenter
            cursorPos.x = surface->w / 2;
        }
		cursorPos.w = 1;
	}

	cursorPos.y = surface->h / 2 - 8;
	cursorPos.h = 16;

	if(carret == true) {
		drawVLine(surface,cursorPos.x,cursorPos.y,cursorPos.y+cursorPos.h,textcolor);
		drawVLine(surface,cursorPos.x+1,cursorPos.y,cursorPos.y+cursorPos.h,textcolor);
	}

	return surface;
}




Point DuneStyle::getMinimumScrollBarArrowButtonSize() {
	return Point(17,17);
}

SDL_Surface* DuneStyle::createScrollBarArrowButton(bool down, bool pressed, bool activated, int color) {
    if(color == -1) {
        color = 147;
    }

	// create surfaces
	SDL_Surface* surface;
	if((surface = SDL_CreateRGBSurface(SDL_HWSURFACE,17,17,8,0,0,0,0))== NULL) {
		return NULL;
	}
	palette.applyToSurface(surface);


	// create button background
	if(pressed == false) {
		// normal mode
		SDL_FillRect(surface, NULL, 115);
		drawRect(surface, 0, 0, surface->w-1, surface->h-1, 229);
		drawHLine(surface, 1, 1, surface->w-2, 108);
		drawVLine(surface, 1, 1, surface->h-2, 108);
		drawHLine(surface, 1, surface->h-2, surface->w-2, 226);
		drawVLine(surface, surface->w-2, 1, surface->h-2, 226);
	} else {
		// pressed button mode
		SDL_FillRect(surface, NULL, 116);
		drawRect(surface, 0, 0, surface->w-1, surface->h-1, 229);
		drawRect(surface, 1, 1, surface->w-2, surface->h-2, 226);
	}

	int col = (pressed | activated) ? (color-2) : color;

	// draw arrow
	if(down == true) {
		// down arrow
		drawHLine(surface,3,4,13,col);
		drawHLine(surface,4,5,12,col);
		drawHLine(surface,5,6,11,col);
		drawHLine(surface,6,7,10,col);
		drawHLine(surface,7,8,9,col);
		drawHLine(surface,8,9,8,col);
	} else {
		// up arrow
		drawHLine(surface,8,5,8,col);
		drawHLine(surface,7,6,9,col);
		drawHLine(surface,6,7,10,col);
		drawHLine(surface,5,8,11,col);
		drawHLine(surface,4,9,12,col);
		drawHLine(surface,3,10,13,col);
	}

	return surface;
}




Uint32 DuneStyle::getListBoxEntryHeight() {
	return 16;
}

SDL_Surface* DuneStyle::createListBoxEntry(Uint32 width, std::string text, bool selected, int color) {
    if(color == -1) {
        color = 147;
    }

	// create surfaces
	SDL_Surface* surface;
	if((surface = SDL_CreateRGBSurface(SDL_HWSURFACE,width,getListBoxEntryHeight(),8,0,0,0,0))== NULL) {
		return NULL;
	}
	palette.applyToSurface(surface);
	if(selected == true) {
		SDL_FillRect(surface, NULL, 115);
	} else {
		SDL_SetColorKey(surface, SDL_SRCCOLORKEY | SDL_RLEACCEL, 0);
	}

	SDL_Surface* textSurface;
	textSurface = createSurfaceWithText(text.c_str(), color, FONT_STD10);
	SDL_Rect textRect = {   3, ((surface->h - textSurface->h) / 2) + 1,
                            textSurface->w, textSurface->h };
	SDL_BlitSurface(textSurface,NULL,surface,&textRect);
	SDL_FreeSurface(textSurface);

	return surface;
}




SDL_Surface* DuneStyle::createProgressBarOverlay(Uint32 width, Uint32 height, double percent, int color) {

	// create surfaces
	SDL_Surface* pSurface;
	if((pSurface = SDL_CreateRGBSurface(SDL_HWSURFACE,width,height,8,0,0,0,0))== NULL) {
		return NULL;
	}

	palette.applyToSurface(pSurface);
	SDL_SetColorKey(pSurface, SDL_SRCCOLORKEY | SDL_RLEACCEL, 0);

	if(color == -1) {
		// default color

        unsigned int max_i = std::max( (int) lround(percent*(( ((int) width) - 4)/100.0)), 0);

		if (!SDL_MUSTLOCK(pSurface) || (SDL_LockSurface(pSurface) == 0)) {
			for (unsigned int i = 2; i < max_i + 2; i++) {
				for (unsigned int j = (i % 2) + 2; j < height-2; j+=2) {
					putPixel(pSurface, i, j, COLOR_BLACK);
				}
			}

			if (SDL_MUSTLOCK(pSurface))
				SDL_UnlockSurface(pSurface);
		}
	} else {
	    unsigned int max_i = lround(percent*(width/100.0));

		SDL_Rect dest = { 0 , 0 , max_i , height};
        SDL_FillRect(pSurface, &dest, color);
	}

	return pSurface;
}



SDL_Surface* DuneStyle::createToolTip(std::string text) {
	SDL_Surface* surface;
	SDL_Surface* helpTextSurface;

	if((helpTextSurface = createSurfaceWithText(text.c_str(), COLOR_YELLOW, FONT_STD10)) == NULL) {
		return NULL;
	}

	// create surfaces
	if((surface = SDL_CreateRGBSurface(SDL_HWSURFACE, helpTextSurface->w + 4, helpTextSurface->h + 2,8,0,0,0,0)) == NULL) {
		SDL_FreeSurface(helpTextSurface);
		return NULL;
	}
	palette.applyToSurface(surface);

	SDL_FillRect(surface, NULL, COLOR_BLACK);
	drawRect(surface, 0, 0, helpTextSurface->w + 4 - 1, helpTextSurface->h + 2 - 1, COLOR_YELLOW);

	SDL_Rect textRect = { 3, 3, helpTextSurface->w, helpTextSurface->h };
	SDL_BlitSurface(helpTextSurface, NULL, surface, &textRect);

	SDL_FreeSurface(helpTextSurface);
	return surface;
}



SDL_Surface* DuneStyle::createBackground(Uint32 width, Uint32 height) {
	SDL_Surface* pSurface;
	if(pGFXManager != NULL) {
		pSurface = getSubPicture(pGFXManager->getUIGraphic(UI_Background), 0, 0, width, height);
		if(pSurface == NULL) {
			return NULL;
		}
	} else {
		// data manager not yet loaded
		if((pSurface = SDL_CreateRGBSurface(SDL_HWSURFACE,width,height,8,0,0,0,0))== NULL) {
			return NULL;
		}
		palette.applyToSurface(pSurface);
		SDL_FillRect(pSurface, NULL, 115);
	}


	drawRect(pSurface, 0, 0, pSurface->w-1, pSurface->h-1, 229);
	drawHLine(pSurface, 1, 1, pSurface->w-2, 108);
	drawHLine(pSurface, 2, 2, pSurface->w-3, 108);
	drawVLine(pSurface, 1, 1, pSurface->h-2, 108);
	drawVLine(pSurface, 2, 2, pSurface->h-3, 108);
	drawHLine(pSurface, 1, pSurface->h-2, pSurface->w-2, 226);
	drawHLine(pSurface, 2, pSurface->h-3, pSurface->w-3, 226);
	drawVLine(pSurface, pSurface->w-2, 1, pSurface->h-2, 226);
	drawVLine(pSurface, pSurface->w-3, 2, pSurface->h-3, 226);

	return pSurface;
}

SDL_Surface* DuneStyle::createWidgetBackground(Uint32 width, Uint32 height) {
	SDL_Surface* surface;

	// create surfaces
	if((surface = SDL_CreateRGBSurface(SDL_HWSURFACE,width,height,8,0,0,0,0))== NULL) {
		return NULL;
	}
	palette.applyToSurface(surface);


	SDL_FillRect(surface, NULL, 116);
	drawRect(surface, 0, 0, surface->w-1, surface->h-1, 229);
	drawRect(surface, 1, 1, surface->w-2, surface->h-2, 226);

	return surface;
}
