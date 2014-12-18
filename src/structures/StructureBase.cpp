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

#include <structures/StructureBase.h>

#include <globals.h>

#include <House.h>
#include <Game.h>
#include <Map.h>
#include <ScreenBorder.h>
#include <Explosion.h>
#include <SoundPlayer.h>

#include <players/HumanPlayer.h>

#include <misc/draw_util.h>

#include <units/UnitBase.h>

#include <GUI/ObjectInterfaces/DefaultStructureInterface.h>

StructureBase::StructureBase(House* newOwner) : ObjectBase(newOwner) {
    StructureBase::init();

    repairing = false;
    fogged = false;
	degradeTimer = MILLI2CYCLES(15*1000);
}

StructureBase::StructureBase(InputStream& stream): ObjectBase(stream) {
    StructureBase::init();

    repairing = stream.readBool();
    fogged = stream.readBool();
    lastVisibleFrame = stream.readUint32();

    degradeTimer = stream.readSint32();

    size_t numSmoke = stream.readUint32();
    for(size_t i=0;i<numSmoke; i++) {
        smoke.push_back(StructureSmoke(stream));
    }
}

void StructureBase::init() {
	aStructure = true;

	structureSize.x = 0;
	structureSize.y = 0;

	justPlacedTimer = 0;

	lastVisibleFrame = curAnimFrame = 2;
	animationCounter = 0;

	structureList.push_back(this);
}

StructureBase::~StructureBase() {
    currentGameMap->removeObjectFromMap(getObjectID());	//no map point will reference now
	currentGame->getObjectManager().removeObject(getObjectID());
	structureList.remove(this);
	owner->decrementStructures(itemID, location);

    removeFromSelectionLists();
}

void StructureBase::save(OutputStream& stream) const {
	ObjectBase::save(stream);

    stream.writeBool(repairing);
	stream.writeBool(fogged);
	stream.writeUint32(lastVisibleFrame);

	stream.writeSint32(degradeTimer);

	stream.writeUint32(smoke.size());
	std::list<StructureSmoke>::const_iterator iter;
	for(iter = smoke.begin(); iter != smoke.end(); ++iter) {
        iter->save(stream);
	}
}

void StructureBase::assignToMap(const Coord& pos) {
    bool bFoundNonConcreteTile = false;

	Coord temp;
	for(int i = pos.x; i < pos.x + structureSize.x; i++) {
		for(int j = pos.y; j < pos.y + structureSize.y; j++) {
			if(currentGameMap->tileExists(i, j)) {
                Tile* pTile = currentGameMap->getTile(i,j);
				pTile->assignNonInfantryGroundObject(getObjectID());
				if(!pTile->isConcrete() && currentGame->getGameInitSettings().getGameOptions().concreteRequired && (currentGame->gameState != START)) {
                    bFoundNonConcreteTile = true;

                    if((itemID != Structure_Wall) && (itemID != Structure_ConstructionYard)) {
                        setHealth(getHealth() - (0.5f*(float)getMaxHealth()/((float)(structureSize.x*structureSize.y))));
                    }
				}
				pTile->setType(Terrain_Rock);
				pTile->setOwner(getOwner()->getHouseID());
				currentGameMap->viewMap(getOwner()->getTeam(), Coord(i,j), getViewRange());

				setVisible(VIS_ALL, true);
				setActive(true);
				setRespondable(true);
			}
		}
	}

	if(!bFoundNonConcreteTile && !currentGame->getGameInitSettings().getGameOptions().structuresDegradeOnConcrete) {
        degradeTimer = -1;
	}
}

