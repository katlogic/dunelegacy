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

#ifndef FADEINVIDEOEVENT_H
#define FADEINVIDEOEVENT_H

#include <CutScenes/VideoEvent.h>
#include <FileClasses/Palette.h>
#include <SDL.h>

/**
    This VideoEvent is used for fading in a picture
*/
class FadeInVideoEvent : public VideoEvent {
public:

    /**
        Constructor
        \param  pSurface            The picture to fade in
        \param  numFrames2FadeIn    The number of frames the fading should take
        \param  bFreeSurface        true = SDL_FreeSurface(pSurface) after fading in is done, false = pSurface is not freed
        \param  bCenterVertical     true = center the surface vertically on the screen, false = blit the surface at the top of the screen (default is true)
        \param  bFadeWhite          true = fade from white, false = fade from black (default is false)
    */
	FadeInVideoEvent(SDL_Surface* pSurface, int numFrames2FadeIn, bool bFreeSurface, bool bCenterVertical = true, bool bFadeWhite = false);

	/// destructor
	virtual ~FadeInVideoEvent();

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

private:
    int currentFrame;           ///< the current frame number relative to the start of this FadeInVideoEvent
    int numFrames2FadeIn;       ///< the number of frames the fading should take
    SDL_Surface* pSurface;      ///< the picture to fade in
    Palette oldPalette;         ///< the saved palette before the fading
    SDL_Surface* pOldScreen;    ///< the screen surface the palette is saved from
    bool bFreeSurface;          ///< true = SDL_FreeSurface(pSurface) after fading in is done, false = pSurface is not freed
    bool bCenterVertical;       ///< true = center the surface vertically on the screen, false = blit the surface at the top of the screen
    bool bFadeWhite;            ///< true = fade from white, false = fade from black
};

#endif // FADEINVIDEOEVENT_H
