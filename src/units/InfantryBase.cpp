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

#include <units/InfantryBase.h>

#include <globals.h>

#include <House.h>
#include <Game.h>
#include <Map.h>
#include <SoundPlayer.h>
#include <ScreenBorder.h>

#include <players/HumanPlayer.h>

#include <structures/StructureBase.h>
#include <structures/Refinery.h>
#include <structures/RepairYard.h>
#include <units/Harvester.h>

#include <misc/strictmath.h>

// the position on the tile
Coord tilePositionOffset[5] = { Coord(0,0), Coord(-TILESIZE/4,-TILESIZE/4), Coord(TILESIZE/4,-TILESIZE/4), Coord(-TILESIZE/4,TILESIZE/4), Coord(TILESIZE/4,TILESIZE/4)};


InfantryBase::InfantryBase(House* newOwner) : GroundUnit(newOwner) {

    InfantryBase::init();

    setHealth(getMaxHealth());

    tilePosition = INVALID_POS;
    oldTilePosition = INVALID_POS;
}

InfantryBase::InfantryBase(InputStream& stream) : GroundUnit(stream) {

    InfantryBase::init();

	tilePosition = stream.readSint8();
	oldTilePosition = stream.readSint8();
}

void InfantryBase::init() {
	infantry = true;
	walkFrame = 0;
}

InfantryBase::~InfantryBase() {
}


void InfantryBase::save(OutputStream& stream) const {

	GroundUnit::save(stream);

	stream.writeSint8(tilePosition);
	stream.writeSint8(oldTilePosition);
}

void InfantryBase::handleCaptureClick(int xPos, int yPos) {
	if(respondable && ((getItemID() == Unit_Soldier) || (getItemID() == Unit_Trooper))) {
		if (currentGameMap->tileExists(xPos, yPos)) {
			if (currentGameMap->getTile(xPos,yPos)->hasAnObject()) {
				// capture structure
				ObjectBase* tempTarget = currentGameMap->getTile(xPos,yPos)->getObject();

				currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_INFANTRY_CAPTURE,objectID,tempTarget->getObjectID()));
			}
		}
	}

}

void InfantryBase::doCaptureStructure(Uint32 targetStructureID) {
	const StructureBase* pStructure = dynamic_cast<StructureBase*>(currentGame->getObjectManager().getObject(targetStructureID));
    doCaptureStructure(pStructure);
}

void InfantryBase::doCaptureStructure(const StructureBase* pStructure) {

	if((pStructure == NULL) || (pStructure->canBeCaptured() == false) || (pStructure->getOwner()->getTeam() == getOwner()->getTeam())) {
	    // does not exist anymore, cannot be captured or is a friendly building
        return;
	}

	doAttackObject(pStructure, true);
	doSetAttackMode(CAPTURE);
}

void InfantryBase::assignToMap(const Coord& pos) {
	if(currentGameMap->tileExists(pos)) {
		oldTilePosition = tilePosition;
		tilePosition = currentGameMap->getTile(pos)->assignInfantry(getObjectID());
	}
}

void InfantryBase::blitToScreen() {
    int imageW = graphic[currentZoomlevel]->w/numImagesX;
    int imageH = graphic[currentZoomlevel]->h/numImagesY;

	SDL_Rect dest = {   screenborder->world2screenX(realX) - imageW/2,
                        screenborder->world2screenY(realY) - imageH/2,
                        imageW, imageH };

    int temp = drawnAngle;
    if(temp == UP) {
        temp = 1;
    } else if (temp == DOWN) {
        temp = 3;
    } else if (temp == LEFTUP || temp == LEFTDOWN || temp == LEFT) {
        temp = 2;
    } else {
        //RIGHT
        temp = 0;
    }

    SDL_Rect source = { temp*imageW, (walkFrame/10 == 3) ? imageH : walkFrame/10*imageH, imageW, imageH };

    SDL_BlitSurface(graphic[currentZoomlevel], &source, screen, &dest);
}

