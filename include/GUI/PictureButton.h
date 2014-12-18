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

#ifndef PICTUREBUTTON_H
#define PICTUREBUTTON_H

#include "Button.h"

#include <SDL.h>

/// A class for a picture button
class PictureButton : public Button {
public:
	/// Default contructor
	PictureButton() : Button() {
		enableResizing(false,false);
	}

	/// destructor
	virtual ~PictureButton() { ; };

	/**
		This method is used for setting the different surfaces for this button.
		\param	pUnpressedSurface		This surface is normally shown
		\param	bFreeUnpressedSurface	Should pUnpressedSurface be freed if this button is destroyed?
		\param	pPressedSurface			This surface is shown when the button is pressed
		\param	bFreePressedSurface		Should pPressedSurface be freed if this button is destroyed?
		\param	pActiveSurface			This surface is shown when the button is activated by keyboard or by mouse hover
		\param	bFreeActiveSurface		Should pActiveSurface be freed if this button is destroyed?
	*/
	virtual void setSurfaces(	SDL_Surface* pUnpressedSurface,bool bFreeUnpressedSurface,
								SDL_Surface* pPressedSurface = NULL,bool bFreePressedSurface = false,
								SDL_Surface* pActiveSurface = NULL,bool bFreeActiveSurface = false) {
		Button::setSurfaces(pUnpressedSurface,bFreeUnpressedSurface,
							pPressedSurface,bFreePressedSurface,
							pActiveSurface,bFreeActiveSurface);

		if(pUnpressedSurface != NULL) {
			resize(pUnpressedSurface->w,pUnpressedSurface->h);
		} else {
			resize(0,0);
		}
	}

	/**
		Returns the minimum size of this button. The button should not
		be resized to a size smaller than this.
		\return the minimum size of this button
	*/
	virtual Point getMinimumSize() const {
		if(pUnpressedSurface != NULL) {
			return Point((Sint32) pUnpressedSurface->w, (Sint32) pUnpressedSurface->h);
		} else {
			return Point(0,0);
		}
	}
};

#endif //PICTUREBUTTON_H