void StructureBase::blitToScreen() {
    int imageW = graphic[currentZoomlevel]->w/numImagesX;
    int imageH = graphic[currentZoomlevel]->h/numImagesY;

    SDL_Rect dest = { screenborder->world2screenX((int) lround(realX)), screenborder->world2screenY((int) lround(realY)), imageW, imageH };
    SDL_Rect source = { imageW * (fogged ? lastVisibleFrame : curAnimFrame), 0, imageW, imageH };

    SDL_BlitSurface(graphic[currentZoomlevel], &source, screen, &dest);

    if(fogged) {
        SDL_Surface* fogSurf = pGFXManager->getTransparent40Surface();
        SDL_BlitSurface(fogSurf, &source, screen, &dest);
    } else {
        SDL_Surface** pSmokeSurface = pGFXManager->getObjPic(ObjPic_Smoke,getOwner()->getHouseID());
        SDL_Rect smokeSource = { 0, 0, pSmokeSurface[currentZoomlevel]->w/3, pSmokeSurface[currentZoomlevel]->h};
        std::list<StructureSmoke>::const_iterator iter;
        for(iter = smoke.begin(); iter != smoke.end(); ++iter) {
            SDL_Rect smokeDest = {  screenborder->world2screenX(iter->realPos.x) - smokeSource.w/2,
                                    screenborder->world2screenY(iter->realPos.y) - smokeSource.h,
                                    pSmokeSurface[currentZoomlevel]->w/3,
                                    pSmokeSurface[currentZoomlevel]->h};
            Uint32 cycleDiff = currentGame->getGameCycleCount() - iter->startGameCycle;

            Uint32 smokeFrame = (cycleDiff/25) % 4;
            if(smokeFrame == 3) {
                smokeFrame = 1;
            }

            smokeSource.x = smokeFrame * smokeSource.w;
            SDL_BlitSurface(pSmokeSurface[currentZoomlevel], &smokeSource, screen, &smokeDest);
        }
    }
}

ObjectInterface* StructureBase::getInterfaceContainer() {
	if((pLocalHouse == owner) || (debug == true)) {
		return DefaultStructureInterface::create(objectID);
	} else {
		return DefaultObjectInterface::create(objectID);
	}
}

void StructureBase::drawSelectionBox() {
    int imageW = graphic[currentZoomlevel]->w/numImagesX;
    int imageH = graphic[currentZoomlevel]->h/numImagesY;

	SDL_Rect dest;
	dest.x = screenborder->world2screenX((int) realX);
	dest.y = screenborder->world2screenY((int) realY);
	dest.w = imageW;
	dest.h = imageH;

	//now draw the selection box thing, with parts at all corners of structure
	if(!SDL_MUSTLOCK(screen) || (SDL_LockSurface(screen) == 0)) {
        // top left bit
        for(int i=0;i<=currentZoomlevel;i++) {
            drawHLineNoLock(screen,dest.x+i, dest.y+i, dest.x+(currentZoomlevel+1)*3, COLOR_WHITE);
            drawVLineNoLock(screen,dest.x+i, dest.y+i, dest.y+(currentZoomlevel+1)*3, COLOR_WHITE);
        }

        // top right bit
        for(int i=0;i<=currentZoomlevel;i++) {
            drawHLineNoLock(screen,dest.x + dest.w-1 - i, dest.y+i, dest.x + dest.w-1 - (currentZoomlevel+1)*3, COLOR_WHITE);
            drawVLineNoLock(screen,dest.x + dest.w-1 - i, dest.y+i, dest.y+(currentZoomlevel+1)*3, COLOR_WHITE);
        }

        // bottom left bit
        for(int i=0;i<=currentZoomlevel;i++) {
            drawHLineNoLock(screen,dest.x+i, dest.y + dest.h-1 - i, dest.x+(currentZoomlevel+1)*3, COLOR_WHITE);
            drawVLineNoLock(screen,dest.x+i, dest.y + dest.h-1 - i, dest.y + dest.h-1 - (currentZoomlevel+1)*3, COLOR_WHITE);
        }

        // bottom right bit
        for(int i=0;i<=currentZoomlevel;i++) {
            drawHLineNoLock(screen,dest.x + dest.w-1 - i, dest.y + dest.h-1 - i, dest.x + dest.w-1 - (currentZoomlevel+1)*3, COLOR_WHITE);
            drawVLineNoLock(screen,dest.x + dest.w-1 - i, dest.y + dest.h-1 - i, dest.y + dest.h-1 - (currentZoomlevel+1)*3, COLOR_WHITE);
        }

		if(SDL_MUSTLOCK(screen)) {
			SDL_UnlockSurface(screen);
		}
	}

    for(int i=1;i<=currentZoomlevel+1;i++) {
        drawHLine(screen, dest.x, dest.y-i-1, dest.x + ((int)((getHealth()/(float)getMaxHealth())*(world2zoomedWorld(TILESIZE)*structureSize.x - 1))), getHealthColor());
    }
}

