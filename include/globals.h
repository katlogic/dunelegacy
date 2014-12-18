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

#ifndef GLOBALS_H
#define GLOBALS_H

#include <DataTypes.h>
#include <Definitions.h>
#include <FileClasses/Palette.h>
#include <data.h>
#include <misc/RobustList.h>
#include <SDL.h>

#define _(msgid) pTextManager->getLocalized(msgid)

// forward declarations
class SoundPlayer;
class MusicPlayer;

class FileManager;
class GFXManager;
class SFXManager;
class FontManager;
class TextManager;
class NetworkManager;

class Game;
class Map;
class ScreenBorder;
class House;
class HumanPlayer;
class UnitBase;
class StructureBase;
class Bullet;

#ifndef SKIP_EXTERN_DEFINITION
 #define EXTERN extern
#else
 #define EXTERN
#endif


// SDL stuff
EXTERN SDL_Surface*         screen;                     ///< the screen
EXTERN Palette              palette;                    ///< the palette for the screen
EXTERN int                  drawnMouseX;                ///< the current mouse position (x coordinate)
EXTERN int                  drawnMouseY;                ///< the current mouse position (y coordinate)
EXTERN int                  cursorFrame;                ///< the current mouse cursor
EXTERN int                  currentZoomlevel;           ///< 0 = the smallest zoom level, 1 = medium zoom level, 2 = maximum zoom level


// abstraction layers
EXTERN SoundPlayer*	        soundPlayer;                ///< manager for playing sfx and voice
EXTERN MusicPlayer*         musicPlayer;                ///< manager for playing background music

EXTERN FileManager*         pFileManager;               ///< manager for loading files from PAKs
EXTERN GFXManager*          pGFXManager;                ///< manager for loading and managing graphics
EXTERN SFXManager*          pSFXManager;                ///< manager for loading and managing sounds
EXTERN FontManager*         pFontManager;               ///< manager for loading and managing fonts
EXTERN TextManager*         pTextManager;               ///< manager for loading and managing texts and providing localization
EXTERN NetworkManager*		pNetworkManager;            ///< manager for all network events (NULL if not in multiplayer game)

// game stuff
EXTERN Game*                currentGame;                ///< the current running game
EXTERN ScreenBorder*        screenborder;               ///< the screen border for the current running game
EXTERN Map*                 currentGameMap;             ///< the map for the current running game
EXTERN House*		        pLocalHouse;                ///< the house of the human player that is playing the current running game on this computer
EXTERN HumanPlayer*         pLocalPlayer;               ///< the player that is playing the current running game on this computer

EXTERN RobustList<UnitBase*>       unitList;           ///< the list of all units
EXTERN RobustList<StructureBase*>  structureList;      ///< the list of all structures
EXTERN RobustList<Bullet*>     bulletList;         ///< the list of all bullets


// misc
EXTERN SettingsClass    settings;                       ///< the settings read from the settings file

EXTERN bool debug;                                      ///< is set for debugging purposes


// constants
static const int lookDist[21] = { 10, 10, 9, 9, 9, 8, 8, 7, 6, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };    ///< lookup table for the circular viewing area
static const int houseColor[NUM_HOUSES] = { COLOR_HARKONNEN, COLOR_ATREIDES, COLOR_ORDOS, COLOR_FREMEN, COLOR_SARDAUKAR, COLOR_MERCENARY };    ///< the base colors for the different houses
static const char houseChar[] = { 'H', 'A', 'O', 'F', 'S', 'M' };   ///< character for each house

#endif //GLOBALS_H
