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


#ifndef DIRECTORYPLAYER_H
#define DIRECTORYPLAYER_H

#include <FileClasses/music/MusicPlayer.h>

#include <vector>
#include <string>
#include <SDL_mixer.h>

class DirectoryPlayer : public MusicPlayer {
public:
    DirectoryPlayer();
    virtual ~DirectoryPlayer();

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

    /*!
        Get all audio files (mp3, ogg, wav and mid) inside the specified directory.
        @param dir the directory to get the files from
        @return the path to all the music files in this directory
    */
	std::vector<std::string> getMusicFileNames(const std::string& dir);

	std::vector<std::string> musicFileList[MUSIC_NUM_MUSIC_TYPES];

	Mix_Music*      music;
};

#endif // DIRECTORYPLAYER_H