void StructureBase::drawOtherPlayerSelectionBox() {
    int imageW = graphic[currentZoomlevel]->w/numImagesX;
    int imageH = graphic[currentZoomlevel]->h/numImagesY;

	SDL_Rect dest;
	dest.x = screenborder->world2screenX((int) realX) + (currentZoomlevel+1);
	dest.y = screenborder->world2screenY((int) realY) + (currentZoomlevel+1);
	dest.w = imageW - 2*(currentZoomlevel+1);
	dest.h = imageH - 2*(currentZoomlevel+1);

	//now draw the selection box thing, with parts at all corners of structure
	if(!SDL_MUSTLOCK(screen) || (SDL_LockSurface(screen) == 0)) {
        // top left bit
        for(int i=0;i<=currentZoomlevel;i++) {
            drawHLineNoLock(screen,dest.x+i, dest.y+i, dest.x+(currentZoomlevel+1)*2, COLOR_LIGHTBLUE);
            drawVLineNoLock(screen,dest.x+i, dest.y+i, dest.y+(currentZoomlevel+1)*2, COLOR_LIGHTBLUE);
        }

        // top right bit
        for(int i=0;i<=currentZoomlevel;i++) {
            drawHLineNoLock(screen,dest.x + dest.w-1 - i, dest.y+i, dest.x + dest.w-1 - (currentZoomlevel+1)*2, COLOR_LIGHTBLUE);
            drawVLineNoLock(screen,dest.x + dest.w-1 - i, dest.y+i, dest.y+(currentZoomlevel+1)*2, COLOR_LIGHTBLUE);
        }

        // bottom left bit
        for(int i=0;i<=currentZoomlevel;i++) {
            drawHLineNoLock(screen,dest.x+i, dest.y + dest.h-1 - i, dest.x+(currentZoomlevel+1)*2, COLOR_LIGHTBLUE);
            drawVLineNoLock(screen,dest.x+i, dest.y + dest.h-1 - i, dest.y + dest.h-1 - (currentZoomlevel+1)*2, COLOR_LIGHTBLUE);
        }

        // bottom right bit
        for(int i=0;i<=currentZoomlevel;i++) {
            drawHLineNoLock(screen,dest.x + dest.w-1 - i, dest.y + dest.h-1 - i, dest.x + dest.w-1 - (currentZoomlevel+1)*2, COLOR_LIGHTBLUE);
            drawVLineNoLock(screen,dest.x + dest.w-1 - i, dest.y + dest.h-1 - i, dest.y + dest.h-1 - (currentZoomlevel+1)*2, COLOR_LIGHTBLUE);
        }

		if(SDL_MUSTLOCK(screen)) {
			SDL_UnlockSurface(screen);
		}
	}

}

/**
    Returns the center point of this structure
    \return the center point in world coordinates
*/
Coord StructureBase::getCenterPoint() const {
    return Coord( lround(realX + structureSize.x*TILESIZE/2),
                  lround(realY + structureSize.y*TILESIZE/2));
}

Coord StructureBase::getClosestCenterPoint(const Coord& objectLocation) const {
	return getClosestPoint(objectLocation) * TILESIZE + Coord(TILESIZE/2, TILESIZE/2);
}

void StructureBase::handleActionClick(int xPos, int yPos) {
	if ((xPos < location.x) || (xPos >= (location.x + structureSize.x)) || (yPos < location.y) || (yPos >= (location.y + structureSize.y))) {
		currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_STRUCTURE_SETDEPLOYPOSITION,objectID, (Uint32) xPos, (Uint32) yPos));
	} else {
		currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_STRUCTURE_SETDEPLOYPOSITION,objectID, (Uint32) NONE, (Uint32) NONE));
	}
}

void StructureBase::handleRepairClick() {
	currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_STRUCTURE_REPAIR,objectID));
}

void StructureBase::doSetDeployPosition(int xPos, int yPos) {
	setTarget(NULL);
	setDestination(xPos,yPos);
	setForced(true);
}


void StructureBase::doRepair() {
	repairing = true;
}

