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

#ifndef FADEOUTVIDEOEVENT_H
#define FADEOUTVIDEOEVENT_H

#include <CutScenes/VideoEvent.h>
#include <FileClasses/Palette.h>
#include <SDL.h>

/**
    This VideoEvent is used for fading out a picture
*/
class FadeOutVideoEvent : public VideoEvent {
public:

    /**
        Constructor
        \param  pSurface            The picture to fade out
        \param  numFrames2FadeOut   The number of frames the fading should take
        \param  bFreeSurface        true = SDL_FreeSurface(pSurface) after fading out is done, false = pSurface is not freed
        \param  bCenterVertical     true = center the surface vertically on the screen, false = blit the surface at the top of the screen (default is true)
        \param  bFadeWhite          true = fade to white, false = fade to black (default is false)
    */
	FadeOutVideoEvent(SDL_Surface* pSurface, int numFrames2FadeOut, bool bFreeSurface, bool bCenterVertical = true, bool bFadeWhite = false);

    /// destructor
	virtual ~FadeOutVideoEvent();

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
    int currentFrame;           ///< the current frame number relative to the start of this FadeOutVideoEvent
    int numFrames2FadeOut;      ///< the number of frames the fading should take
    SDL_Surface* pSurface;      ///< the picture to fade out
    Palette oldPalette;         ///< the saved palette before the fading
    SDL_Surface* pOldScreen;    ///< the screen surface the palette is saved from
    bool bFreeSurface;          ///< true = SDL_FreeSurface(pSurface) after fading out is done, false = pSurface is not freed
    bool bCenterVertical;       ///< true = center the surface vertically on the screen, false = blit the surface at the top of the screen
    bool bFadeWhite;            ///< true = fade to white, false = fade to black
};

#endif // FADEOUTVIDEOEVENT_H
