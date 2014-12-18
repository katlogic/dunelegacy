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

#include <FileClasses/music/XMIPlayer.h>

#include <globals.h>

#include <FileClasses/FileManager.h>
#include <FileClasses/xmidi/xmidi.h>

#include <misc/FileSystem.h>
#include <misc/fnkdat.h>
#include <mmath.h>

#include <iostream>
#include <cstdio>

XMIPlayer::XMIPlayer() : MusicPlayer(settings.audio.playMusic) {
    musicVolume = MIX_MAX_VOLUME/2;
    Mix_VolumeMusic(musicVolume);

	music = NULL;
}

XMIPlayer::~XMIPlayer() {
	if(music != NULL) {
		Mix_FreeMusic(music);
		music = NULL;
	}

	remove(getTmpFileName().c_str());
}

void XMIPlayer::changeMusic(MUSICTYPE musicType)
{
	int musicNum = -1;
	std::string filename = "";

	if(currentMusicType == musicType && Mix_PlayingMusic()) {
		return;
	}

    /* currently unused:
        DUNE0.XMI/4
        DUNE1.XMI/2 and DUNE10.XMI/2
    */


	switch(musicType) {
		case MUSIC_ATTACK: {

            switch(getRandomInt(0, 5)) {
                case 0:     filename = "DUNE10.XMI";    musicNum = 7;   break;
                case 1:     filename = "DUNE11.XMI";    musicNum = 7;   break;
                case 2:     filename = "DUNE12.XMI";    musicNum = 7;   break;
                case 3:     filename = "DUNE13.XMI";    musicNum = 7;   break;
                case 4:     filename = "DUNE14.XMI";    musicNum = 7;   break;
                case 5:     filename = "DUNE15.XMI";    musicNum = 7;   break;
            }

		} break;

		case MUSIC_PEACE: {

            switch(getRandomInt(0, 8)) {
                case 0:     filename = "DUNE1.XMI";     musicNum = 6;   break;
                case 1:     filename = "DUNE2.XMI";     musicNum = 6;   break;
                case 2:     filename = "DUNE3.XMI";     musicNum = 6;   break;
                case 3:     filename = "DUNE4.XMI";     musicNum = 6;   break;
                case 4:     filename = "DUNE5.XMI";     musicNum = 6;   break;
                case 5:     filename = "DUNE6.XMI";     musicNum = 6;   break;
                case 6:     filename = "DUNE9.XMI";     musicNum = 4;   break;
                case 7:     filename = "DUNE9.XMI";     musicNum = 5;   break;
                case 8:     filename = "DUNE18.XMI";    musicNum = 6;   break;
            }

		} break;

		case MUSIC_INTRO: {
		    filename = "DUNE0.XMI";
		    musicNum = 2;
        } break;

		case MUSIC_MENU: {
		    filename = "DUNE7.XMI";
		    musicNum = 6;
        } break;

		case MUSIC_BRIEFING_H: {
		    filename = "DUNE7.XMI";
		    musicNum = 2;
        } break;

		case MUSIC_BRIEFING_A: {
		    filename = "DUNE7.XMI";
		    musicNum = 3;
        } break;

		case MUSIC_BRIEFING_O: {
		    filename = "DUNE7.XMI";
		    musicNum = 4;
        } break;

		case MUSIC_WIN_H: {
		    filename = "DUNE8.XMI";
		    musicNum = 3;
		} break;

		case MUSIC_WIN_A: {
		    filename = "DUNE8.XMI";
		    musicNum = 2;
		} break;

		case MUSIC_WIN_O: {
		    filename = "DUNE17.XMI";
		    musicNum = 4;
		} break;

		case MUSIC_LOSE: {
		    filename = "DUNE1.XMI";
		    musicNum = 4;
		} break;

        case MUSIC_GAMESTAT: {
            filename = "DUNE20.XMI";
            musicNum = 2;
		} break;

        case MUSIC_MAPCHOICE: {
		    filename = "DUNE16.XMI";
		    musicNum = 7;
		} break;

		case MUSIC_MEANWHILE: {
		    filename = "DUNE16.XMI";
		    musicNum = 8;
		} break;

        case MUSIC_FINALE_H: {
		    filename = "DUNE19.XMI";
		    musicNum = 4;
		} break;

        case MUSIC_FINALE_A: {
		    filename = "DUNE19.XMI";
		    musicNum = 2;
		} break;

        case MUSIC_FINALE_O: {
		    filename = "DUNE19.XMI";
		    musicNum = 3;
		} break;

		case MUSIC_RANDOM:
		default: {

            switch(getRandomInt(0, 14)) {
                // attack
                case 0:     filename = "DUNE10.XMI";    musicNum = 7;   break;
                case 1:     filename = "DUNE11.XMI";    musicNum = 7;   break;
                case 2:     filename = "DUNE12.XMI";    musicNum = 7;   break;
                case 3:     filename = "DUNE13.XMI";    musicNum = 7;   break;
                case 4:     filename = "DUNE14.XMI";    musicNum = 7;   break;
                case 5:     filename = "DUNE15.XMI";    musicNum = 7;   break;

                // peace
                case 6:     filename = "DUNE1.XMI";     musicNum = 6;   break;
                case 7:     filename = "DUNE2.XMI";     musicNum = 6;   break;
                case 8:     filename = "DUNE3.XMI";     musicNum = 6;   break;
                case 9:     filename = "DUNE4.XMI";     musicNum = 6;   break;
                case 10:    filename = "DUNE5.XMI";     musicNum = 6;   break;
                case 11:    filename = "DUNE6.XMI";     musicNum = 6;   break;
                case 12:    filename = "DUNE9.XMI";     musicNum = 4;   break;
                case 13:    filename = "DUNE9.XMI";     musicNum = 5;   break;
                case 14:    filename = "DUNE18.XMI";    musicNum = 6;   break;
            }

		} break;
	}

	currentMusicType = musicType;

	if((musicOn == true) && (filename != "")) {
        SDL_RWops* inputrwop = pFileManager->openFile(filename);
        if(inputrwop == NULL) {
            std::cerr << "Cannot open file " << filename << "!" << std::endl;
            return;
        }
        SDLDataSource input(inputrwop,1);

        std::string tmpFilename = getTmpFileName();

        SDL_RWops* outputrwop = SDL_RWFromFile(tmpFilename.c_str(),"wb");
        if(outputrwop == NULL) {
            std::cerr << "Cannot open file " << tmpFilename << "!" << std::endl;
            return;
        }
        SDLDataSource output(outputrwop,1);

        XMIDI myXMIDI(&input, XMIDI_CONVERT_NOCONVERSION);
        myXMIDI.retrieve(musicNum, &output );

        input.close();
        output.close();

        Mix_HaltMusic();
		if(music != NULL) {
			Mix_FreeMusic(music);
			music = NULL;
		}

		music = Mix_LoadMUS(tmpFilename.c_str());
		if(music != NULL) {
			printf("Now playing %s!\n", tmpFilename.c_str());
			Mix_PlayMusic(music, -1);
		} else {
			printf("Unable to play %s: %s!\n", filename.c_str(), Mix_GetError());
		}

	}
}

void XMIPlayer::musicCheck() {
	if(musicOn) {
		if(!Mix_PlayingMusic()) {
			changeMusic(MUSIC_PEACE);
		}
	}
}

void XMIPlayer::setMusic(bool value) {
	musicOn = value;

	if(musicOn) {
		changeMusic(MUSIC_RANDOM);
	} else if(music != NULL) {
		Mix_HaltMusic();
	}
}


void XMIPlayer::toggleSound()
{
	if(musicOn == false) {
		musicOn = true;
		changeMusic(MUSIC_PEACE);
	} else {
		musicOn = false;
		if (music != NULL) {
			Mix_HaltMusic();
            Mix_FreeMusic(music);
            music = NULL;
		}
	}
}

std::string XMIPlayer::getTmpFileName() {
	// determine path to config file
	char tmp[FILENAME_MAX];
	fnkdat("tmp.mid", tmp, FILENAME_MAX, FNKDAT_USER | FNKDAT_CREAT);
	return std::string(tmp);
}