void StructureBase::setDestination(int newX, int newY) {
	if(currentGameMap->tileExists(newX, newY) || ((newX == INVALID_POS) && (newY == INVALID_POS))) {
		destination.x = newX;
		destination.y = newY;
	}
}

void StructureBase::setJustPlaced() {
	justPlacedTimer = 6;
	curAnimFrame = 0;
	animationCounter = -ANIMATIONTIMER; // make first build animation double as long
}

bool StructureBase::update() {
    //update map
    if(currentGame->randomGen.rand(0,40) == 0) {
        // PROBLEM: causes very low fps
        currentGameMap->viewMap(owner->getTeam(), location, getViewRange());
    }

    if(!fogged) {
        lastVisibleFrame = curAnimFrame;
    }

    // degrade
    if((degradeTimer >= 0) && currentGame->getGameInitSettings().getGameOptions().concreteRequired && (owner->getPowerRequirement() > owner->getProducedPower())) {
        degradeTimer--;
        if(degradeTimer <= 0) {
            degradeTimer = MILLI2CYCLES(15*1000);

            int damageMultiplyer = 1;
            if(owner->getHouseID() == HOUSE_HARKONNEN || owner->getHouseID() == HOUSE_SARDAUKAR) {
                damageMultiplyer = 3;
            } else if(owner->getHouseID() == HOUSE_ORDOS) {
                damageMultiplyer = 2;
            } else if(owner->getHouseID() == HOUSE_MERCENARY) {
                damageMultiplyer = 5;
            }

            if(getHealth() > getMaxHealth() / 2) {
                setHealth( getHealth() - ((damageMultiplyer * (float)getMaxHealth())/100.0f));
            }
        }
    }

    updateStructureSpecificStuff();

    if(getHealth() <= 0.0f) {
        destroy();
        return false;
    }

    if(repairing) {
        if(owner->getCredits() >= 5) {
            // Original dune 2 is doing the repair calculation with fix-point math (multiply everything with 256).
            // It is calculating what fraction 2 hitpoints of the maximum health would be.
            int fraction = (2*256)/getMaxHealth();
            float repairprice = (fraction * currentGame->objectData.data[itemID][originalHouseID].price) / 256.0f;

            // Original dune is always repairing 5 hitpoints (for the costs of 2) but we are only repairing 1/30th of that
            owner->takeCredits(repairprice/30.0f);
            float newHealth = getHealth();
            newHealth += 5.0f/30.0f;
            if(newHealth >= getMaxHealth()) {
                setHealth(getMaxHealth());
                repairing = false;
            } else {
                setHealth(newHealth);
            }
        } else {
            repairing = false;
        }
    } else if(owner->isAI() && ((getHealth()/(float)getMaxHealth()) < 0.75f)) {
        doRepair();
    }

    // check smoke
    std::list<StructureSmoke>::iterator iter = smoke.begin();
    while(iter != smoke.end()) {
        if(currentGame->getGameCycleCount() - iter->startGameCycle >= MILLI2CYCLES(8*1000)) {
            smoke.erase(iter++);
        } else {
            ++iter;
        }
    }

    // update animations
    animationCounter++;
    if(animationCounter > ANIMATIONTIMER) {
        animationCounter = 0;
        curAnimFrame++;
        if((curAnimFrame < firstAnimFrame) || (curAnimFrame > lastAnimFrame)) {
            curAnimFrame = firstAnimFrame;
        }

        justPlacedTimer--;
        if((justPlacedTimer > 0) && (justPlacedTimer % 2 == 0)) {
            curAnimFrame = 0;
        }
    }

    return true;
}

