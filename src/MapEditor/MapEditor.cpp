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

#include <MapEditor/MapEditor.h>

#include <MapEditor/MapGenerator.h>
#include <MapEditor/MapMirror.h>

#include <FileClasses/GFXManager.h>
#include <FileClasses/TextManager.h>
#include <FileClasses/INIFile.h>

#include <structures/Wall.h>

#include <misc/FileSystem.h>
#include <misc/draw_util.h>
#include <misc/string_util.h>

#include <globals.h>
#include <mmath.h>
#include <sand.h>
#include <Tile.h>

#include <config.h>

#include <typeinfo>
#include <algorithm>

extern int currentZoomlevel;

// functor for std::find_if
class CoordDistance {
public:
    CoordDistance(Coord centerCoord, int distance)
     : centerCoord(centerCoord), distance(distance) {
    }

    bool operator()(Coord& coord) {
        return distanceFrom(centerCoord, coord) <= 5;
    }

private:
    Coord centerCoord;
    int distance;
};


MapEditor::MapEditor() : pInterface(NULL) {
	bQuitEditor = false;
	scrollDownMode = false;
	scrollLeftMode = false;
	scrollRightMode = false;
	scrollUpMode = false;
	shift = false;

	bChangedSinceLastSave = false;

    bLeftMousePressed = false;
	lastTerrainEditPosX = -1;
	lastTerrainEditPosY = -1;

	selectedUnitID = INVALID;
	selectedStructureID = INVALID;
	selectedMapItemCoord.invalidate();

	currentZoomlevel = settings.video.preferredZoomLevel;

	// Load SideBar
	SDL_Surface* pGameBarSurface = pGFXManager->getUIGraphic(UI_MapEditor_SideBar);
	gameBarPos.w = pGameBarSurface->w;
	gameBarPos.h = pGameBarSurface->h;
	gameBarPos.x = screen->w - gameBarPos.w;
	gameBarPos.y = 0;

	// Load TopBar
	SDL_Surface* pTopBarSurface =  pGFXManager->getUIGraphic(UI_TopBar);
	topBarPos.w = pTopBarSurface->w;
	topBarPos.h = pTopBarSurface->h;
	topBarPos.x = topBarPos.y = 0;

	// Load BottomBar
	SDL_Surface* pBottomBarSurface =  pGFXManager->getUIGraphic(UI_MapEditor_BottomBar);
	bottomBarPos.w = pBottomBarSurface->w;
	bottomBarPos.h = pBottomBarSurface->h;
	bottomBarPos.x = bottomBarPos.y = 0;

	SDL_Rect gameBoardRect = { 0, topBarPos.h, gameBarPos.x, screen->h - topBarPos.h - bottomBarPos.h };
    screenborder = new ScreenBorder(gameBoardRect);

	setMap(MapData(128,128,Terrain_Sand), MapInfo());
	setMirrorMode(MirrorModeNone);

	pInterface = new MapEditorInterface(this);

	// setup windtrap color index
	SDL_Color windtrapColor = { 192, 192, 192, 0};
	SDL_SetPalette(screen, SDL_PHYSPAL, &windtrapColor, COLOR_WINDTRAP_COLORCYCLE, 1);

	pInterface->onNew();
}

MapEditor::~MapEditor() {
    delete pInterface;

    delete screenborder;
	screenborder = NULL;
}

std::string MapEditor::generateMapname() const {
    int numPlayers = 0;
    std::vector<MapEditor::Player>::const_iterator iter;
    for(iter = players.begin(); iter != players.end(); ++iter) {
        if(iter->bActive) {
            numPlayers++;
        }
    }

    return stringify(numPlayers) + "P - " + stringify(map.getSizeX()) + "x" + stringify(map.getSizeY()) + " - " + _("New Map");
}

void MapEditor::setMirrorMode(MirrorMode newMirrorMode) {
    currentMirrorMode = newMirrorMode;

    mapMirror = std::shared_ptr<MapMirror>(MapMirror::createMapMirror(currentMirrorMode,map.getSizeX(), map.getSizeY()));
}

void MapEditor::RunEditor() {
	while(!bQuitEditor) {

	    int frameStart = SDL_GetTicks();

		processInput();
		drawScreen();

		int frameTime = SDL_GetTicks() - frameStart;
        if(settings.video.frameLimit == true) {
            if(frameTime < 32) {
                SDL_Delay(32 - frameTime);
            }
        }
	}
}

void MapEditor::setMap(const MapData& mapdata, const MapInfo& newMapInfo) {
    map = mapdata;
    mapInfo = newMapInfo;

	screenborder->adjustScreenBorderToMapsize(map.getSizeX(),map.getSizeY());

	// reset tools
	selectedUnitID = INVALID;
	selectedStructureID = INVALID;
	selectedMapItemCoord.invalidate();

    if(pInterface != NULL) {
        pInterface->deselectAll();
    }

    while(!redoOperationStack.empty()) {
        redoOperationStack.pop();
    }

    while(!undoOperationStack.empty()) {
        undoOperationStack.pop();
    }

    // reset other map properties
    loadedINIFile.reset();
    lastSaveName = "";

    spiceBlooms.clear();
    specialBlooms.clear();
    spiceFields.clear();
    choam.clear();
    reinforcements.clear();
    teams.clear();
    structures.clear();
    units.clear();
    players.clear();

    // setup default players
    if(getMapVersion() < 2) {
        players.push_back(Player(getHouseNameByNumber(HOUSE_HARKONNEN),HOUSE_HARKONNEN,HOUSE_HARKONNEN,true,false,"Human",25));
        players.push_back(Player(getHouseNameByNumber(HOUSE_ATREIDES),HOUSE_ATREIDES,HOUSE_ATREIDES,true,false,"CPU",25));
        players.push_back(Player(getHouseNameByNumber(HOUSE_ORDOS),HOUSE_ORDOS,HOUSE_ORDOS,true,false,"CPU",25));
        players.push_back(Player(getHouseNameByNumber(HOUSE_FREMEN),HOUSE_FREMEN,HOUSE_FREMEN,false,false,"CPU",25));
        players.push_back(Player(getHouseNameByNumber(HOUSE_SARDAUKAR),HOUSE_SARDAUKAR,HOUSE_SARDAUKAR,true,false,"CPU",25));
        players.push_back(Player(getHouseNameByNumber(HOUSE_MERCENARY),HOUSE_MERCENARY,HOUSE_MERCENARY,false,false,"CPU",25));
    } else {
        players.push_back(Player(getHouseNameByNumber(HOUSE_HARKONNEN),HOUSE_HARKONNEN,HOUSE_HARKONNEN,true,true,"Team1"));
        players.push_back(Player(getHouseNameByNumber(HOUSE_ATREIDES),HOUSE_ATREIDES,HOUSE_ATREIDES,true,true,"Team2"));
        players.push_back(Player(getHouseNameByNumber(HOUSE_ORDOS),HOUSE_ORDOS,HOUSE_ORDOS,true,true,"Team3"));
        players.push_back(Player(getHouseNameByNumber(HOUSE_FREMEN),HOUSE_FREMEN,HOUSE_FREMEN,false,false,"Team4"));
        players.push_back(Player(getHouseNameByNumber(HOUSE_SARDAUKAR),HOUSE_SARDAUKAR,HOUSE_SARDAUKAR,true,true,"Team5"));
        players.push_back(Player(getHouseNameByNumber(HOUSE_MERCENARY),HOUSE_MERCENARY,HOUSE_MERCENARY,false,false,"Team6"));
    }

	// setup default choam
	choam[Unit_Carryall] = 2;
	choam[Unit_Harvester] = 4;
	choam[Unit_Launcher] = 5;
	choam[Unit_MCV] = 2;
	choam[Unit_Ornithopter] = 5;
	choam[Unit_Quad] = 5;
	choam[Unit_SiegeTank] = 6;
	choam[Unit_Tank] = 6;
	choam[Unit_Trike] = 5;

    if(pInterface != NULL) {
        pInterface->onNewMap();
        pInterface->onHouseChanges();
    }

	currentEditorMode = EditorMode();

    bChangedSinceLastSave = true;
}

bool MapEditor::isTileBlocked(int x, int y, bool bSlabIsBlocking, bool bUnitsAreBlocking) const {
    std::vector<Structure>::const_iterator sIter;
    for(sIter = structures.begin(); sIter != structures.end(); ++sIter) {
        if(!bSlabIsBlocking && ((sIter->itemID == Structure_Slab1) || (sIter->itemID == Structure_Slab4)) ) {
            continue;
        }

        Coord structureSize = getStructureSize(sIter->itemID);
        Coord position = sIter->position;
        if((x >= position.x) && (x < position.x+structureSize.x) && (y >= position.y) && (y < position.y+structureSize.y)) {
            return true;
        }
    }

    if(bUnitsAreBlocking) {
        std::vector<Unit>::const_iterator uIter;
        for(uIter = units.begin(); uIter != units.end(); ++uIter) {
            if((x == uIter->position.x) && (y == uIter->position.y)) {
                return true;
            }
        }
    }

    return false;
}

std::vector<int> MapEditor::getMirrorStructures(int structureID) {
    std::vector<int> mirrorStructures;

    MapEditor::Structure* pStructure = getStructure(structureID);

    if(pStructure == NULL) {
        return mirrorStructures;
    }

    Coord structureSize = getStructureSize(pStructure->itemID);

    for(int i=0;i<mapMirror->getSize();i++) {
        Coord position = mapMirror->getCoord( pStructure->position, i, structureSize);

        std::vector<Structure>::iterator iter;
        for(iter = structures.begin(); iter != structures.end(); ++iter) {
            if(iter->position == position) {
                mirrorStructures.push_back(iter->id);
                break;
            }
        }
    }

    return mirrorStructures;
}

std::vector<int> MapEditor::getMirrorUnits(int unitID, bool bAddMissingAsInvalid) {
    std::vector<int> mirrorUnits;

    MapEditor::Unit* pUnit = getUnit(unitID);

    if(pUnit == NULL) {
        return mirrorUnits;
    }

    for(int i=0;i<mapMirror->getSize();i++) {
        Coord position = mapMirror->getCoord( pUnit->position, i);

        std::vector<Unit>::iterator iter;
        for(iter = units.begin(); iter != units.end(); ++iter) {
            if(iter->position == position) {
                mirrorUnits.push_back(iter->id);
                break;
            }
        }

        if(bAddMissingAsInvalid && (mirrorUnits.size() < (unsigned int) (i+1) )) {
            mirrorUnits.push_back(INVALID);
        }
    }

    return mirrorUnits;
}

void MapEditor::setEditorMode(const EditorMode& newEditorMode) {

    if(pInterface != NULL) {
        pInterface->deselectObject();
    }

    selectedUnitID = INVALID;
    selectedStructureID = INVALID;
    selectedMapItemCoord.invalidate();

    currentEditorMode = newEditorMode;
}

void MapEditor::startOperation() {
    if(undoOperationStack.empty() || (typeid(*undoOperationStack.top().get()) != typeid(MapEditorStartOperation))) {
        addUndoOperation(std::shared_ptr<MapEditorOperation>(new MapEditorStartOperation()));
    }
}