bool InfantryBase::canPass(int xPos, int yPos) const {
	bool passable = false;
	if(currentGameMap->tileExists(xPos, yPos)) {
		Tile* pTile = currentGameMap->getTile(xPos, yPos);
		if(!pTile->hasAGroundObject()) {
			if(pTile->getType() != Terrain_Mountain) {
				passable = true;
			} else {
				/* if this unit is infantry so can climb, and tile can take more infantry */
				if(pTile->infantryNotFull()) {
					passable = true;
                }
			}
		} else {
			ObjectBase *object = pTile->getGroundObject();

			if((object != NULL) && (object->getObjectID() == target.getObjectID())
				&& object->isAStructure()
				&& (object->getOwner()->getTeam() != owner->getTeam())
				&& object->isVisible(getOwner()->getTeam())) {
				passable = true;
			} else {
				passable = (!pTile->hasANonInfantryGroundObject()
							&& (pTile->infantryNotFull()
							&& (pTile->getInfantryTeam() == getOwner()->getTeam())));
			}
		}
	}
	return passable;
}

void InfantryBase::checkPos() {
	if(moving && !justStoppedMoving) {
		if(++walkFrame > 39) {
			walkFrame = 0;
		}
	}

	if(justStoppedMoving) {
		walkFrame = 0;

		if(currentGameMap->getTile(location)->isSpiceBloom()) {
		    setHealth(0.0f);
			currentGameMap->getTile(location)->triggerSpiceBloom(getOwner());
		} else if(currentGameMap->getTile(location)->isSpecialBloom()){
            currentGameMap->getTile(location)->triggerSpecialBloom(getOwner());
		}

        //check to see if close enough to blow up target
        if(target.getObjPointer() != NULL
            && target.getObjPointer()->isAStructure()
            && (getOwner()->getTeam() != target.getObjPointer()->getOwner()->getTeam()))
        {
            Coord	closestPoint;

            closestPoint = target.getObjPointer()->getClosestPoint(location);

            if(blockDistance(location, closestPoint) <= 0.5f) {
                StructureBase* pCapturedStructure = target.getStructurePointer();
                if(pCapturedStructure->getHealthColor() == COLOR_RED) {
                    House* pOwner = pCapturedStructure->getOwner();
                    int targetID = pCapturedStructure->getItemID();
                    int posX = pCapturedStructure->getX();
                    int posY = pCapturedStructure->getY();
                    int origHouse = pCapturedStructure->getOriginalHouseID();
                    int oldHealth = pCapturedStructure->getHealth();
                    bool isSelected = pCapturedStructure->isSelected();
                    bool isSelectedByOtherPlayer = pCapturedStructure->isSelectedByOtherPlayer();

                    float capturedSpice = 0.0f;

                    UnitBase* pContainedUnit = NULL;

                    if(pCapturedStructure->getItemID() == Structure_Silo) {
                        capturedSpice = currentGame->objectData.data[Structure_Silo][originalHouseID].capacity * (pOwner->getStoredCredits() / pOwner->getCapacity());
                    } else if(pCapturedStructure->getItemID() == Structure_Refinery) {
                        capturedSpice = currentGame->objectData.data[Structure_Silo][originalHouseID].capacity * (pOwner->getStoredCredits() / pOwner->getCapacity());
                        Refinery* pRefinery = dynamic_cast<Refinery*>(pCapturedStructure);
                        if(pRefinery->isFree() == false) {
                            pContainedUnit = pRefinery->getHarvester();
                        }
                    } else if(pCapturedStructure->getItemID() == Structure_RepairYard) {
                        RepairYard* pRepairYard = dynamic_cast<RepairYard*>(pCapturedStructure);
                        if(pRepairYard->isFree() == false) {
                            pContainedUnit = pRepairYard->getRepairUnit();
                        }
                    }

                    Uint32 containedUnitID = NONE;
                    float containedUnitHealth = 0.0f;
                    float containedHarvesterSpice = 0.0f;
                    if(pContainedUnit != NULL) {
                        containedUnitID = pContainedUnit->getItemID();
                        containedUnitHealth = pContainedUnit->getHealth();
                        if(containedUnitID == Unit_Harvester) {
                            containedHarvesterSpice = dynamic_cast<Harvester*>(pContainedUnit)->getAmountOfSpice();
                        }

                        // will be destroyed by the captured structure
                        pContainedUnit = NULL;
                    }

                    // remove all other infantry units capturing this building
                    Coord capturedStructureLocation = pCapturedStructure->getLocation();
                    for(int i = capturedStructureLocation.x; i < capturedStructureLocation.x + pCapturedStructure->getStructureSizeX(); i++) {
                        for(int j = capturedStructureLocation.y; j < capturedStructureLocation.y + pCapturedStructure->getStructureSizeY(); j++) {

                            // make a copy of infantry list to avoid problems of modifing the list during iteration (!)
                            const std::list<Uint32> infantryList = currentGameMap->getTile(i,j)->getInfantryList();
                            std::list<Uint32>::const_iterator iter;
                            for(iter = infantryList.begin(); iter != infantryList.end(); ++iter) {
                                if(*iter != getObjectID()) {
                                    ObjectBase* pObject = currentGame->getObjectManager().getObject(*iter);
                                    if(pObject->getLocation() == Coord(i,j)) {
                                        pObject->destroy();
                                    }
                                }
                            }

                        }
                    }


                    // destroy captured structure ...
                    pCapturedStructure->setHealth(0.0f);
                    delete pCapturedStructure;

                    // ... and create a new one
                    StructureBase* pNewStructure = owner->placeStructure(NONE, targetID, posX, posY, true);

                    pNewStructure->setOriginalHouseID(origHouse);
                    pNewStructure->setHealth(oldHealth);
                    if(isSelected == true) {
                        pNewStructure->setSelected(true);
                        currentGame->getSelectedList().insert(pNewStructure->getObjectID());
                        currentGame->selectionChanged();
                    }

                    if(isSelectedByOtherPlayer == true) {
                        pNewStructure->setSelectedByOtherPlayer(true);
                        currentGame->getSelectedByOtherPlayerList().insert(pNewStructure->getObjectID());
                    }

                    if(containedUnitID != NONE) {
                        UnitBase* pNewUnit = owner->createUnit(containedUnitID);

                        pNewUnit->setRespondable(false);
                        pNewUnit->setActive(false);
                        pNewUnit->setVisible(VIS_ALL, false);
                        pNewUnit->setHealth(containedUnitHealth);

                        if(pNewUnit->getItemID() == Unit_Harvester) {
                            dynamic_cast<Harvester*>(pNewUnit)->setAmountOfSpice(containedHarvesterSpice);
                        }

                        if(pNewStructure->getItemID() == Structure_Refinery) {
                            Refinery* pRefinery = dynamic_cast<Refinery*>(pNewStructure);
                            pRefinery->book();
                            pRefinery->assignHarvester(dynamic_cast<Harvester*>(pNewUnit));
                        } else if(pNewStructure->getItemID() == Structure_RepairYard) {
                            RepairYard* pRepairYard = dynamic_cast<RepairYard*>(pNewStructure);
                            pRepairYard->book();
                            pRepairYard->assignUnit(pNewUnit);
                        }
                    }

                    // steal credits
                    pOwner->takeCredits(capturedSpice);
                    owner->addCredits(capturedSpice, false);
                    owner->updateBuildLists();

                } else {
                    int damage = std::min(pCapturedStructure->getHealth()/2, getHealth()*2);
                    pCapturedStructure->handleDamage(damage, NONE, getOwner());
                }
                // destroy unit indirectly
                setTarget(NULL);
                setHealth(0.0f);
                return;
            }
        } else if(target.getObjPointer() != NULL && target.getObjPointer()->isAStructure())	{
            Coord	closestPoint;
            closestPoint = target.getObjPointer()->getClosestPoint(location);

            if(blockDistance(location, closestPoint) <= 0.5f) {
                // destroy unit indirectly
                setTarget(NULL);
                setHealth(0.0f);
                return;
            }
        }
	}
}