void StructureBase::destroy() {
    int*    pDestroyedStructureTiles = NULL;
    int     DestroyedStructureTilesSizeY = 0;
    static int DestroyedStructureTilesWall[] = { DestroyedStructure_Wall };
    static int DestroyedStructureTiles1x1[] = { Destroyed1x1Structure };
    static int DestroyedStructureTiles2x2[] = { Destroyed2x2Structure_TopLeft, Destroyed2x2Structure_TopRight,
                                                Destroyed2x2Structure_BottomLeft, Destroyed2x2Structure_BottomRight };
    static int DestroyedStructureTiles3x2[] = { Destroyed3x2Structure_TopLeft, Destroyed3x2Structure_TopCenter, Destroyed3x2Structure_TopRight,
                                                Destroyed3x2Structure_BottomLeft, Destroyed3x2Structure_BottomCenter, Destroyed3x2Structure_BottomRight};
    static int DestroyedStructureTiles3x3[] = { Destroyed3x3Structure_TopLeft, Destroyed3x3Structure_TopCenter, Destroyed3x3Structure_TopRight,
                                                Destroyed3x3Structure_CenterLeft, Destroyed3x3Structure_CenterCenter, Destroyed3x3Structure_CenterRight,
                                                Destroyed3x3Structure_BottomLeft, Destroyed3x3Structure_BottomCenter, Destroyed3x3Structure_BottomRight};


    if(itemID == Structure_Wall) {
        pDestroyedStructureTiles = DestroyedStructureTilesWall;
        DestroyedStructureTilesSizeY = 1;
    } else {
        switch(structureSize.y) {
            case 1: {
                pDestroyedStructureTiles = DestroyedStructureTiles1x1;
                DestroyedStructureTilesSizeY = 1;
            } break;

            case 2: {
                if(structureSize.x == 2) {
                    pDestroyedStructureTiles = DestroyedStructureTiles2x2;
                    DestroyedStructureTilesSizeY = 2;
                } else if(structureSize.x == 3) {
                    pDestroyedStructureTiles = DestroyedStructureTiles3x2;
                    DestroyedStructureTilesSizeY = 3;
                } else {
                    throw std::runtime_error("StructureBase::destroy(): Invalid structure size");
                }
            } break;

            case 3: {
                pDestroyedStructureTiles = DestroyedStructureTiles3x3;
                DestroyedStructureTilesSizeY = 3;
            } break;

            default: {
                throw std::runtime_error("StructureBase::destroy(): Invalid structure size");
            } break;
        }
    }

    if(itemID != Structure_Wall) {
        for(int j = 0; j < structureSize.y; j++) {
            for(int i = 0; i < structureSize.x; i++) {
                Tile* pTile = currentGameMap->getTile(location.x + i, location.y + j);
                pTile->setDestroyedStructureTile(pDestroyedStructureTiles[DestroyedStructureTilesSizeY*j + i]);

                Coord position((location.x+i)*TILESIZE + TILESIZE/2, (location.y+j)*TILESIZE + TILESIZE/2);
                Uint32 explosionID = currentGame->randomGen.getRandOf(2,Explosion_Large1,Explosion_Large2);
                currentGame->getExplosionList().push_back(new Explosion(explosionID, position, owner->getHouseID()) );

                if(currentGame->randomGen.rand(1,100) <= getInfSpawnProp()) {
                    UnitBase* pNewUnit = owner->createUnit(Unit_Soldier);
                    pNewUnit->setHealth(pNewUnit->getMaxHealth()/2);
                    pNewUnit->deploy(location + Coord(i,j));
                    if(getOwner()->isAI()) {
                        pNewUnit->doSetAttackMode(HUNT);
                    }
                }
            }
        }
    }

	if(isVisible(pLocalHouse->getTeam()))
		soundPlayer->playSoundAt(Sound_ExplosionStructure, location);


    delete this;
}

Coord StructureBase::getClosestPoint(const Coord& objectLocation) const {
	Coord closestPoint;

	// find the closest tile of a structure from a location
	if(objectLocation.x <= location.x) {
	    // if we are left of the structure
        // set destination, left most point
		closestPoint.x = location.x;
	} else if(objectLocation.x >= (location.x + structureSize.x-1)) {
	    //vica versa
		closestPoint.x = location.x + structureSize.x-1;
	} else {
        //we are above or below at least one tile of the structure, closest path is straight
		closestPoint.x = objectLocation.x;
	}

	//same deal but with y
	if(objectLocation.y <= location.y) {
		closestPoint.y = location.y;
	} else if(objectLocation.y >= (location.y + structureSize.y-1)) {
		closestPoint.y = location.y + structureSize.y-1;
	} else {
		closestPoint.y = objectLocation.y;
	}

	return closestPoint;
}