void MapEditor::undoLastOperation() {
    if(!undoOperationStack.empty()) {
        redoOperationStack.push(std::shared_ptr<MapEditorOperation>(new MapEditorStartOperation()));

        while((!undoOperationStack.empty()) && (typeid(*undoOperationStack.top().get()) != typeid(MapEditorStartOperation))) {
            redoOperationStack.push(undoOperationStack.top()->perform(this));
            undoOperationStack.pop();
        }

        if(!undoOperationStack.empty()) {
            undoOperationStack.pop();
        }
    }
}

void MapEditor::redoLastOperation() {
    if(!redoOperationStack.empty()) {
        undoOperationStack.push(std::shared_ptr<MapEditorOperation>(new MapEditorStartOperation()));

        while((!redoOperationStack.empty()) && (typeid(*redoOperationStack.top().get()) != typeid(MapEditorStartOperation))) {
            undoOperationStack.push(redoOperationStack.top()->perform(this));
            redoOperationStack.pop();
        }

        if(!redoOperationStack.empty()) {
            redoOperationStack.pop();
        }
    }
}

void MapEditor::loadMap(const std::string& filepath) {
    // reset tools
	selectedUnitID = INVALID;
	selectedStructureID = INVALID;

    if(pInterface != NULL) {
        pInterface->deselectAll();
    }

    while(!redoOperationStack.empty()) {
        redoOperationStack.pop();
    }

    while(!undoOperationStack.empty()) {
        undoOperationStack.pop();
    }

    // reset other map properties
    spiceBlooms.clear();
    specialBlooms.clear();
    spiceFields.clear();
    choam.clear();
    reinforcements.clear();
    teams.clear();
    structures.clear();
    units.clear();
    players.clear();

    players.push_back(Player(getHouseNameByNumber(HOUSE_HARKONNEN),HOUSE_HARKONNEN,HOUSE_HARKONNEN,false,true,"Team1"));
    players.push_back(Player(getHouseNameByNumber(HOUSE_ATREIDES),HOUSE_ATREIDES,HOUSE_ATREIDES,false,true,"Team2"));
    players.push_back(Player(getHouseNameByNumber(HOUSE_ORDOS),HOUSE_ORDOS,HOUSE_ORDOS,false,true,"Team3"));
    players.push_back(Player(getHouseNameByNumber(HOUSE_FREMEN),HOUSE_FREMEN,HOUSE_FREMEN,false,false,"Team4"));
    players.push_back(Player(getHouseNameByNumber(HOUSE_SARDAUKAR),HOUSE_SARDAUKAR,HOUSE_SARDAUKAR,false,false,"Team5"));
    players.push_back(Player(getHouseNameByNumber(HOUSE_MERCENARY),HOUSE_MERCENARY,HOUSE_MERCENARY,false,false,"Team6"));

    // load map
    loadedINIFile = std::shared_ptr<INIFile>(new INIFile(filepath, false));
    lastSaveName = filepath;

    INIMapEditorLoader* pINIMapEditorLoader = new INIMapEditorLoader(this, loadedINIFile);
    delete pINIMapEditorLoader;

    // update interface
    if(pInterface != NULL) {
        pInterface->onNewMap();
        pInterface->onHouseChanges();
    }

	currentEditorMode = EditorMode();

	bChangedSinceLastSave = false;
}

void MapEditor::saveMap(const std::string& filepath) {
    if(!loadedINIFile) {
        std::string comment = "Created with Dune Legacy " + std::string(VERSION) + " Map Editor.";
        loadedINIFile = std::shared_ptr<INIFile>(new INIFile(false, comment));
    }

    int version = (mapInfo.mapSeed == INVALID) ? 2 : 1;

    if(version > 1) {
        loadedINIFile->setIntValue("BASIC", "Version", version);
    }

    if(!mapInfo.license.empty()) {
        loadedINIFile->setStringValue("BASIC", "License", mapInfo.license);
    }

    if(!mapInfo.author.empty()) {
        loadedINIFile->setStringValue("BASIC", "Author", mapInfo.author);
    }

    if((version > 1) && (mapInfo.techLevel > 0)) {
        loadedINIFile->setIntValue("BASIC", "TechLevel", mapInfo.techLevel);
    }

    loadedINIFile->setIntValue("BASIC", "WinFlags", mapInfo.winFlags);
    loadedINIFile->setIntValue("BASIC", "LoseFlags", mapInfo.loseFlags);

    loadedINIFile->setStringValue("BASIC", "LosePicture", mapInfo.losePicture, false);
    loadedINIFile->setStringValue("BASIC", "WinPicture", mapInfo.winPicture, false);
    loadedINIFile->setStringValue("BASIC", "BriefPicture", mapInfo.briefPicture, false);

    loadedINIFile->setIntValue("BASIC","TimeOut", mapInfo.timeout);

    int logicalSizeX;
    //int logicalSizeY;
    int logicalOffsetX;
    int logicalOffsetY;

    if(version < 2) {
        logicalSizeX = 64;
	    //logicalSizeY = 64;

        int mapscale = 0;
        switch(map.getSizeX()) {
            case 21: {
                mapscale = 2;
                logicalOffsetX = logicalOffsetY = 11;
            } break;
            case 32: {
                mapscale = 1;
                logicalOffsetX = logicalOffsetY = 16;
            } break;
            case 62:
            default: {
                mapscale = 0;
                logicalOffsetX = logicalOffsetY = 1;
            } break;

        }

        loadedINIFile->setIntValue("BASIC", "MapScale", mapscale);

        int cursorPos = (logicalOffsetY+mapInfo.cursorPos.y) * logicalSizeX + (logicalOffsetX+mapInfo.cursorPos.x);
        loadedINIFile->setIntValue("BASIC", "CursorPos", cursorPos);
        int tacticalPos = (logicalOffsetY+mapInfo.tacticalPos.y) * logicalSizeX + (logicalOffsetX+mapInfo.tacticalPos.x);
        loadedINIFile->setIntValue("BASIC", "TacticalPos", tacticalPos);

        // field, spice bloom and special bloom
        std::string strSpiceBloom = "";
        for(size_t i=0;i<spiceBlooms.size();++i) {
            if(i>0) {
                strSpiceBloom += ",";
            }

            int position = (logicalOffsetY+spiceBlooms[i].y) * logicalSizeX + (logicalOffsetX+spiceBlooms[i].x);
            strSpiceBloom += stringify(position);
        }

        if(!strSpiceBloom.empty()) {
            loadedINIFile->setStringValue("MAP", "Bloom", strSpiceBloom, false);
        } else {
            loadedINIFile->removeKey("MAP", "Bloom");
        }


        std::string strSpecialBloom = "";
        for(size_t i=0;i<specialBlooms.size();++i) {
            if(i>0) {
                strSpecialBloom += ",";
            }

            int position = (logicalOffsetY+specialBlooms[i].y) * logicalSizeX + (logicalOffsetX+specialBlooms[i].x);
            strSpecialBloom += stringify(position);
        }

        if(!strSpecialBloom.empty()) {
            loadedINIFile->setStringValue("MAP", "Special", strSpecialBloom, false);
        } else {
            loadedINIFile->removeKey("MAP", "Special");
        }


        std::string strFieldBloom = "";
        for(size_t i=0;i<spiceFields.size();++i) {
            if(i>0) {
                strFieldBloom += ",";
            }

            int position = (logicalOffsetY+spiceFields[i].y) * logicalSizeX + (logicalOffsetX+spiceFields[i].x);
            strFieldBloom += stringify(position);
        }

        if(!strFieldBloom.empty()) {
            loadedINIFile->setStringValue("MAP", "Field", strFieldBloom, false);
        } else {
            loadedINIFile->removeKey("MAP", "Field");
        }


        loadedINIFile->setIntValue("MAP", "Seed", mapInfo.mapSeed);
    } else {
        logicalSizeX = map.getSizeX();
        //logicalSizeY = map.getSizeY();
        logicalOffsetX = logicalOffsetY = 0;

        loadedINIFile->clearSection("MAP");
        loadedINIFile->setIntValue("MAP", "SizeX", map.getSizeX());
        loadedINIFile->setIntValue("MAP", "SizeY", map.getSizeY());

        for(int y = 0; y < map.getSizeY(); y++) {
            std::string rowKey = strprintf("%.3d", y);

            std::string row = "";
            for(int x = 0; x < map.getSizeX(); x++) {
                switch(map(x,y)) {

                    case Terrain_Dunes: {
                        // Sand dunes
                        row += '^';
                    } break;

                    case Terrain_Spice: {
                        // Spice
                        row += '~';
                    } break;

                    case Terrain_ThickSpice: {
                        // Thick spice
                        row += '+';
                    } break;

                    case Terrain_Rock: {
                        // Rock
                        row += '%';
                    } break;

                    case Terrain_Mountain: {
                        // Mountain
                        row += '@';
                    } break;

                    case Terrain_SpiceBloom: {
                        // Spice Bloom
                        row += 'O';
                    } break;

                    case Terrain_SpecialBloom: {
                        // Special Bloom
                        row += 'Q';
                    } break;

                    case Terrain_Sand:
                    default: {
                        // Normal sand
                        row += '-';
                    } break;
                }
            }

            loadedINIFile->setStringValue("MAP", rowKey, row, false);
        }
    }


    for(int i=1;i<=NUM_HOUSES;i++) {
        loadedINIFile->removeSection("player" + stringify(i));
    }

    std::string house2housename[NUM_HOUSES];
    int currentAnyHouseNumber = 1;
    std::vector<Player>::const_iterator playerIter = players.begin();
    for(int i = 0; playerIter != players.end(); ++playerIter, ++i) {
        if(playerIter->bAnyHouse) {
            house2housename[i] =  "Player" + stringify(currentAnyHouseNumber);
        } else {
            house2housename[i] =  playerIter->name;
        }

        if(playerIter->bActive) {
            if(version < 2) {
                loadedINIFile->setIntValue(house2housename[i], "Quota", playerIter->quota);
                loadedINIFile->setIntValue(house2housename[i], "Credits", playerIter->credits);
                loadedINIFile->setStringValue(house2housename[i], "Brain", playerIter->brain, false);
                loadedINIFile->setIntValue(house2housename[i], "MaxUnit", playerIter->maxunit);
            } else {
                if(playerIter->quota > 0) {
                    loadedINIFile->setIntValue(house2housename[i], "Quota", playerIter->quota);
                } else {
                    loadedINIFile->removeKey(house2housename[i], "Quota");
                }
                loadedINIFile->setIntValue(house2housename[i], "Credits", playerIter->credits);
                loadedINIFile->setStringValue(house2housename[i], "Brain", playerIter->brain, false);

                if(playerIter->bAnyHouse) {
                    currentAnyHouseNumber++;
                }
            }

            if(playerIter->bAnyHouse) {
                // remove corresponding house name
                loadedINIFile->removeSection(playerIter->name);
            }

        } else {
            // remove corresponding house name
            loadedINIFile->removeSection(playerIter->name);
        }
    }

    // remove players that are leftovers
    for(int i=currentAnyHouseNumber;i<NUM_HOUSES;i++) {
        loadedINIFile->removeSection("Player" + stringify(i));
    }


    if(choam.empty()) {
        loadedINIFile->removeSection("CHOAM");
    } else {
        loadedINIFile->clearSection("CHOAM");

        std::map<int,int>::const_iterator iter;
        for(iter = choam.begin(); iter != choam.end(); ++iter) {
            int itemID = iter->first;
            int num = iter->second;

            if(num == 0) {
                num = -1;
            }

            loadedINIFile->setIntValue("CHOAM", getItemNameByID(itemID), num);
        }
    }

    if(teams.empty()) {
        loadedINIFile->removeSection("TEAMS");
    } else {
        loadedINIFile->clearSection("TEAMS");

        // we start at 0 for version 1 maps if we have 16 entries to not overflow the table
        int currentIndex = ((getMapVersion() < 2) && (teams.size() >= 16)) ? 0 : 1;
        std::vector<TeamInfo>::const_iterator iter;
        for(iter = teams.begin(); iter != teams.end(); ++iter, ++currentIndex) {

            std::string value = house2housename[iter->houseID] + "," + getTeamBehaviorNameByID(iter->teamBehavior) + "," + getTeamTypeNameByID(iter->teamType) + "," + stringify(iter->minUnits) + "," + stringify(iter->maxUnits);

            loadedINIFile->setStringValue("TEAMS", stringify(currentIndex), value, false);
        }
    }


    loadedINIFile->clearSection("UNITS");
    std::vector<Unit>::const_iterator unitsIter;
    for(unitsIter = units.begin(); unitsIter != units.end(); ++unitsIter) {
        std::string unitKey = strprintf("ID%.3d", unitsIter->id);

        int position = (logicalOffsetY+unitsIter->position.y) * logicalSizeX + (logicalOffsetX+unitsIter->position.x);

        int angle = (int) unitsIter->angle;

        angle = (((NUM_ANGLES - angle) + 2) % NUM_ANGLES) * 32;

        std::string unitValue = house2housename[unitsIter->house] + "," + getItemNameByID(unitsIter->itemID) + "," + stringify(unitsIter->health)
                                + "," + stringify(position) + "," + stringify(angle) + "," + getAttackModeNameByMode(unitsIter->attackmode);

        loadedINIFile->setStringValue("UNITS", unitKey, unitValue, false);
    }

    loadedINIFile->clearSection("STRUCTURES");
    std::vector<Structure>::const_iterator structuresIter;
    for(structuresIter = structures.begin(); structuresIter != structures.end(); ++structuresIter) {

        int position = (logicalOffsetY+structuresIter->position.y) * logicalSizeX + (logicalOffsetX+structuresIter->position.x);

        if((structuresIter->itemID == Structure_Slab1) || (structuresIter->itemID == Structure_Slab4) || (structuresIter->itemID == Structure_Wall)) {
            std::string structureKey = strprintf("GEN%.3d", position);

            std::string structureValue = house2housename[structuresIter->house] + "," + getItemNameByID(structuresIter->itemID);

            loadedINIFile->setStringValue("STRUCTURES", structureKey, structureValue, false);

        } else {

            std::string structureKey = strprintf("ID%.3d", structuresIter->id);

            std::string structureValue = house2housename[structuresIter->house] + "," + getItemNameByID(structuresIter->itemID) + "," + stringify(structuresIter->health) + "," + stringify(position);

            loadedINIFile->setStringValue("STRUCTURES", structureKey, structureValue, false);
        }
    }

    if(reinforcements.empty()) {
        loadedINIFile->removeSection("REINFORCEMENTS");
    } else {
        loadedINIFile->clearSection("REINFORCEMENTS");

        // we start at 0 for version 1 maps if we have 16 entries to not overflow the table
        int currentIndex = ((getMapVersion() < 2) && (reinforcements.size() >= 16)) ? 0 : 1;
        std::vector<ReinforcementInfo>::const_iterator iter;
        for(iter = reinforcements.begin(); iter != reinforcements.end(); ++iter, ++currentIndex) {

            std::string value = house2housename[iter->houseID] + "," + getItemNameByID(iter->unitID) + "," + getDropLocationNameByID(iter->dropLocation) + "," + stringify(iter->droptime);

            if(iter->bRepeat) {
                value += ",+";
            }

            loadedINIFile->setStringValue("REINFORCEMENTS", stringify(currentIndex), value, false);
        }
    }

    loadedINIFile->saveChangesTo(filepath, getMapVersion() < 2);

    lastSaveName = filepath;
    bChangedSinceLastSave = false;
}