void InfantryBase::destroy() {
    if(currentGameMap->tileExists(location) && isVisible()) {
        Tile* pTile = currentGameMap->getTile(location);

        if(pTile->hasANonInfantryGroundObject() == true) {
            if(pTile->getNonInfantryGroundObject()->isAUnit()) {
                // squashed
                pTile->assignDeadUnit( currentGame->randomGen.randBool() ? DeadUnit_Infantry_Squashed1 : DeadUnit_Infantry_Squashed2,
                                            owner->getHouseID(),
                                            Coord(lround(realX), lround(realY)) );

                if(isVisible(getOwner()->getTeam())) {
                    soundPlayer->playSoundAt(Sound_Squashed,location);
                }
            } else {
                // this unit has captured a building
            }

        } else if(getItemID() != Unit_Saboteur) {
            // "normal" dead
            pTile->assignDeadUnit( DeadUnit_Infantry,
                                        owner->getHouseID(),
                                        Coord(lround(realX), lround(realY)));

            if(isVisible(getOwner()->getTeam())) {
                soundPlayer->playSoundAt((Sound_enum) getRandomOf(6,Sound_Scream1,Sound_Scream2,Sound_Scream3,Sound_Scream4,Sound_Scream5,Sound_Trumpet),location);
            }
        }
    }

	GroundUnit::destroy();
}

