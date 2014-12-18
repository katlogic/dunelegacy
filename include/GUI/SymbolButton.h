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

#ifndef SYMBOLBUTTON_H
#define SYMBOLBUTTON_H

#include "Button.h"

#include <SDL.h>

/// A class for a symbol button
class SymbolButton : public Button {
public:
	/// Default contructor
	SymbolButton() : Button() {
		enableResizing(true,true);
		pSymbolSurface = NULL;
		bFreeSymbolSurface = false;
		pActiveSymbolSurface = NULL;
		bFreeActiveSymbolSurface = false;
	}

	/// destructor
	virtual ~SymbolButton() {
		if(bFreeSymbolSurface) {
			SDL_FreeSurface(pSymbolSurface);
			pSymbolSurface = NULL;
			bFreeSymbolSurface = false;
		}

		if(bFreeActiveSymbolSurface) {
			SDL_FreeSurface(pActiveSymbolSurface);
			pActiveSymbolSurface = NULL;
			bFreeActiveSymbolSurface = false;
		}
	};

	/**
		This method is used for setting the symbol for this button.
		\param	pSymbolSurface		    This is the symbol to show
		\param	bFreeSymbolSurface	    Should pSymbolSurface be freed if this button is destroyed?
		\param	pActiveSymbolSurface		This is the symbol to show on mouse over
		\param	bActiveFreeSymbolSurface	Should pActiveSymbolSurface be freed if this button is destroyed?
	*/
	virtual void setSymbol(SDL_Surface* pSymbolSurface,bool bFreeSymbolSurface, SDL_Surface* pActiveSymbolSurface = NULL, bool bFreeActiveSymbolSurface = false) {
		if(pSymbolSurface == NULL) {
			return;
		}

		if(this->bFreeSymbolSurface) {
			SDL_FreeSurface(this->pSymbolSurface);
			this->pSymbolSurface = NULL;
			this->bFreeSymbolSurface = false;
		}

		if(this->bFreeActiveSymbolSurface) {
			SDL_FreeSurface(this->pActiveSymbolSurface);
			this->pActiveSymbolSurface = NULL;
			this->bFreeActiveSymbolSurface = false;
		}

		this->pSymbolSurface = pSymbolSurface;
		this->bFreeSymbolSurface = bFreeSymbolSurface;

		this->pActiveSymbolSurface = pActiveSymbolSurface;
		this->bFreeActiveSymbolSurface = bFreeActiveSymbolSurface;

		resizeAll();
	}

	/**
		This method resized the button to width and height. This method should only
		called if the new size is a valid size for this button (See getMinumumSize).
		\param	width	the new width of this button
		\param	height	the new height of this button
	*/
	virtual void resize(Uint32 width, Uint32 height) {
		SDL_Surface *Unpressed = NULL;
		SDL_Surface *Pressed = NULL;
		SDL_Surface *Active = NULL;

		Unpressed = GUIStyle::getInstance().createButtonSurface(width, height, "", false, true);
		Pressed = GUIStyle::getInstance().createButtonSurface(width, height, "", true, true);


		if(pSymbolSurface != NULL) {
			SDL_Rect dest = {   (Unpressed->w / 2) - (pSymbolSurface->w / 2),
                                (Unpressed->h / 2) - (pSymbolSurface->h / 2),
                                pSymbolSurface->w,
                                pSymbolSurface->h   };
			SDL_BlitSurface(pSymbolSurface, NULL, Unpressed, &dest);

            SDL_Rect dest2 = {  (Unpressed->w / 2) - (pSymbolSurface->w / 2) + 1,
                                (Unpressed->h / 2) - (pSymbolSurface->h / 2) + 1,
                                pSymbolSurface->w,
                                pSymbolSurface->h   };
			SDL_BlitSurface(pActiveSymbolSurface != NULL ? pActiveSymbolSurface : pSymbolSurface, NULL, Pressed, &dest2);
		}

        if(pActiveSymbolSurface != NULL) {
            Active = GUIStyle::getInstance().createButtonSurface(width, height, "", false, true);

			SDL_Rect dest = {   (Active->w / 2) - (pActiveSymbolSurface->w / 2),
                                (Active->h / 2) - (pActiveSymbolSurface->h / 2),
                                pActiveSymbolSurface->w,
                                pActiveSymbolSurface->h };
			SDL_BlitSurface(pActiveSymbolSurface, NULL, Active, &dest);
		}

		Button::setSurfaces(Unpressed,true,Pressed,true,Active,true);

		Widget::resize(width,height);
	}

	/**
		Returns the minimum size of this button. The button should not
		resized to a size smaller than this.
		\return the minimum size of this button
	*/
	virtual Point getMinimumSize() const {
		if(pSymbolSurface != NULL) {
			return Point((Sint32) pSymbolSurface->w + 5, (Sint32) pSymbolSurface->h + 5);
		} else {
			return Point(0,0);
		}
	}

private:
	SDL_Surface* pSymbolSurface;
	bool bFreeSymbolSurface;

	SDL_Surface* pActiveSymbolSurface;
	bool bFreeActiveSymbolSurface;

};

#endif //SYMBOLBUTTON_H