void MapEditor::performMapEdit(int xpos, int ypos, bool bRepeated) {
    switch(currentEditorMode.mode) {
        case EditorMode::EditorMode_Terrain: {
            clearRedoOperations();

            if(!bRepeated) {
                startOperation();
            }

            if(getMapVersion() < 2) {
                // classic map
                if(!bRepeated && map.isInsideMap(xpos, ypos)) {
                    TERRAINTYPE terrainType = currentEditorMode.terrainType;

                    switch(terrainType) {
                        case Terrain_SpiceBloom: {
                            MapEditorTerrainAddSpiceBloomOperation editOperation(xpos, ypos);
                            addUndoOperation(editOperation.perform(this));
                        } break;

                        case Terrain_SpecialBloom: {
                            MapEditorTerrainAddSpecialBloomOperation editOperation(xpos, ypos);
                            addUndoOperation(editOperation.perform(this));
                        } break;

                        case Terrain_Spice: {
                            MapEditorTerrainAddSpiceFieldOperation editOperation(xpos, ypos);
                            addUndoOperation(editOperation.perform(this));
                        } break;

                        default: {
                        } break;
                    }


                }

            } else {
                for(int i=0;i<mapMirror->getSize();i++) {

                    Coord position = mapMirror->getCoord( Coord(xpos, ypos), i);

                    int halfsize = currentEditorMode.pensize/2;
                    for(int y = position.y - halfsize; y <= position.y + halfsize; y++) {
                        for(int x = position.x - halfsize; x <= position.x + halfsize; x++) {
                            if(map.isInsideMap(x, y)) {
                                performTerrainChange(x, y, currentEditorMode.terrainType);
                            }
                        }
                    }

                }
            }

        } break;

        case EditorMode::EditorMode_Structure: {
            if(!bRepeated || currentEditorMode.itemID == Structure_Slab1 || currentEditorMode.itemID == Structure_Wall) {

                Coord structureSize = getStructureSize(currentEditorMode.itemID);

                if(mapMirror->mirroringPossible( Coord(xpos, ypos), structureSize) == false) {
                    return;
                }

                // check if all places are free
                for(int i=0;i<mapMirror->getSize();i++) {
                    Coord position = mapMirror->getCoord( Coord(xpos, ypos), i, structureSize);

                    for(int x = position.x; x < position.x + structureSize.x; x++) {
                        for(int y = position.y; y < position.y + structureSize.y; y++) {
                            if(!map.isInsideMap(x,y) || isTileBlocked(x, y, true, (currentEditorMode.itemID != Structure_Slab1) )) {
                                return;
                            }
                        }
                    }
                }

                clearRedoOperations();

                if(!bRepeated) {
                    startOperation();
                }

                int currentHouse = currentEditorMode.house;
                bool bHouseIsActive = players[currentHouse].bActive;
                for(int i=0;i<mapMirror->getSize();i++) {

                    int nextHouse = HOUSE_INVALID;
                    for(int k = currentHouse; k < currentHouse+NUM_HOUSES;k++) {
                        if(players[k%NUM_HOUSES].bActive == bHouseIsActive) {
                            nextHouse = k;
                            break;
                        }
                    }

                    if(nextHouse != HOUSE_INVALID) {
                        Coord position = mapMirror->getCoord( Coord(xpos, ypos), i, structureSize);

                        MapEditorStructurePlaceOperation placeOperation(position, (HOUSETYPE) (nextHouse%NUM_HOUSES), currentEditorMode.itemID, currentEditorMode.health);

                        addUndoOperation(placeOperation.perform(this));

                        currentHouse = nextHouse + 1;
                    }
                }
            }
        } break;

        case EditorMode::EditorMode_Unit: {
            if(!bRepeated) {

                // first check if all places are free
                for(int i=0;i<mapMirror->getSize();i++) {
                    Coord position = mapMirror->getCoord( Coord(xpos, ypos), i);

                    if(!map.isInsideMap(position.x,position.y) || isTileBlocked(position.x, position.y, false, true)) {
                        return;
                    }
                }

                clearRedoOperations();

                startOperation();


                int currentHouse = currentEditorMode.house;
                bool bHouseIsActive = players[currentHouse].bActive;
                for(int i=0;i<mapMirror->getSize();i++) {

                    int nextHouse = HOUSE_INVALID;
                    for(int k = currentHouse; k < currentHouse+NUM_HOUSES;k++) {
                        if(players[k%NUM_HOUSES].bActive == bHouseIsActive) {
                            nextHouse = k;
                            break;
                        }
                    }

                    if(nextHouse != HOUSE_INVALID) {
                        Coord position = mapMirror->getCoord( Coord(xpos, ypos), i);

                        int angle =  mapMirror->getAngle(currentEditorMode.angle, i);

                        MapEditorUnitPlaceOperation placeOperation(position, (HOUSETYPE) (nextHouse%NUM_HOUSES), currentEditorMode.itemID, currentEditorMode.health, angle, currentEditorMode.attackmode);

                        addUndoOperation(placeOperation.perform(this));
                        currentHouse = nextHouse + 1;
                    }
                }
            }
        } break;

        case EditorMode::EditorMode_TacticalPos: {
            if(!map.isInsideMap(xpos,ypos)) {
                return;
            }

            clearRedoOperations();

            startOperation();

            MapEditorSetTacticalPositionOperation setOperation(xpos,ypos);

            addUndoOperation(setOperation.perform(this));

            setEditorMode(EditorMode());
        } break;

        default: {

        } break;

    }
}