void InfantryBase::move() {
	if(!moving && !justStoppedMoving && currentGame->randomGen.rand(0,40) == 0) {
		currentGameMap->viewMap(owner->getTeam(), location, getViewRange());
	}

	if(moving && !justStoppedMoving) {
		realX += xSpeed;
		realY += ySpeed;


        // check if unit is on the first half of the way
        float fromDistanceX;
        float fromDistanceY;
		float toDistanceX;
		float toDistanceY;

        const float epsilon = 3.75f;

		if(location != nextSpot) {
		    float abstractDistanceX = strictmath::abs(location.x*TILESIZE + TILESIZE/2 - (realX-bumpyOffsetX));
		    float abstractDistanceY = strictmath::abs(location.y*TILESIZE + TILESIZE/2 - (realY-bumpyOffsetY));

            fromDistanceX = strictmath::abs(location.x*TILESIZE + TILESIZE/2 + tilePositionOffset[oldTilePosition].x - (realX-bumpyOffsetX));
		    fromDistanceY = strictmath::abs(location.y*TILESIZE + TILESIZE/2 + tilePositionOffset[oldTilePosition].y - (realY-bumpyOffsetY));
		    toDistanceX = strictmath::abs(nextSpot.x*TILESIZE + TILESIZE/2 + tilePositionOffset[tilePosition].x - (realX-bumpyOffsetX));
		    toDistanceY = strictmath::abs(nextSpot.y*TILESIZE + TILESIZE/2 + tilePositionOffset[tilePosition].y - (realY-bumpyOffsetY));

		    // check if unit is half way out of old tile
            if((abstractDistanceX >= TILESIZE/2 + epsilon) || (abstractDistanceY >= TILESIZE/2 + epsilon)) {
                // let something else go in
                unassignFromMap(location);
                oldLocation = location;
                location = nextSpot;

                currentGameMap->viewMap(owner->getTeam(), location, getViewRange());
		    }

		} else {
            fromDistanceX = strictmath::abs(oldLocation.x*TILESIZE + TILESIZE/2 + tilePositionOffset[oldTilePosition].x - (realX-bumpyOffsetX));
		    fromDistanceY = strictmath::abs(oldLocation.y*TILESIZE + TILESIZE/2 + tilePositionOffset[oldTilePosition].y - (realY-bumpyOffsetY));
		    toDistanceX = strictmath::abs(location.x*TILESIZE + TILESIZE/2 + tilePositionOffset[tilePosition].x - (realX-bumpyOffsetX));
		    toDistanceY = strictmath::abs(location.y*TILESIZE + TILESIZE/2 + tilePositionOffset[tilePosition].y - (realY-bumpyOffsetY));

            Coord	wantedReal;
            wantedReal.x = nextSpot.x*TILESIZE + TILESIZE/2 + tilePositionOffset[tilePosition].x;
            wantedReal.y = nextSpot.y*TILESIZE + TILESIZE/2 + tilePositionOffset[tilePosition].y;

            if( (strictmath::abs((float)wantedReal.x - (realX-bumpyOffsetX)) <= strictmath::abs(xSpeed)/2 + epsilon)
                && (strictmath::abs((float)wantedReal.y - (realY-bumpyOffsetY)) <= strictmath::abs(ySpeed)/2 + epsilon) ) {
                realX = wantedReal.x;
                realY = wantedReal.y;
                bumpyOffsetX = 0.0f;
                bumpyOffsetY = 0.0f;

                if(forced && (location == destination) && !target) {
                    setForced(false);
                }

                moving = false;
                justStoppedMoving = true;

                oldLocation.invalidate();
            }
		}

		bumpyMovementOnRock(fromDistanceX, fromDistanceY, toDistanceX, toDistanceY);

	} else {
		justStoppedMoving = false;
	}

	checkPos();
}


