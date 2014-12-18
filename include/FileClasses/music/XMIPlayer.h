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


#ifndef XMIPLAYER_H
#define XMIPLAYER_H

#include <FileClasses/music/MusicPlayer.h>

#include <Definitions.h>

#include <vector>
#include <string>
#include <SDL_mixer.h>

// Forward declarations
class CadlPlayer;

class XMIPlayer : public MusicPlayer {
public:
    XMIPlayer();
    virtual ~XMIPlayer();

	/*!
		change type of current music
		@param musicType type of music to be played
	*/
	void changeMusic(MUSICTYPE musicType);

    /*!
		sets current music to MUSIC_PEACE if there's no
		other song being played
	*/
	void musicCheck();

    /*!
        Toggle the music on and off
    */
	void toggleSound();

    /*!
		turns music playing on or off
		@param value when true the function turns music on
	*/
	void setMusic(bool value);

    /**
        Sets the volume of the music channel
        \param  newVolume   the new volume [0;MIX_MAX_VOLUME]
    */
	virtual void setMusicVolume(int newVolume) {
		MusicPlayer::setMusicVolume(newVolume);
		Mix_VolumeMusic(newVolume);
	}

private:

    std::string getTmpFileName();


    Mix_Music*      music;
};

#endif // XMIPLAYER_H