void MapEditor::performTerrainChange(int x, int y, TERRAINTYPE terrainType) {

    MapEditorTerrainEditOperation editOperation(x, y, terrainType);
    addUndoOperation(editOperation.perform(this));

    switch(terrainType) {
        case Terrain_Mountain: {
            if(map.isInsideMap(x-1, y) && (map(x-1,y) != Terrain_Mountain) && (map(x-1,y) != Terrain_Rock))     performTerrainChange(x-1,y,Terrain_Rock);
            if(map.isInsideMap(x, y-1) && (map(x,y-1) != Terrain_Mountain) && (map(x,y-1) != Terrain_Rock))     performTerrainChange(x,y-1,Terrain_Rock);
            if(map.isInsideMap(x+1, y) && (map(x+1,y) != Terrain_Mountain) && (map(x+1,y) != Terrain_Rock))     performTerrainChange(x+1,y,Terrain_Rock);
            if(map.isInsideMap(x, y+1) && (map(x,y+1) != Terrain_Mountain) && (map(x,y+1) != Terrain_Rock))     performTerrainChange(x,y+1,Terrain_Rock);
        } break;

        case Terrain_ThickSpice: {
            if(map.isInsideMap(x-1, y) && (map(x-1,y) != Terrain_ThickSpice) && (map(x-1,y) != Terrain_Spice))     performTerrainChange(x-1,y,Terrain_Spice);
            if(map.isInsideMap(x, y-1) && (map(x,y-1) != Terrain_ThickSpice) && (map(x,y-1) != Terrain_Spice))     performTerrainChange(x,y-1,Terrain_Spice);
            if(map.isInsideMap(x+1, y) && (map(x+1,y) != Terrain_ThickSpice) && (map(x+1,y) != Terrain_Spice))     performTerrainChange(x+1,y,Terrain_Spice);
            if(map.isInsideMap(x, y+1) && (map(x,y+1) != Terrain_ThickSpice) && (map(x,y+1) != Terrain_Spice))     performTerrainChange(x,y+1,Terrain_Spice);
        } break;

        case Terrain_Rock: {
            if(map.isInsideMap(x-1, y) && (map(x-1,y) == Terrain_ThickSpice))     performTerrainChange(x-1,y,Terrain_Spice);
            if(map.isInsideMap(x, y-1) && (map(x,y-1) == Terrain_ThickSpice))     performTerrainChange(x,y-1,Terrain_Spice);
            if(map.isInsideMap(x+1, y) && (map(x+1,y) == Terrain_ThickSpice))     performTerrainChange(x+1,y,Terrain_Spice);
            if(map.isInsideMap(x, y+1) && (map(x,y+1) == Terrain_ThickSpice))     performTerrainChange(x,y+1,Terrain_Spice);
        } break;

        case Terrain_Spice: {
            if(map.isInsideMap(x-1, y) && (map(x-1,y) == Terrain_Mountain))     performTerrainChange(x-1,y,Terrain_Rock);
            if(map.isInsideMap(x, y-1) && (map(x,y-1) == Terrain_Mountain))     performTerrainChange(x,y-1,Terrain_Rock);
            if(map.isInsideMap(x+1, y) && (map(x+1,y) == Terrain_Mountain))     performTerrainChange(x+1,y,Terrain_Rock);
            if(map.isInsideMap(x, y+1) && (map(x,y+1) == Terrain_Mountain))     performTerrainChange(x,y+1,Terrain_Rock);
        } break;

        case Terrain_Sand:
        case Terrain_Dunes:
        case Terrain_SpiceBloom:
        case Terrain_SpecialBloom: {
            if(map.isInsideMap(x-1, y) && (map(x-1,y) == Terrain_Mountain))     performTerrainChange(x-1,y,Terrain_Rock);
            if(map.isInsideMap(x, y-1) && (map(x,y-1) == Terrain_Mountain))     performTerrainChange(x,y-1,Terrain_Rock);
            if(map.isInsideMap(x+1, y) && (map(x+1,y) == Terrain_Mountain))     performTerrainChange(x+1,y,Terrain_Rock);
            if(map.isInsideMap(x, y+1) && (map(x,y+1) == Terrain_Mountain))     performTerrainChange(x,y+1,Terrain_Rock);

            if(map.isInsideMap(x-1, y) && (map(x-1,y) == Terrain_ThickSpice))     performTerrainChange(x-1,y,Terrain_Spice);
            if(map.isInsideMap(x, y-1) && (map(x,y-1) == Terrain_ThickSpice))     performTerrainChange(x,y-1,Terrain_Spice);
            if(map.isInsideMap(x+1, y) && (map(x+1,y) == Terrain_ThickSpice))     performTerrainChange(x+1,y,Terrain_Spice);
            if(map.isInsideMap(x, y+1) && (map(x,y+1) == Terrain_ThickSpice))     performTerrainChange(x,y+1,Terrain_Spice);
        } break;

        default: {
        } break;
    }

}

void MapEditor::drawScreen() {
	// clear whole screen
	SDL_FillRect(screen,NULL,0);

	//the actuall map
	drawMap(screen, screenborder, false);

    pInterface->draw(screen, Point(0,0));
	pInterface->drawOverlay(screen, Point(0,0));

	// Cursor
	drawCursor();

	SDL_Flip(screen);
}

void MapEditor::processInput() {
	SDL_Event event;

	while(SDL_PollEvent(&event)) {

	    // first of all update mouse
		if(event.type == SDL_MOUSEMOTION) {
			SDL_MouseMotionEvent* mouse = &event.motion;
			drawnMouseX = mouse->x;
			drawnMouseY = mouse->y;
		}

	    if(pInterface->hasChildWindow()) {
            pInterface->handleInput(event);
	    } else {
            switch (event.type) {
                case SDL_KEYDOWN:
                {
                    switch(event.key.keysym.sym) {

                        case SDLK_RETURN: {
                            if(SDL_GetModState() & KMOD_ALT) {
                                SDL_WM_ToggleFullScreen(screen);
                            }
                        } break;

                        case SDLK_TAB: {
                            if(SDL_GetModState() & KMOD_ALT) {
                                SDL_WM_IconifyWindow();
                            }
                        } break;

                        case SDLK_F1: {
                            Coord oldCenterCoord = screenborder->getCurrentCenter();
                            currentZoomlevel = 0;
                            screenborder->adjustScreenBorderToMapsize(map.getSizeX(), map.getSizeY());
                            screenborder->setNewScreenCenter(oldCenterCoord);
                        } break;

                        case SDLK_F2: {
                            Coord oldCenterCoord = screenborder->getCurrentCenter();
                            currentZoomlevel = 1;
                            screenborder->adjustScreenBorderToMapsize(map.getSizeX(), map.getSizeY());
                            screenborder->setNewScreenCenter(oldCenterCoord);
                        } break;

                        case SDLK_F3: {
                            Coord oldCenterCoord = screenborder->getCurrentCenter();
                            currentZoomlevel = 2;
                            screenborder->adjustScreenBorderToMapsize(map.getSizeX(), map.getSizeY());
                            screenborder->setNewScreenCenter(oldCenterCoord);
                        } break;

                        case SDLK_p: {
                            if(SDL_GetModState() & KMOD_CTRL) {
                                saveMapshot();
                            }
                        } break;

                        case SDLK_PRINT:
                        case SDLK_SYSREQ: {
                            saveMapshot();
                        } break;

                        case SDLK_z: {
                            if(SDL_GetModState() & KMOD_CTRL) {
                                    pInterface->onUndo();
                            }
                        } break;

                        case SDLK_y: {
                            if(SDL_GetModState() & KMOD_CTRL) {
                                    pInterface->onRedo();
                            }
                        } break;

                        default:
                            break;
                    }

                } break;

                case SDL_KEYUP:
                {
                    switch(event.key.keysym.sym) {
                        case SDLK_ESCAPE: {
                            // quiting
                            pInterface->onQuit();
                        } break;

                        case SDLK_DELETE:
                        case SDLK_BACKSPACE: {

                            // check units first
                            if(selectedUnitID != INVALID) {
                                clearRedoOperations();
                                startOperation();

                                std::vector<int> selectedUnits = getMirrorUnits(selectedUnitID);

                                for(size_t i=0;i<selectedUnits.size();i++) {
                                    MapEditorRemoveUnitOperation removeOperation(selectedUnits[i]);
                                    addUndoOperation(removeOperation.perform(this));
                                }
                                selectedUnitID = INVALID;

                                pInterface->deselectAll();
                            } else if(selectedStructureID != INVALID) {
                                // We only try deleting structures if we had not yet deleted a unit (e.g. a unit on concrete)
                                clearRedoOperations();
                                startOperation();

                                std::vector<int> selectedStructures = getMirrorStructures(selectedStructureID);

                                for(size_t i=0;i<selectedStructures.size();i++) {
                                    MapEditorRemoveStructureOperation removeOperation(selectedStructures[i]);
                                    addUndoOperation(removeOperation.perform(this));
                                }
                                selectedStructureID = INVALID;

                                pInterface->deselectAll();
                            } else if(selectedMapItemCoord.isValid()) {
                                std::vector<Coord>::iterator iter = std::find(specialBlooms.begin(), specialBlooms.end(), selectedMapItemCoord);

                                if(iter != specialBlooms.end()) {
                                    clearRedoOperations();
                                    startOperation();
                                    MapEditorTerrainRemoveSpecialBloomOperation removeOperation(iter->x, iter->y);
                                    addUndoOperation(removeOperation.perform(this));

                                    selectedMapItemCoord.invalidate();
                                } else {
                                    iter = std::find(spiceBlooms.begin(), spiceBlooms.end(), selectedMapItemCoord);

                                    if(iter != spiceBlooms.end()) {
                                        clearRedoOperations();
                                        startOperation();
                                        MapEditorTerrainRemoveSpiceBloomOperation removeOperation(iter->x, iter->y);
                                        addUndoOperation(removeOperation.perform(this));

                                        selectedMapItemCoord.invalidate();
                                    } else {
                                        iter = std::find(spiceFields.begin(), spiceFields.end(), selectedMapItemCoord);

                                        if(iter != spiceBlooms.end()) {
                                            clearRedoOperations();
                                            startOperation();
                                            MapEditorTerrainRemoveSpiceFieldOperation removeOperation(iter->x, iter->y);
                                            addUndoOperation(removeOperation.perform(this));

                                            selectedMapItemCoord.invalidate();
                                        }
                                    }
                                }
                            }

                        } break;

                        default:
                            break;
                    }
                } break;

                case SDL_MOUSEMOTION:
                {
                    pInterface->handleMouseMovement(drawnMouseX,drawnMouseY);

                    if(bLeftMousePressed) {
                        if(screenborder->isScreenCoordInsideMap(drawnMouseX, drawnMouseY) == true) {
                            //if mouse is not over side bar

                            int xpos = screenborder->screen2MapX(drawnMouseX);
                            int ypos = screenborder->screen2MapY(drawnMouseY);

                            if((xpos != lastTerrainEditPosX) || (ypos != lastTerrainEditPosY)) {
                                performMapEdit(xpos, ypos, true);
                            }
                        }
                    }
                } break;

                case SDL_MOUSEBUTTONDOWN:
                {
                    SDL_MouseButtonEvent* mouse = &event.button;

                    switch(mouse->button) {
                        case SDL_BUTTON_LEFT: {
                            if(pInterface->handleMouseLeft(mouse->x, mouse->y, true) == false) {

                                bLeftMousePressed = true;

                                if(screenborder->isScreenCoordInsideMap(mouse->x, mouse->y) == true) {
                                    //if mouse is not over side bar

                                    int xpos = screenborder->screen2MapX(mouse->x);
                                    int ypos = screenborder->screen2MapY(mouse->y);

                                    performMapEdit(xpos, ypos, false);
                                }
                            }

                        } break;

                        case SDL_BUTTON_RIGHT: {
                            if(pInterface->handleMouseRight(mouse->x, mouse->y, true) == false) {

                                if(screenborder->isScreenCoordInsideMap(mouse->x, mouse->y) == true) {
                                    //if mouse is not over side bar
                                    setEditorMode(EditorMode());
                                    pInterface->deselectAll();
                                }
                            }
                        } break;

                        case SDL_BUTTON_WHEELUP: {
                            if(screenborder->isScreenCoordInsideMap(mouse->x, mouse->y) == true) {
                                //if mouse is not over side bar
                                int xpos = screenborder->screen2MapX(mouse->x);
                                int ypos = screenborder->screen2MapY(mouse->y);

                                std::vector<Unit>::iterator uIter;
                                for(uIter = units.begin(); uIter != units.end(); ++uIter) {
                                    Coord position = uIter->position;

                                    if((position.x == xpos) && (position.y == ypos)) {
                                        pInterface->onUnitRotateLeft(uIter->id);
                                        break;
                                    }
                                }
                            }

                            pInterface->handleMouseWheel(mouse->x, mouse->y,true);
                        } break;

                        case SDL_BUTTON_WHEELDOWN: {
                            if(screenborder->isScreenCoordInsideMap(mouse->x, mouse->y) == true) {
                                //if mouse is not over side bar
                                int xpos = screenborder->screen2MapX(mouse->x);
                                int ypos = screenborder->screen2MapY(mouse->y);

                                std::vector<Unit>::iterator uIter;
                                for(uIter = units.begin(); uIter != units.end(); ++uIter) {
                                    Coord position = uIter->position;

                                    if((position.x == xpos) && (position.y == ypos)) {
                                        pInterface->onUnitRotateRight(uIter->id);
                                        break;
                                    }
                                }
                            }

                            pInterface->handleMouseWheel(mouse->x, mouse->y,false);
                        } break;
                    }
                } break;

                case SDL_MOUSEBUTTONUP:
                {
                    SDL_MouseButtonEvent* mouse = &event.button;

                    switch(mouse->button) {
                        case SDL_BUTTON_LEFT: {

                            pInterface->handleMouseLeft(mouse->x, mouse->y, false);

                            if(bLeftMousePressed) {

                                bLeftMousePressed = false;
                                lastTerrainEditPosX = -1;
                                lastTerrainEditPosY = -1;

                                if(currentEditorMode.mode == EditorMode::EditorMode_Selection) {
                                    if(screenborder->isScreenCoordInsideMap(mouse->x, mouse->y) == true) {
                                        //if mouse is not over side bar

                                        int xpos = screenborder->screen2MapX(mouse->x);
                                        int ypos = screenborder->screen2MapY(mouse->y);

                                        selectedUnitID = INVALID;
                                        selectedStructureID = INVALID;
                                        selectedMapItemCoord.invalidate();

                                        bool bUnitSelected = false;

                                        std::vector<Unit>::iterator uIter;
                                        for(uIter = units.begin(); uIter != units.end(); ++uIter) {
                                            Coord position = uIter->position;

                                            if((position.x == xpos) && (position.y == ypos)) {
                                                selectedUnitID = uIter->id;
                                                bUnitSelected = true;
                                                pInterface->onObjectSelected();
                                                mapInfo.cursorPos = position;
                                                break;
                                            }
                                        }

                                        bool bStructureSelected = false;

                                        std::vector<Structure>::iterator sIter;
                                        for(sIter = structures.begin(); sIter != structures.end(); ++sIter) {
                                            Coord position = sIter->position;
                                            Coord structureSize = getStructureSize(sIter->itemID);

                                            if(!bUnitSelected && (xpos >= position.x) && (xpos < position.x+structureSize.x) && (ypos >= position.y) && (ypos < position.y+structureSize.y)) {
                                                selectedStructureID = sIter->id;
                                                bStructureSelected = true;
                                                pInterface->onObjectSelected();
                                                mapInfo.cursorPos = position;
                                                break;
                                            }
                                        }

                                        if(!bUnitSelected && !bStructureSelected) {
                                            pInterface->deselectAll();

                                            // find map items (spice bloom, special bloom or spice field)
                                            if( (std::find(spiceBlooms.begin(), spiceBlooms.end(), Coord(xpos,ypos)) != spiceBlooms.end())
                                                || (std::find(specialBlooms.begin(), specialBlooms.end(), Coord(xpos,ypos)) != specialBlooms.end())
                                                || (std::find(spiceFields.begin(), spiceFields.end(), Coord(xpos,ypos)) != spiceFields.end())) {
                                                selectedMapItemCoord = Coord(xpos, ypos);
                                            }
                                        }


                                    }
                                }
                            }
                        } break;

                        case SDL_BUTTON_RIGHT: {
                            pInterface->handleMouseRight(mouse->x, mouse->y, false);
                        } break;
                    }
                } break;

                case SDL_QUIT:
                {
                    bQuitEditor = true;
                } break;
            }
	    }
	}

    if((pInterface->hasChildWindow() == false) && (SDL_GetAppState() & SDL_APPMOUSEFOCUS)) {
        Uint8 *keystate = SDL_GetKeyState(NULL);
        scrollDownMode =  (drawnMouseY >= screen->h-1-SCROLLBORDER) || keystate[SDLK_DOWN];
        scrollLeftMode = (drawnMouseX <= SCROLLBORDER) || keystate[SDLK_LEFT];
        scrollRightMode = (drawnMouseX >= screen->w-1-SCROLLBORDER) || keystate[SDLK_RIGHT];
        scrollUpMode = (drawnMouseY <= SCROLLBORDER) || keystate[SDLK_UP];

        if(scrollLeftMode && scrollRightMode) {
            // do nothing
        } else if(scrollLeftMode) {
            scrollLeftMode = screenborder->scrollLeft();
        } else if(scrollRightMode) {
            scrollRightMode = screenborder->scrollRight();
        }

        if(scrollDownMode && scrollUpMode) {
            // do nothing
        } else if(scrollDownMode) {
            scrollDownMode = screenborder->scrollDown();
        } else if(scrollUpMode) {
            scrollUpMode = screenborder->scrollUp();
        }
    } else {
        scrollDownMode = false;
        scrollLeftMode = false;
        scrollRightMode = false;
        scrollUpMode = false;
    }
}