void InfantryBase::setLocation(int xPos, int yPos) {
	if(currentGameMap->tileExists(xPos, yPos) || ((xPos == INVALID_POS) && (yPos == INVALID_POS))) {
		oldTilePosition = tilePosition = INVALID_POS;
		GroundUnit::setLocation(xPos, yPos);

		if(tilePosition != INVALID_POS) {
            realX += tilePositionOffset[tilePosition].x;
            realY += tilePositionOffset[tilePosition].y;
		}
	}
}

void InfantryBase::setSpeeds() {
	if(oldTilePosition == INVALID_POS) {
		fprintf(stderr, "InfantryBase::setSpeeds(): Infantry tile position  == INVALID_POS.\n");
	} else if(tilePosition == oldTilePosition) {
	    // havent changed infantry position
		GroundUnit::setSpeeds();
	} else {

		int sx = tilePositionOffset[oldTilePosition].x;
		int sy = tilePositionOffset[oldTilePosition].y;

		int dx = 0;
		int dy = 0;
		switch(drawnAngle) {
            case RIGHT:     dx += TILESIZE;                 break;
            case RIGHTUP:   dx += TILESIZE; dy -= TILESIZE; break;
            case UP:                        dy -= TILESIZE; break;
            case LEFTUP:    dx -= TILESIZE; dy -= TILESIZE; break;
            case LEFT:      dx -= TILESIZE;                 break;
            case LEFTDOWN:  dx -= TILESIZE; dy += TILESIZE; break;
            case DOWN:                      dy += TILESIZE; break;
            case RIGHTDOWN: dx += TILESIZE; dy += TILESIZE; break;
		}

		if(tilePosition != INVALID_POS) {
		    dx += tilePositionOffset[tilePosition].x;
			dy += tilePositionOffset[tilePosition].y;
		}

		dx -= sx;
		dy -= sy;

		float scale = currentGame->objectData.data[itemID][originalHouseID].maxspeed/std::sqrt((float)(dx*dx + dy*dy));
		xSpeed = dx*scale;
		ySpeed = dy*scale;
	}
}

void InfantryBase::squash() {
	destroy();
	return;
}

void InfantryBase::playConfirmSound() {
	soundPlayer->playSound((Sound_enum) getRandomOf(2,MovingOut,InfantryOut));
}

void InfantryBase::playSelectSound() {
	soundPlayer->playSound(YesSir);
}
