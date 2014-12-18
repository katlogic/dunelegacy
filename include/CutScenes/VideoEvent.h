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

#ifndef VIDEOEVENT_H
#define VIDEOEVENT_H

#include <SDL.h>

/**
    This class is the base class for all video events/effects.
*/
class VideoEvent {
public:
    /// default constructor
	VideoEvent();

	/// destructor
	virtual ~VideoEvent();

    /**
        This method draws the video effect. It is called before setupPalette() is called.
        \param  pScreen the surface to draw to
        \return the milliseconds until the next frame shall be drawn.
    */
	virtual int draw(SDL_Surface* pScreen);

    /**
        This method is used for palette animations. It should only change the physical palette of pScreen.
        It is called after draw().
        \param  pScreen the surface to set the palette of
    */
	virtual void setupPalette(SDL_Surface* pScreen);

    /**
        This method checks if this VideoEvent is already finished
        \return true, if there are no more frames to draw with this VideoEvent
    */
	virtual bool isFinished();
};

#endif // VIDEOEVENT_H