void MapEditor::drawCursor() {

    if(!(SDL_GetAppState() & SDL_APPMOUSEFOCUS)) {
        return;
    }

	SDL_Surface* pCursor = NULL;
    SDL_Rect dest = { drawnMouseX, drawnMouseY, 0, 0};
	if(scrollLeftMode || scrollRightMode || scrollUpMode || scrollDownMode) {
        if(scrollLeftMode && !scrollRightMode) {
	        pCursor = pGFXManager->getUIGraphic(UI_CursorLeft);
	        dest.y -= 5;
	    } else if(scrollRightMode && !scrollLeftMode) {
            pCursor = pGFXManager->getUIGraphic(UI_CursorRight);
	        dest.x -= pCursor->w / 2;
	        dest.y -= 5;
	    }

        if(pCursor == NULL) {
            if(scrollUpMode && !scrollDownMode) {
                pCursor = pGFXManager->getUIGraphic(UI_CursorUp);
                dest.x -= 5;
            } else if(scrollDownMode && !scrollUpMode) {
                pCursor = pGFXManager->getUIGraphic(UI_CursorDown);
                dest.x -= 5;
                dest.y -= pCursor->h / 2;
            } else {
                pCursor = pGFXManager->getUIGraphic(UI_CursorNormal);
            }
        }
	} else {
	    pCursor = pGFXManager->getUIGraphic(UI_CursorNormal);

	    if((drawnMouseX < gameBarPos.x) && (drawnMouseY > topBarPos.h) && (currentMirrorMode != MirrorModeNone) && (pInterface->hasChildWindow() == false)) {

            SDL_Surface* pMirrorIcon = NULL;
            switch(currentMirrorMode) {
                case MirrorModeHorizontal:  pMirrorIcon = pGFXManager->getUIGraphic(UI_MapEditor_MirrorHorizontalIcon);  break;
                case MirrorModeVertical:    pMirrorIcon = pGFXManager->getUIGraphic(UI_MapEditor_MirrorVerticalIcon);    break;
                case MirrorModeBoth:        pMirrorIcon = pGFXManager->getUIGraphic(UI_MapEditor_MirrorBothIcon);        break;
                case MirrorModePoint:       pMirrorIcon = pGFXManager->getUIGraphic(UI_MapEditor_MirrorPointIcon);       break;
                default:                    pMirrorIcon = pGFXManager->getUIGraphic(UI_MapEditor_MirrorNoneIcon);       break;
            }

            SDL_Rect dest2 = { drawnMouseX + 5, drawnMouseY + 5, pMirrorIcon->w, pMirrorIcon->h};
            SDL_BlitSurface(pMirrorIcon, NULL, screen, &dest2);
	    }
	}

    dest.w = pCursor->w;
    dest.h = pCursor->h;

	if(SDL_BlitSurface(pCursor, NULL, screen, &dest) != 0) {
        fprintf(stderr,"MapEditor::drawCursor(): %s\n", SDL_GetError());
	}
}

TERRAINTYPE MapEditor::getTerrain(int x, int y) {
    TERRAINTYPE terrainType = map(x,y);

    if(map(x,y) == Terrain_Sand) {
        if(std::find(spiceFields.begin(), spiceFields.end(), Coord(x,y)) != spiceFields.end()) {
            terrainType = Terrain_ThickSpice;
        } else if(std::find_if(spiceFields.begin(), spiceFields.end(), CoordDistance(Coord(x,y),5)) != spiceFields.end()) {
            terrainType = Terrain_Spice;
        }
    }

    // check for classic map items (spice blooms, special blooms)
    if(std::find(spiceBlooms.begin(), spiceBlooms.end(), Coord(x,y)) != spiceBlooms.end()) {
        terrainType = Terrain_SpiceBloom;
    }

    if(std::find(specialBlooms.begin(), specialBlooms.end(), Coord(x,y)) != specialBlooms.end()) {
        terrainType = Terrain_SpecialBloom;
    }

    return terrainType;
}

void MapEditor::drawMap(SDL_Surface* pScreen, ScreenBorder* pScreenborder, bool bCompleteMap) {
    int zoomedTilesize = world2zoomedWorld(TILESIZE);

    Coord TopLeftTile = pScreenborder->getTopLeftTile();
    Coord BottomRightTile = pScreenborder->getBottomRightTile();

    // extend the view a little bit to avoid graphical glitches
    TopLeftTile.x = std::max(0, TopLeftTile.x - 1);
    TopLeftTile.y = std::max(0, TopLeftTile.y - 1);
    BottomRightTile.x = std::min(map.getSizeX()-1, BottomRightTile.x + 1);
    BottomRightTile.y = std::min(map.getSizeY()-1, BottomRightTile.y + 1);

    // Load Terrain Surface
	SDL_Surface* TerrainSprite = pGFXManager->getObjPic(ObjPic_Terrain)[currentZoomlevel];

    /* draw ground */
	for(int y = TopLeftTile.y; y <= BottomRightTile.y; y++) {
		for(int x = TopLeftTile.x; x <= BottomRightTile.x; x++) {

		    int tile;

            switch(getTerrain(x,y)) {
                case Terrain_Slab: {
                    tile = Tile::TerrainTile_Slab;
                } break;

                case Terrain_Sand: {
                    tile = Tile::TerrainTile_Sand;
                } break;

                case Terrain_Rock: {
                    //determine which surounding tiles are rock
                    bool up = (y-1 < 0) || (getTerrain(x, y-1) == Terrain_Rock) || (getTerrain(x, y-1) == Terrain_Slab) || (getTerrain(x, y-1) == Terrain_Mountain);
                    bool right = (x+1 >= map.getSizeX()) || (getTerrain(x+1, y) == Terrain_Rock) || (getTerrain(x+1, y) == Terrain_Slab) || (getTerrain(x+1, y) == Terrain_Mountain);
                    bool down = (y+1 >= map.getSizeY()) || (getTerrain(x, y+1) == Terrain_Rock) || (getTerrain(x, y+1) == Terrain_Slab) || (getTerrain(x, y+1) == Terrain_Mountain);
                    bool left = (x-1 < 0) || (getTerrain(x-1, y) == Terrain_Rock) || (getTerrain(x-1, y) == Terrain_Slab) || (getTerrain(x-1, y) == Terrain_Mountain);

                    tile = Tile::TerrainTile_Rock + (up | (right << 1) | (down << 2) | (left << 3));
                } break;

                case Terrain_Dunes: {
                    //determine which surounding tiles are dunes
                    bool up = (y-1 < 0) || (getTerrain(x, y-1) == Terrain_Dunes);
                    bool right = (x+1 >= map.getSizeX()) || (getTerrain(x+1, y) == Terrain_Dunes);
                    bool down = (y+1 >= map.getSizeY()) || (getTerrain(x, y+1) == Terrain_Dunes);
                    bool left = (x-1 < 0) || (getTerrain(x-1, y) == Terrain_Dunes);

                    tile = Tile::TerrainTile_Dunes + (up | (right << 1) | (down << 2) | (left << 3));
                } break;

                case Terrain_Mountain: {
                    //determine which surounding tiles are mountains
                    bool up = (y-1 < 0) || (getTerrain(x, y-1) == Terrain_Mountain);
                    bool right = (x+1 >= map.getSizeX()) || (getTerrain(x+1, y) == Terrain_Mountain);
                    bool down = (y+1 >= map.getSizeY()) || (getTerrain(x, y+1) == Terrain_Mountain);
                    bool left = (x-1 < 0) || (getTerrain(x-1, y) == Terrain_Mountain);

                    tile = Tile::TerrainTile_Mountain + (up | (right << 1) | (down << 2) | (left << 3));
                } break;

                case Terrain_Spice: {
                    //determine which surounding tiles are spice
                    bool up = (y-1 < 0) || (getTerrain(x, y-1) == Terrain_Spice) || (getTerrain(x, y-1) == Terrain_ThickSpice);
                    bool right = (x+1 >= map.getSizeX()) || (getTerrain(x+1, y) == Terrain_Spice) || (getTerrain(x+1, y) == Terrain_ThickSpice);
                    bool down = (y+1 >= map.getSizeY()) || (getTerrain(x, y+1) == Terrain_Spice) || (getTerrain(x, y+1) == Terrain_ThickSpice);
                    bool left = (x-1 < 0) || (getTerrain(x-1, y) == Terrain_Spice) || (getTerrain(x-1, y) == Terrain_ThickSpice);

                    tile = Tile::TerrainTile_Spice + (up | (right << 1) | (down << 2) | (left << 3));
                } break;

                case Terrain_ThickSpice: {
                    //determine which surounding tiles are thick spice
                    bool up = (y-1 < 0) || (getTerrain(x, y-1) == Terrain_ThickSpice);
                    bool right = (x+1 >= map.getSizeX()) || (getTerrain(x+1, y) == Terrain_ThickSpice);
                    bool down = (y+1 >= map.getSizeY()) || (getTerrain(x, y+1) == Terrain_ThickSpice);
                    bool left = (x-1 < 0) || (getTerrain(x-1, y) == Terrain_ThickSpice);

                    tile = Tile::TerrainTile_ThickSpice + (up | (right << 1) | (down << 2) | (left << 3));
                } break;

                case Terrain_SpiceBloom: {
                    tile = Tile::TerrainTile_SpiceBloom;
                } break;

                case Terrain_SpecialBloom: {
                    tile = Tile::TerrainTile_SpecialBloom;
                } break;

                default: {
                    throw std::runtime_error("MapEditor::DrawMap(): Invalid terrain type");
                } break;
            }

            //draw map[x][y]
            SDL_Rect source = { tile*zoomedTilesize, 0,
                                zoomedTilesize, zoomedTilesize };
            SDL_Rect drawLocation = {   pScreenborder->world2screenX(x*TILESIZE), pScreenborder->world2screenY(y*TILESIZE),
                                        zoomedTilesize, zoomedTilesize };
			SDL_BlitSurface(TerrainSprite, &source, pScreen, &drawLocation);
		}
	}


    std::vector<int> selectedStructures = getMirrorStructures(selectedStructureID);

	std::vector<Structure>::const_iterator sIter;
	for(sIter = structures.begin(); sIter != structures.end(); ++sIter) {

	    Coord position = sIter->position;

        SDL_Rect selectionDest;
	    if(sIter->itemID == Structure_Slab1) {
            // Load Terrain Surface
            SDL_Surface* TerrainSprite = pGFXManager->getObjPic(ObjPic_Terrain)[currentZoomlevel];

            SDL_Rect source = { Tile::TerrainTile_Slab * world2zoomedWorld(TILESIZE), 0, world2zoomedWorld(TILESIZE), world2zoomedWorld(TILESIZE)};
            SDL_Rect dest = { pScreenborder->world2screenX(position.x*TILESIZE), pScreenborder->world2screenY(position.y*TILESIZE), world2zoomedWorld(TILESIZE), world2zoomedWorld(TILESIZE)};

            SDL_BlitSurface(TerrainSprite, &source, pScreen, &dest);

            selectionDest = dest;
	    } else if(sIter->itemID == Structure_Slab4) {
            // Load Terrain Surface
            SDL_Surface* TerrainSprite = pGFXManager->getObjPic(ObjPic_Terrain)[currentZoomlevel];

            for(int y = position.y; y < position.y+2; y++) {
                for(int x = position.x; x < position.x+2; x++) {
                    SDL_Rect source = { Tile::TerrainTile_Slab * world2zoomedWorld(TILESIZE), 0, world2zoomedWorld(TILESIZE), world2zoomedWorld(TILESIZE)};
                    SDL_Rect dest = { pScreenborder->world2screenX(x*TILESIZE), pScreenborder->world2screenY(y*TILESIZE), world2zoomedWorld(TILESIZE), world2zoomedWorld(TILESIZE)};

                    SDL_BlitSurface(TerrainSprite, &source, pScreen, &dest);
                }
            }

            selectionDest.x = pScreenborder->world2screenX(position.x*TILESIZE);
            selectionDest.y = pScreenborder->world2screenY(position.y*TILESIZE);
            selectionDest.w = world2zoomedWorld(2*TILESIZE);
            selectionDest.h = world2zoomedWorld(2*TILESIZE);
	    } else if(sIter->itemID == Structure_Wall) {
	        std::vector<Structure>::const_iterator sIter2;

	        bool left = false;
	        bool down = false;
	        bool right = false;
	        bool up = false;
	        for(sIter2 = structures.begin(); sIter2 != structures.end(); ++sIter2) {
	            if(sIter2->itemID == Structure_Wall) {
                    if((sIter2->position.x == position.x - 1) && (sIter2->position.y == position.y))  left = true;
                    if((sIter2->position.x == position.x) && (sIter2->position.y == position.y + 1))  down = true;
                    if((sIter2->position.x == position.x + 1) && (sIter2->position.y == position.y))  right = true;
                    if((sIter2->position.x == position.x) && (sIter2->position.y == position.y - 1))  up = true;
                }
	        }

            int maketile = 0;
            if((left == true) && (right == true) && (up == true) && (down == true)) {
                maketile = Wall::Wall_Full; //solid wall
            } else if((left == false) && (right == true) && (up == true) && (down == true)) {
                maketile = Wall::Wall_UpDownRight; //missing left edge
            } else if((left == true) && (right == false)&& (up == true) && (down == true)) {
                maketile = Wall::Wall_UpDownLeft; //missing right edge
            } else if((left == true) && (right == true) && (up == false) && (down == true)) {
                maketile = Wall::Wall_DownLeftRight; //missing top edge
            } else if((left == true) && (right == true) && (up == true) && (down == false)) {
                maketile = Wall::Wall_UpLeftRight; //missing bottom edge
            } else if((left == false) && (right == true) && (up == false) && (down == true)) {
                maketile = Wall::Wall_DownRight; //missing top left edge
            } else if((left == true) && (right == false) && (up == true) && (down == false)) {
                maketile = Wall::Wall_UpLeft; //missing bottom right edge
            } else if((left == true) && (right == false) && (up == false) && (down == true)) {
                maketile = Wall::Wall_DownLeft; //missing top right edge
            } else if((left == false) && (right == true) && (up == true) && (down == false)) {
                maketile = Wall::Wall_UpRight; //missing bottom left edge
            } else if((left == true) && (right == false) && (up == false) && (down == false)) {
                maketile = Wall::Wall_LeftRight; //missing above, right and below
            } else if((left == false) && (right == true) && (up == false) && (down == false)) {
                maketile = Wall::Wall_LeftRight; //missing above, left and below
            } else if((left == false) && (right == false) && (up == true) && (down == false)) {
                maketile = Wall::Wall_UpDown; //only up
            } else if((left == false) && (right == false) && (up == false) && (down == true)) {
                maketile = Wall::Wall_UpDown; //only down
            } else if((left == true) && (right == true) && (up == false) && (down == false)) {
                maketile = Wall::Wall_LeftRight; //missing above and below
            } else if((left == false) && (right == false) && (up == true) && (down == true)) {
                maketile = Wall::Wall_UpDown; //missing left and right
            } else if((left == false) && (right == false) && (up == false) && (down == false)) {
                maketile = Wall::Wall_Standalone; //missing left and right
            }

            // Load Wall Surface
            SDL_Surface* WallSprite = pGFXManager->getObjPic(ObjPic_Wall)[currentZoomlevel];

            SDL_Rect source = { maketile * world2zoomedWorld(TILESIZE), 0, world2zoomedWorld(TILESIZE), world2zoomedWorld(TILESIZE)};
            SDL_Rect dest = { pScreenborder->world2screenX(position.x*TILESIZE), pScreenborder->world2screenY(position.y*TILESIZE), world2zoomedWorld(TILESIZE), world2zoomedWorld(TILESIZE)};

            SDL_BlitSurface(WallSprite, &source, pScreen, &dest);

            selectionDest = dest;
	    } else {

            int objectPic = 0;
            switch(sIter->itemID) {
                case Structure_Barracks:            objectPic = ObjPic_Barracks;            break;
                case Structure_ConstructionYard:    objectPic = ObjPic_ConstructionYard;    break;
                case Structure_GunTurret:           objectPic = ObjPic_GunTurret;           break;
                case Structure_HeavyFactory:        objectPic = ObjPic_HeavyFactory;        break;
                case Structure_HighTechFactory:     objectPic = ObjPic_HighTechFactory;     break;
                case Structure_IX:                  objectPic = ObjPic_IX;                  break;
                case Structure_LightFactory:        objectPic = ObjPic_LightFactory;        break;
                case Structure_Palace:              objectPic = ObjPic_Palace;              break;
                case Structure_Radar:               objectPic = ObjPic_Radar;               break;
                case Structure_Refinery:            objectPic = ObjPic_Refinery;            break;
                case Structure_RepairYard:          objectPic = ObjPic_RepairYard;          break;
                case Structure_RocketTurret:        objectPic = ObjPic_RocketTurret;        break;
                case Structure_Silo:                objectPic = ObjPic_Silo;                break;
                case Structure_StarPort:            objectPic = ObjPic_Starport;            break;
                case Structure_Wall:                objectPic = ObjPic_Wall;                break;
                case Structure_WindTrap:            objectPic = ObjPic_Windtrap;            break;
                case Structure_WOR:                 objectPic = ObjPic_WOR;                 break;
                default:                            objectPic = 0;                          break;
            }

            SDL_Surface* ObjectSprite = pGFXManager->getObjPic(objectPic, sIter->house)[currentZoomlevel];

            Coord frameSize = world2zoomedWorld(getStructureSize(sIter->itemID)*TILESIZE);

            SDL_Rect source = { frameSize.x*2, 0, frameSize.x, frameSize.y };
            SDL_Rect dest = { pScreenborder->world2screenX(position.x*TILESIZE), pScreenborder->world2screenY(position.y*TILESIZE), frameSize.x, frameSize.y };

            SDL_BlitSurface(ObjectSprite, &source, pScreen, &dest);

            selectionDest = dest;
	    }

	    // draw selection frame
        if(!bCompleteMap && (std::find(selectedStructures.begin(), selectedStructures.end(), sIter->id) != selectedStructures.end()) ) {
            //now draw the selection box thing, with parts at all corners of structure
            if(!SDL_MUSTLOCK(pScreen) || (SDL_LockSurface(pScreen) == 0)) {
                // top left bit
                for(int i=0;i<=currentZoomlevel;i++) {
                    drawHLineNoLock(pScreen,selectionDest.x+i, selectionDest.y+i, selectionDest.x+(currentZoomlevel+1)*3, COLOR_WHITE);
                    drawVLineNoLock(pScreen,selectionDest.x+i, selectionDest.y+i, selectionDest.y+(currentZoomlevel+1)*3, COLOR_WHITE);
                }

                // top right bit
                for(int i=0;i<=currentZoomlevel;i++) {
                    drawHLineNoLock(pScreen,selectionDest.x + selectionDest.w-1 - i, selectionDest.y+i, selectionDest.x + selectionDest.w-1 - (currentZoomlevel+1)*3, COLOR_WHITE);
                    drawVLineNoLock(pScreen,selectionDest.x + selectionDest.w-1 - i, selectionDest.y+i, selectionDest.y+(currentZoomlevel+1)*3, COLOR_WHITE);
                }

                // bottom left bit
                for(int i=0;i<=currentZoomlevel;i++) {
                    drawHLineNoLock(pScreen,selectionDest.x+i, selectionDest.y + selectionDest.h-1 - i, selectionDest.x+(currentZoomlevel+1)*3, COLOR_WHITE);
                    drawVLineNoLock(pScreen,selectionDest.x+i, selectionDest.y + selectionDest.h-1 - i, selectionDest.y + selectionDest.h-1 - (currentZoomlevel+1)*3, COLOR_WHITE);
                }

                // bottom right bit
                for(int i=0;i<=currentZoomlevel;i++) {
                    drawHLineNoLock(pScreen,selectionDest.x + selectionDest.w-1 - i, selectionDest.y + selectionDest.h-1 - i, selectionDest.x + selectionDest.w-1 - (currentZoomlevel+1)*3, COLOR_WHITE);
                    drawVLineNoLock(pScreen,selectionDest.x + selectionDest.w-1 - i, selectionDest.y + selectionDest.h-1 - i, selectionDest.y + selectionDest.h-1 - (currentZoomlevel+1)*3, COLOR_WHITE);
                }

                if(SDL_MUSTLOCK(pScreen)) {
                    SDL_UnlockSurface(pScreen);
                }
            }
        }

	}

	std::vector<Unit>::const_iterator uIter;
	for(uIter = units.begin(); uIter != units.end(); ++uIter) {

	    Coord position = uIter->position;

        const Coord tankTurretOffset[] =    {   Coord(0, 0),
                                                Coord(0, 0),
                                                Coord(0, 0),
                                                Coord(0, 0),
                                                Coord(0, 0),
                                                Coord(0, 0),
                                                Coord(0, 0),
                                                Coord(0, 0)
                                            };

        const Coord siegeTankTurretOffset[] =   {   Coord(8, -12),
                                                    Coord(0, -20),
                                                    Coord(0, -20),
                                                    Coord(-4, -20),
                                                    Coord(-8, -12),
                                                    Coord(-8, -4),
                                                    Coord(-4, -12),
                                                    Coord(8, -4)
                                            };

        const Coord sonicTankTurretOffset[] =   {   Coord(0, -8),
                                                    Coord(0, -8),
                                                    Coord(0, -8),
                                                    Coord(0, -8),
                                                    Coord(0, -8),
                                                    Coord(0, -8),
                                                    Coord(0, -8),
                                                    Coord(0, -8)
                                                };

        const Coord launcherTurretOffset[] =    {   Coord(0, -12),
                                                    Coord(0, -8),
                                                    Coord(0, -8),
                                                    Coord(0, -8),
                                                    Coord(0, -12),
                                                    Coord(0, -8),
                                                    Coord(0, -8),
                                                    Coord(0, -8)
                                                };

        const Coord devastatorTurretOffset[] =  {
                                                    Coord(8, -16),
                                                    Coord(-4, -12),
                                                    Coord(0, -16),
                                                    Coord(4, -12),
                                                    Coord(-8, -16),
                                                    Coord(0, -12),
                                                    Coord(-4, -12),
                                                    Coord(0, -12)
                                                };



	    int objectPicBase = 0;
	    int framesX = NUM_ANGLES;
	    int framesY = 1;
	    int objectPicGun = -1;
	    const Coord* gunOffset = NULL;

        switch(uIter->itemID) {
            case Unit_Carryall:         objectPicBase = ObjPic_Carryall;        framesY = 2;                                                                    break;
            case Unit_Devastator:       objectPicBase = ObjPic_Devastator_Base; objectPicGun = ObjPic_Devastator_Gun;   gunOffset = devastatorTurretOffset;     break;
            case Unit_Deviator:         objectPicBase = ObjPic_Tank_Base;       objectPicGun = ObjPic_Launcher_Gun;     gunOffset = launcherTurretOffset;       break;
            case Unit_Frigate:          objectPicBase = ObjPic_Frigate;                                                                                         break;
            case Unit_Harvester:        objectPicBase = ObjPic_Harvester;                                                                                       break;
            case Unit_Soldier:          objectPicBase = ObjPic_Soldier;         framesX = 4;    framesY = 3;                                                    break;
            case Unit_Launcher:         objectPicBase = ObjPic_Tank_Base;       objectPicGun = ObjPic_Launcher_Gun;     gunOffset = launcherTurretOffset;       break;
            case Unit_MCV:              objectPicBase = ObjPic_MCV;                                                                                             break;
            case Unit_Ornithopter:      objectPicBase = ObjPic_Ornithopter;     framesY = 3;                                                                    break;
            case Unit_Quad:             objectPicBase = ObjPic_Quad;                                                                                            break;
            case Unit_Saboteur:         objectPicBase = ObjPic_Saboteur;        framesX = 4;    framesY = 3;                                                    break;
            case Unit_Sandworm:         objectPicBase = ObjPic_Sandworm;        framesX = 1;    framesY = 9;                                                    break;
            case Unit_SiegeTank:        objectPicBase = ObjPic_Siegetank_Base;  objectPicGun = ObjPic_Siegetank_Gun;    gunOffset = siegeTankTurretOffset;      break;
            case Unit_SonicTank:        objectPicBase = ObjPic_Tank_Base;       objectPicGun = ObjPic_Sonictank_Gun;    gunOffset = sonicTankTurretOffset;      break;
            case Unit_Tank:             objectPicBase = ObjPic_Tank_Base;       objectPicGun = ObjPic_Tank_Gun;         gunOffset = tankTurretOffset;           break;
            case Unit_Trike:            objectPicBase = ObjPic_Trike;                                                                                           break;
            case Unit_RaiderTrike:      objectPicBase = ObjPic_Trike;                                                                                           break;
            case Unit_Trooper:          objectPicBase = ObjPic_Trooper;         framesX = 4;    framesY = 3;                                                    break;
            case Unit_Special:          objectPicBase = ObjPic_Devastator_Base; objectPicGun = ObjPic_Devastator_Gun;   gunOffset = devastatorTurretOffset;     break;
            case Unit_Infantry:         objectPicBase = ObjPic_Infantry;         framesX = 4;    framesY = 4;                                                   break;
            case Unit_Troopers:         objectPicBase = ObjPic_Troopers;         framesX = 4;    framesY = 4;                                                   break;
        }

        SDL_Surface* pObjectSprite = pGFXManager->getObjPic(objectPicBase, uIter->house)[currentZoomlevel];

        Coord frameSize = Coord(pObjectSprite->w/framesX, pObjectSprite->h/framesY);

        int angle = uIter->angle / (NUM_ANGLES/framesX);

        int frame = (uIter->itemID == Unit_Sandworm) ? 5 : 0;

        SDL_Rect source = { frameSize.x * angle, frameSize.y * frame, frameSize.x, frameSize.y };
        SDL_Rect drawLocation = {   pScreenborder->world2screenX((position.x*TILESIZE)+(TILESIZE/2)) - frameSize.x/2,
                                    pScreenborder->world2screenY((position.y*TILESIZE)+(TILESIZE/2)) - frameSize.y/2,
                                    frameSize.x,
                                    frameSize.y };

        SDL_BlitSurface(pObjectSprite, &source, pScreen, &drawLocation);

        if(objectPicGun >= 0) {
            SDL_Surface* pGunSprite = pGFXManager->getObjPic(objectPicGun, uIter->house)[currentZoomlevel];

            Coord frameSize2 = Coord(pGunSprite->w/NUM_ANGLES, pGunSprite->h);

            SDL_Rect source2 = { frameSize2.x * uIter->angle, 0, frameSize2.x, frameSize2.y };
            SDL_Rect drawLocation2 = {  pScreenborder->world2screenX((position.x*TILESIZE)+(TILESIZE/2)+gunOffset[uIter->angle].x) - frameSize2.x/2,
                                        pScreenborder->world2screenY((position.y*TILESIZE)+(TILESIZE/2)+gunOffset[uIter->angle].y) - frameSize2.y/2,
                                        frameSize2.x,
                                        frameSize2.y };

            SDL_BlitSurface(pGunSprite, &source2, pScreen, &drawLocation2);
        }

        if(uIter->itemID == Unit_RaiderTrike || uIter->itemID == Unit_Deviator || uIter->itemID == Unit_Special) {
            SDL_Surface* pStarSprite = pGFXManager->getObjPic(ObjPic_Star)[currentZoomlevel];

            SDL_Rect drawLocation2 = {  pScreenborder->world2screenX((position.x*TILESIZE)+(TILESIZE/2)) - frameSize.x/2 + frameSize.x - pStarSprite->w,
                                        pScreenborder->world2screenY((position.y*TILESIZE)+(TILESIZE/2)) - frameSize.y/2 + frameSize.y -  pStarSprite->h,
                                        pStarSprite->w,
                                        pStarSprite->h };

            SDL_BlitSurface(pStarSprite, NULL, pScreen, &drawLocation2);
        }

	}

	// draw tactical pos rectangle (the starting screen)
	if(!bCompleteMap && getMapVersion() < 2 && mapInfo.tacticalPos.isValid()) {

        SDL_Rect dest;
        dest.x = pScreenborder->world2screenX( mapInfo.tacticalPos.x*TILESIZE);
        dest.y = pScreenborder->world2screenY( mapInfo.tacticalPos.y*TILESIZE);
        dest.w = world2zoomedWorld(15*TILESIZE);
        dest.h = world2zoomedWorld(10*TILESIZE);

        drawRect(pScreen, dest.x, dest.y, dest.x+dest.w, dest.y+dest.h, COLOR_DARKGREY);
	}

    SDL_Surface* validPlace = NULL;
    SDL_Surface* invalidPlace = NULL;
    SDL_Surface* greyPlace = NULL;

    switch(currentZoomlevel) {
        case 0: {
            validPlace = pGFXManager->getUIGraphic(UI_ValidPlace_Zoomlevel0);
            invalidPlace = pGFXManager->getUIGraphic(UI_InvalidPlace_Zoomlevel0);
            greyPlace = pGFXManager->getUIGraphic(UI_GreyPlace_Zoomlevel0);
        } break;

        case 1: {
            validPlace = pGFXManager->getUIGraphic(UI_ValidPlace_Zoomlevel1);
            invalidPlace = pGFXManager->getUIGraphic(UI_InvalidPlace_Zoomlevel1);
            greyPlace = pGFXManager->getUIGraphic(UI_GreyPlace_Zoomlevel1);
        } break;

        case 2:
        default: {
            validPlace = pGFXManager->getUIGraphic(UI_ValidPlace_Zoomlevel2);
            invalidPlace = pGFXManager->getUIGraphic(UI_InvalidPlace_Zoomlevel2);
            greyPlace = pGFXManager->getUIGraphic(UI_GreyPlace_Zoomlevel2);
        } break;
    }

    int mouseX;
    int mouseY;

    SDL_GetMouseState(&mouseX, &mouseY);

    if(!bCompleteMap && !pInterface->hasChildWindow() && pScreenborder->isScreenCoordInsideMap(mouseX, mouseY)) {

        int	xPos = pScreenborder->screen2MapX(mouseX);
		int yPos = pScreenborder->screen2MapY(mouseY);

        if(currentEditorMode.mode == EditorMode::EditorMode_Terrain) {

            int halfsize = currentEditorMode.pensize/2;

            for(int m=0;m<mapMirror->getSize();m++) {

                Coord position = mapMirror->getCoord( Coord(xPos, yPos), m);


                SDL_Rect dest;
                dest.x = pScreenborder->world2screenX( (position.x-halfsize)*TILESIZE);
                dest.y = pScreenborder->world2screenY( (position.y-halfsize)*TILESIZE);
                dest.w = world2zoomedWorld(currentEditorMode.pensize*TILESIZE);
                dest.h = world2zoomedWorld(currentEditorMode.pensize*TILESIZE);

                //now draw the box with parts at all corners
                if(!SDL_MUSTLOCK(pScreen) || (SDL_LockSurface(pScreen) == 0)) {
                    // top left bit
                    for(int i=0;i<=currentZoomlevel;i++) {
                        drawHLineNoLock(pScreen,dest.x+i, dest.y+i, dest.x+(currentZoomlevel+1)*3, COLOR_WHITE);
                        drawVLineNoLock(pScreen,dest.x+i, dest.y+i, dest.y+(currentZoomlevel+1)*3, COLOR_WHITE);
                    }

                    // top right bit
                    for(int i=0;i<=currentZoomlevel;i++) {
                        drawHLineNoLock(pScreen,dest.x + dest.w-1 - i, dest.y+i, dest.x + dest.w-1 - (currentZoomlevel+1)*3, COLOR_WHITE);
                        drawVLineNoLock(pScreen,dest.x + dest.w-1 - i, dest.y+i, dest.y+(currentZoomlevel+1)*3, COLOR_WHITE);
                    }

                    // bottom left bit
                    for(int i=0;i<=currentZoomlevel;i++) {
                        drawHLineNoLock(pScreen,dest.x+i, dest.y + dest.h-1 - i, dest.x+(currentZoomlevel+1)*3, COLOR_WHITE);
                        drawVLineNoLock(pScreen,dest.x+i, dest.y + dest.h-1 - i, dest.y + dest.h-1 - (currentZoomlevel+1)*3, COLOR_WHITE);
                    }

                    // bottom right bit
                    for(int i=0;i<=currentZoomlevel;i++) {
                        drawHLineNoLock(pScreen,dest.x + dest.w-1 - i, dest.y + dest.h-1 - i, dest.x + dest.w-1 - (currentZoomlevel+1)*3, COLOR_WHITE);
                        drawVLineNoLock(pScreen,dest.x + dest.w-1 - i, dest.y + dest.h-1 - i, dest.y + dest.h-1 - (currentZoomlevel+1)*3, COLOR_WHITE);
                    }

                    if(SDL_MUSTLOCK(pScreen)) {
                        SDL_UnlockSurface(pScreen);
                    }
                }
            }

        } else if(currentEditorMode.mode == EditorMode::EditorMode_Structure) {
            Coord structureSize = getStructureSize(currentEditorMode.itemID);

            for(int m=0;m<mapMirror->getSize();m++) {

                Coord position = mapMirror->getCoord( Coord(xPos, yPos), m, structureSize);

                for(int x = position.x; x < (position.x + structureSize.x); x++) {
                    for(int y = position.y; y < (position.y + structureSize.y); y++) {
                        SDL_Surface* image = validPlace;

                        // check if mirroring of the original (!) position is possible
                        if(mapMirror->mirroringPossible( Coord(xPos, yPos), structureSize) == false) {
                            image = invalidPlace;
                        }

                        // check all mirrored places
                        for(int k=0;k<mapMirror->getSize();k++) {
                            Coord pos = mapMirror->getCoord( Coord(x, y), k);

                            if(!map.isInsideMap(pos.x,pos.y) || isTileBlocked(pos.x, pos.y, true, (currentEditorMode.itemID != Structure_Slab1) )) {
                                image = invalidPlace;
                            } else if((image != invalidPlace) && (map(pos.x,pos.y) != Terrain_Rock)) {
                                image = greyPlace;
                            }
                        }

                        SDL_Rect drawLocation = {   pScreenborder->world2screenX(x*TILESIZE), pScreenborder->world2screenY(y*TILESIZE),
                                                    zoomedTilesize, zoomedTilesize };
                        SDL_BlitSurface(image, NULL, pScreen, &drawLocation);
                    }
                }
            }
        } else if(currentEditorMode.mode == EditorMode::EditorMode_Unit) {
            for(int m=0;m<mapMirror->getSize();m++) {

                Coord position = mapMirror->getCoord( Coord(xPos, yPos), m);

                SDL_Surface* image = validPlace;
                // check all mirrored places
                for(int k=0;k<mapMirror->getSize();k++) {
                    Coord pos = mapMirror->getCoord( position, k);

                    if(!map.isInsideMap(pos.x,pos.y) || isTileBlocked(pos.x, pos.y, false, true)) {
                        image = invalidPlace;
                    }
                }
                SDL_Rect drawLocation = {   pScreenborder->world2screenX(position.x*TILESIZE), pScreenborder->world2screenY(position.y*TILESIZE),
                                            image->w, image->h };
                SDL_BlitSurface(image, NULL, pScreen, &drawLocation);
            }
        } else if(currentEditorMode.mode == EditorMode::EditorMode_TacticalPos) {
            // draw tactical pos rectangle (the starting screen)
            if(mapInfo.tacticalPos.isValid()) {

                SDL_Rect dest = {   pScreenborder->world2screenX(xPos*TILESIZE), pScreenborder->world2screenY(yPos*TILESIZE),
                                    world2zoomedWorld(15*TILESIZE), world2zoomedWorld(10*TILESIZE) };
                drawRect(pScreen, dest.x, dest.y, dest.x+dest.w, dest.y+dest.h, COLOR_WHITE);
            }
        }
    }

    // draw selection rect for units (selection rect for structures is already drawn)
    if(!bCompleteMap) {
        std::vector<int> selectedUnits = getMirrorUnits(selectedUnitID);

        for(uIter = units.begin(); uIter != units.end(); ++uIter) {
            if(std::find(selectedUnits.begin(), selectedUnits.end(), uIter->id) != selectedUnits.end()) {
                Coord position = uIter->position;

                SDL_Surface* selectionBox = NULL;

                switch(currentZoomlevel) {
                    case 0:     selectionBox = pGFXManager->getUIGraphic(UI_SelectionBox_Zoomlevel0);   break;
                    case 1:     selectionBox = pGFXManager->getUIGraphic(UI_SelectionBox_Zoomlevel1);   break;
                    case 2:
                    default:    selectionBox = pGFXManager->getUIGraphic(UI_SelectionBox_Zoomlevel2);   break;
                }

                SDL_Rect dest = {   pScreenborder->world2screenX((position.x*TILESIZE)+(TILESIZE/2)) - selectionBox->w/2,
                                    pScreenborder->world2screenY((position.y*TILESIZE)+(TILESIZE/2)) - selectionBox->h/2,
                                    selectionBox->w,
                                    selectionBox->h };

                SDL_BlitSurface(selectionBox, NULL, pScreen, &dest);
            }
        }
    }

    // draw selection rect for map items (spice bloom, special bloom or spice field)
    if(!bCompleteMap && selectedMapItemCoord.isValid()
        && ( (std::find(spiceBlooms.begin(), spiceBlooms.end(), selectedMapItemCoord) != spiceBlooms.end())
            || (std::find(specialBlooms.begin(), specialBlooms.end(), selectedMapItemCoord) != specialBlooms.end())
            || (std::find(spiceFields.begin(), spiceFields.end(), selectedMapItemCoord) != spiceFields.end()) )) {
        SDL_Surface* selectionBox = NULL;

        switch(currentZoomlevel) {
            case 0:     selectionBox = pGFXManager->getUIGraphic(UI_SelectionBox_Zoomlevel0);   break;
            case 1:     selectionBox = pGFXManager->getUIGraphic(UI_SelectionBox_Zoomlevel1);   break;
            case 2:
            default:    selectionBox = pGFXManager->getUIGraphic(UI_SelectionBox_Zoomlevel2);   break;
        }

        SDL_Rect dest = {   pScreenborder->world2screenX((selectedMapItemCoord.x*TILESIZE)+(TILESIZE/2)) - selectionBox->w/2,
                            pScreenborder->world2screenY((selectedMapItemCoord.y*TILESIZE)+(TILESIZE/2)) - selectionBox->h/2,
                            selectionBox->w,
                            selectionBox->h };

        SDL_BlitSurface(selectionBox, NULL, pScreen, &dest);
    }
}

void MapEditor::saveMapshot() {
    int oldCurrentZoomlevel = currentZoomlevel;
    currentZoomlevel = 0;

    std::string mapshotFilename = (lastSaveName.empty() ? generateMapname() : getBasename(lastSaveName, true)) + ".bmp";

    int sizeX = world2zoomedWorld(map.getSizeX()*TILESIZE);
    int sizeY = world2zoomedWorld(map.getSizeY()*TILESIZE);

    SDL_Surface* pMapshotSurface = NULL;
    if((pMapshotSurface = SDL_CreateRGBSurface(SDL_SWSURFACE,sizeX,sizeY,8,0,0,0,0)) == NULL) {
        fprintf(stderr,"MapEditor::SaveMapshot: Cannot create new surface!\n");
        exit(EXIT_FAILURE);
    }
    palette.applyToSurface(pMapshotSurface);

    SDL_Rect board = { 0,0,sizeX,sizeY };

    ScreenBorder tmpScreenborder(board);
    tmpScreenborder.adjustScreenBorderToMapsize(map.getSizeX(), map.getSizeY());

    drawMap(pMapshotSurface, &tmpScreenborder, true);

    // set windtrap color to grey
    replaceColor(pMapshotSurface, COLOR_WINDTRAP_COLORCYCLE, COLOR_LIGHTGREY);

    SDL_SaveBMP(pMapshotSurface, mapshotFilename.c_str());

    SDL_FreeSurface(pMapshotSurface);
    currentZoomlevel = oldCurrentZoomlevel;
}
