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


#include <players/AIPlayer.h>

#include <Game.h>
#include <GameInitSettings.h>
#include <Map.h>
#include <sand.h>
#include <House.h>

#include <structures/StructureBase.h>
#include <structures/BuilderBase.h>
#include <structures/StarPort.h>
#include <structures/ConstructionYard.h>
#include <units/UnitBase.h>
#include <units/MCV.h>
#include <units/Harvester.h>

#include <algorithm>

#define AIUPDATEINTERVAL 50

AIPlayer::AIPlayer(House* associatedHouse, std::string playername, Uint8 difficulty)
 : Player(associatedHouse, playername), difficulty(difficulty) {
	attackTimer = ((2-difficulty) * MILLI2CYCLES(2*60*1000)) + getRandomGen().rand(MILLI2CYCLES(8*60*1000), MILLI2CYCLES(11*60*1000));
	buildTimer = getRandomGen().rand(0,3) * 50;
}

AIPlayer::AIPlayer(InputStream& stream, House* associatedHouse) : Player(stream, associatedHouse) {
    AIPlayer::init();

	difficulty = stream.readUint8();
	attackTimer = stream.readSint32();
	buildTimer = stream.readSint32();

	Uint32 NumPlaceLocations = stream.readUint32();
	for(Uint32 i = 0; i < NumPlaceLocations; i++) {
        Sint32 x = stream.readSint32();
        Sint32 y = stream.readSint32();

		placeLocations.push_back(Coord(x,y));
	}
}

void AIPlayer::init() {
}


AIPlayer::~AIPlayer() {
}

void AIPlayer::save(OutputStream& stream) const {
    Player::save(stream);

	stream.writeUint8(difficulty);
	stream.writeSint32(attackTimer);
    stream.writeSint32(buildTimer);

	stream.writeUint32(placeLocations.size());
	std::list<Coord>::const_iterator iter;
	for(iter = placeLocations.begin(); iter != placeLocations.end(); ++iter) {
		stream.writeSint32(iter->x);
		stream.writeSint32(iter->y);
	}
}



void AIPlayer::update() {
    if( (getGameCylceCount() + getHouse()->getHouseID()) % AIUPDATEINTERVAL != 0) {
        // we are not updating this AI player this cycle
        return;
    }

    checkAllUnits();

	if(buildTimer <= 0) {
        build();
	} else {
        buildTimer -= AIUPDATEINTERVAL;
	}

	if(attackTimer <= 0) {
        attack();
	} else {
        attackTimer -= AIUPDATEINTERVAL;
	}
}

void AIPlayer::onIncrementStructures(int itemID) {
}

void AIPlayer::onDecrementStructures(int itemID, const Coord& location) {
}

void AIPlayer::onDamage(const ObjectBase* pObject, int damage, Uint32 damagerID) {
    const ObjectBase* pDamager = getObject(damagerID);

    if(pDamager == NULL || pDamager->getOwner()->getTeam() == getHouse()->getTeam()) {
        return;
    }

    if(pObject->isAStructure()) {
	    //scramble some free units to defend
        scrambleUnitsAndDefend(pDamager);
    } else if(pObject->getItemID() == Unit_Harvester) {
	    //scramble some free units to defend
        scrambleUnitsAndDefend(pDamager);

        if((pDamager != NULL) && pDamager->isInfantry()) {
            const UnitBase* pUnit = dynamic_cast<const UnitBase*>(pObject);
            doAttackObject(pUnit, pDamager, false);
        }
	} else if(pObject->isAUnit() && pObject->canAttack(pDamager)) {
        const UnitBase* pUnit = dynamic_cast<const UnitBase*>(pObject);

        if(pUnit->getAttackMode() == GUARD || pUnit->getAttackMode() == AMBUSH) {
            doSetAttackMode(pUnit, HUNT);
            doAttackObject(pUnit, pDamager, false);
        } else if(pUnit->getAttackMode() == AREAGUARD) {
            doAttackObject(pUnit, pDamager, false);
        }
	}
}

void AIPlayer::scrambleUnitsAndDefend(const ObjectBase* pIntruder) {
    RobustList<const UnitBase*>::const_iterator iter;
    for(iter = getUnitList().begin(); iter != getUnitList().end(); ++iter) {
        const UnitBase* pUnit = *iter;
        if(pUnit->isRespondable() && (pUnit->getOwner() == getHouse())) {

            if((pUnit->getAttackMode() != HUNT) && !pUnit->hasATarget()) {
                Uint32 itemID = pUnit->getItemID();
                if((itemID != Unit_Harvester) && (pUnit->getItemID() != Unit_MCV) && (pUnit->getItemID() != Unit_Carryall)
                    && (pUnit->getItemID() != Unit_Frigate) && (pUnit->getItemID() != Unit_Saboteur) && (pUnit->getItemID() != Unit_Sandworm)) {
                    doAttackObject(pUnit, pIntruder, true);
                }
            }
        }
    }
}

Coord AIPlayer::findPlaceLocation(Uint32 itemID) {
    int structureSizeX = getStructureSize(itemID).x;
    int structureSizeY = getStructureSize(itemID).y;

	int minX = getMap().getSizeX();
	int maxX = -1;
    int minY = getMap().getSizeY();
    int maxY = -1;

    if(itemID == Structure_ConstructionYard || itemID == Structure_Slab1) {
        // construction yard can only be build with mcv and thus be build anywhere
        minX = 0;
        minY = 0;
        maxX = getMap().getSizeX() - 1;
        maxY = getMap().getSizeY() - 1;
    } else {
        RobustList<const StructureBase*>::const_iterator iter;
        for(iter = getStructureList().begin(); iter != getStructureList().end(); ++iter) {
            const StructureBase* structure = *iter;
            if (structure->getOwner() == getHouse()) {
                if (structure->getX() < minX)
                    minX = structure->getX();
                if (structure->getX() > maxX)
                    maxX = structure->getX();
                if (structure->getY() < minY)
                    minY = structure->getY();
                if (structure->getY() > maxY)
                    maxY = structure->getY();
            }
        }
	}

    // make search rect a bit bigger to make it possible to build on places far off the main base and only connected through slab
	minX -= structureSizeX + 5;
	maxX += 5;
	minY -= structureSizeY + 5;
	maxY += 5;
	if (minX < 0) minX = 0;
	if (maxX >= getMap().getSizeX()) maxX = getMap().getSizeX() - structureSizeX;
	if (minY < 0) minY = 0;
	if (maxY >= getMap().getSizeY()) maxY = getMap().getSizeY() - structureSizeY;

    float bestrating = 0.0f;
	Coord bestLocation = Coord::Invalid();
	int count = 0;
	do {
	    int x = getRandomGen().rand(minX, maxX);
        int y = getRandomGen().rand(minY, maxY);

	    Coord pos = Coord(x, y);

		count++;

		if(getMap().okayToPlaceStructure(pos.x, pos.y, structureSizeX, structureSizeY, false, (itemID == Structure_ConstructionYard) ? NULL : getHouse())) {
            float rating;

		    switch(itemID) {
                case Structure_Slab1: {
                    rating = 10000000.0f;
                } break;

                case Structure_Refinery: {
                    // place near spice
                    Coord spicePos;
                    if(getMap().findSpice(spicePos, pos)) {
                        rating = 10000000.0f - blockDistance(pos, spicePos);
                    } else {
                        rating = 10000000.0f;
                    }
                } break;


                case Structure_ConstructionYard: {
                    float nearestUnit = 10000000.0f;

                    RobustList<const UnitBase*>::const_iterator iter;
                    for(iter = getUnitList().begin(); iter != getUnitList().end(); ++iter) {
                        const UnitBase* pUnit = *iter;
                        if(pUnit->getOwner() == getHouse()) {
                            float tmp = blockDistance(pos, pUnit->getLocation());
                            if(tmp < nearestUnit) {
                                nearestUnit = tmp;
                            }
                        }
                    }

                    rating = 10000000.0f - nearestUnit;
                } break;

                case Structure_Barracks:
                case Structure_HeavyFactory:
                case Structure_LightFactory:
                case Structure_RepairYard:
                case Structure_StarPort:
                case Structure_WOR: {
                    // place near sand

                    float nearestSand = 10000000.0f;

                    for(int y = 0 ; y < currentGameMap->getSizeY(); y++) {
                        for(int x = 0; x < currentGameMap->getSizeX(); x++) {
                            int type = currentGameMap->getTile(x,y)->getType();

                            if(type != Terrain_Rock || type != Terrain_Slab || type != Terrain_Mountain) {
                                float tmp = blockDistance(pos, Coord(x,y));
                                if(tmp < nearestSand) {
                                    nearestSand = tmp;
                                }
                            }
                        }
                    }

                    rating = 10000000.0f - nearestSand;
                    rating *= (1+getNumAdjacentStructureTiles(pos, structureSizeX, structureSizeY));
                } break;

                case Structure_Wall:
                case Structure_GunTurret:
                case Structure_RocketTurret: {
                    // place towards enemy
                    float nearestEnemy = 10000000.0f;

                    RobustList<const StructureBase*>::const_iterator iter2;
                    for(iter2 = getStructureList().begin(); iter2 != getStructureList().end(); ++iter2) {
                        const StructureBase* pStructure = *iter2;
                        if(pStructure->getOwner()->getTeam() != getHouse()->getTeam()) {

                            float tmp = blockDistance(pos, pStructure->getLocation());
                            if(tmp < nearestEnemy) {
                                nearestEnemy = tmp;
                            }
                        }
                    }

                    rating = 10000000.0f - nearestEnemy;
                } break;

                case Structure_HighTechFactory:
                case Structure_IX:
                case Structure_Palace:
                case Structure_Radar:
                case Structure_Silo:
                case Structure_WindTrap:
                default: {
                    // place at a save place
                    float nearestEnemy = 10000000.0f;

                    RobustList<const StructureBase*>::const_iterator iter2;
                    for(iter2 = getStructureList().begin(); iter2 != getStructureList().end(); ++iter2) {
                        const StructureBase* pStructure = *iter2;
                        if(pStructure->getOwner()->getTeam() != getHouse()->getTeam()) {

                            float tmp = blockDistance(pos, pStructure->getLocation());
                            if(tmp < nearestEnemy) {
                                nearestEnemy = tmp;
                            }
                        }
                    }

                    rating = nearestEnemy;
                    rating *= (1+getNumAdjacentStructureTiles(pos, structureSizeX, structureSizeY));
                } break;
		    }

		    if(rating > bestrating) {
                bestLocation = pos;
                bestrating = rating;
		    }
		}

	} while(count <= ((itemID == Structure_ConstructionYard) ? 10000 : 100));

	return bestLocation;
}

int AIPlayer::getNumAdjacentStructureTiles(Coord pos, int structureSizeX, int structureSizeY) {

    int numAdjacentStructureTiles = 0;

    for(int y = pos.y; y < pos.y + structureSizeY; y++) {
        if(getMap().tileExists(pos.x-1, y) && getMap().getTile(pos.x-1, y)->hasAStructure()) {
            numAdjacentStructureTiles++;
        }
        if(getMap().tileExists(pos.x+structureSizeX, y) && getMap().getTile(pos.x+structureSizeX, y)->hasAStructure()) {
            numAdjacentStructureTiles++;
        }
    }

    for(int x = pos.x; x < pos.x + structureSizeX; x++) {
        if(getMap().tileExists(x, pos.y-1) && getMap().getTile(x, pos.y-1)->hasAStructure()) {
            numAdjacentStructureTiles++;
        }
        if(getMap().tileExists(x, pos.y+structureSizeY) && getMap().getTile(x, pos.y+structureSizeY)->hasAStructure()) {
            numAdjacentStructureTiles++;
        }
    }

    return numAdjacentStructureTiles;
}

void AIPlayer::build() {
	bool bConstructionYardChecked = false;

    RobustList<const StructureBase*>::const_iterator iter;
    for(iter = getStructureList().begin(); iter != getStructureList().end(); ++iter) {
        const StructureBase* pStructure = *iter;

        //if this players structure, and its a heavy factory, build something
        if(pStructure->getOwner() == getHouse()) {

            if((pStructure->isRepairing() == false) && (pStructure->getHealth() < pStructure->getMaxHealth())) {
                doRepair(pStructure);
            }

            const BuilderBase* pBuilder = dynamic_cast<const BuilderBase*>(pStructure);
            if(pBuilder != NULL) {

                if((getHouse()->getCredits() > 2000) && (pBuilder->getHealth() >= pBuilder->getMaxHealth()) && (pBuilder->isUpgrading() == false) && (pBuilder->getCurrentUpgradeLevel() < pBuilder->getMaxUpgradeLevel())) {
                    doUpgrade(pBuilder);
                    continue;
                }

                switch (pStructure->getItemID()) {

                    case Structure_Barracks: {
                        if(isAllowedToArm() && (getHouse()->hasLightFactory() == false) && (getHouse()->hasHeavyFactory() == false)) {
                            if((getHouse()->getCredits() > 1500) && (pBuilder->getProductionQueueSize() < 1) && (pBuilder->getBuildListSize() > 0)) {
                                doBuildRandom(pBuilder);
                            }
                        }
                    } break;

                    case Structure_LightFactory: {
                        if(isAllowedToArm() && getHouse()->hasHeavyFactory() == false) {
                            if((getHouse()->getCredits() > 1500) && (pBuilder->getProductionQueueSize() < 1) && (pBuilder->getBuildListSize() > 0)) {
                                doBuildRandom(pBuilder);
                            }
                        }
                    } break;

                    case Structure_WOR: {
                        if(isAllowedToArm() && getHouse()->hasHeavyFactory() == false) {
                            if((getHouse()->getCredits() > 1500) && (pBuilder->getProductionQueueSize() < 1) && (pBuilder->getBuildListSize() > 0)) {
                                doBuildRandom(pBuilder);
                            }
                        }
                    } break;

                    case Structure_HeavyFactory: {
                        if(isAllowedToArm() && (pBuilder->getProductionQueueSize() < 1) && (pBuilder->getBuildListSize() > 0)) {

                            if(getHouse()->getNumItems(Unit_Harvester) < getMaxHarvester()) {
                                doProduceItem(pBuilder, Unit_Harvester);
                            } else if(getHouse()->getCredits() > 1500) {
                                int numTanks = getHouse()->getNumItems(Unit_Devastator) + getHouse()->getNumItems(Unit_SiegeTank) + getHouse()->getNumItems(Unit_Tank);
                                int numLauncher = getHouse()->getNumItems(Unit_Launcher) + getHouse()->getNumItems(Unit_Deviator);

                                if(pBuilder->isAvailableToBuild(Unit_SonicTank)) {
                                    doProduceItem(pBuilder, Unit_SonicTank);
                                } else if(pBuilder->isAvailableToBuild(Unit_Devastator) && numTanks <= 5*numLauncher && getHouse()->getNumItems(Unit_Devastator) < getHouse()->getNumItems(Unit_SiegeTank)) {
                                    doProduceItem(pBuilder, Unit_Devastator);
                                } else if(pBuilder->isAvailableToBuild(Unit_Deviator) && 5*getHouse()->getNumItems(Unit_Deviator) <= numTanks) {
                                    doProduceItem(pBuilder, Unit_Deviator);
                                } else if(pBuilder->isAvailableToBuild(Unit_SiegeTank) && numTanks <= 5*numLauncher) {
                                    doProduceItem(pBuilder, Unit_SiegeTank);
                                } else if(pBuilder->isAvailableToBuild(Unit_Launcher) && 5*numLauncher <= numTanks) {
                                    doProduceItem(pBuilder, Unit_Launcher);
                                } else if(pBuilder->isAvailableToBuild(Unit_SiegeTank)) {
                                    doProduceItem(pBuilder, Unit_SiegeTank);
                                } else if(pBuilder->isAvailableToBuild(Unit_Tank)) {
                                    doProduceItem(pBuilder, Unit_Tank);
                                }
                            }
                        }
                    } break;

                    case Structure_HighTechFactory: {
                        if(isAllowedToArm() && (getHouse()->getCredits() > 800) && (pBuilder->getProductionQueueSize() < 1)) {

                            if(getHouse()->getNumItems(Unit_Carryall) < (getHouse()->getNumItems(Unit_Harvester)+1)/2) {
                                doProduceItem(pBuilder, Unit_Carryall);
                            } else if(getHouse()->getCredits() > 2500) {
                                doProduceItem(pBuilder, Unit_Ornithopter);
                            }
                        }
                    } break;

                    case Structure_StarPort: {
                        const StarPort* pStarPort = dynamic_cast<const StarPort*>(pBuilder);
                        if(isAllowedToArm() && pStarPort->okToOrder())	{
                            const Choam& choam = getHouse()->getChoam();

                            if(getHouse()->getNumItems(Unit_Harvester) < getMaxHarvester() && choam.getNumAvailable(Unit_Harvester) > 0) {
                                if(getHouse()->getCredits() > 300) {
                                    doProduceItem(pBuilder, Unit_Harvester);
                                    if(getHouse()->getCredits() > 300 && choam.getNumAvailable(Unit_Harvester) > 0) {
                                        doProduceItem(pBuilder, Unit_Harvester);
                                    }
                                    doPlaceOrder(pStarPort);
                                }
                            } else if(getHouse()->getNumItems(Unit_Carryall) < (getHouse()->getNumItems(Unit_Harvester)+1)/2 && choam.getNumAvailable(Unit_Carryall) > 0) {
                                if(getHouse()->getCredits() > 800) {
                                    doProduceItem(pBuilder, Unit_Carryall);
                                    doPlaceOrder(pStarPort);
                                }
                            } else {
                                // order max 6 units
                                int num = 6;
                                while((num > 0) && (getHouse()->getCredits() > 2000)) {
                                    if(pStarPort->isAvailableToBuild(Unit_SiegeTank) && choam.getNumAvailable(Unit_SiegeTank) > 0 && choam.isCheap(Unit_SiegeTank)) {
                                        doProduceItem(pBuilder, Unit_SiegeTank);
                                    } else if(pStarPort->isAvailableToBuild(Unit_Launcher) && choam.getNumAvailable(Unit_Launcher) > 0 && choam.isCheap(Unit_Launcher)) {
                                        doProduceItem(pBuilder, Unit_Launcher);
                                    } else if(pStarPort->isAvailableToBuild(Unit_Tank) && choam.getNumAvailable(Unit_Tank) > 0 && choam.isCheap(Unit_Tank)) {
                                        doProduceItem(pBuilder, Unit_Tank);
                                    } else if(pStarPort->isAvailableToBuild(Unit_Quad) && choam.getNumAvailable(Unit_Quad) > 0 && choam.isCheap(Unit_Quad)) {
                                        doProduceItem(pBuilder, Unit_Quad);
                                    } else if(pStarPort->isAvailableToBuild(Unit_Trike) && choam.getNumAvailable(Unit_Trike) > 0 && choam.isCheap(Unit_Trike)) {
                                        doProduceItem(pBuilder, Unit_Trike);
                                    }
                                    num--;
                                }
                                doPlaceOrder(pStarPort);
                            }
                        }
                    } break;

                    case Structure_ConstructionYard: {
                        if((getHouse()->getCredits() > 900) && ((pBuilder->getCurrentUpgradeLevel() == 0) || (getHouse()->hasRadar()))
                            && (pBuilder->getHealth() >= pBuilder->getMaxHealth())
                            && (pBuilder->isUpgrading() == false)
                            && (pBuilder->getCurrentUpgradeLevel() < pBuilder->getMaxUpgradeLevel()) ) {
                            doUpgrade(pBuilder);
                        }

                        if(bConstructionYardChecked == false && !pBuilder->isUpgrading()) {
                            bConstructionYardChecked = true;
                            if(getHouse()->getCredits() > 100) {
                                if((pBuilder->getProductionQueueSize() < 1) && (pBuilder->getBuildListSize() > 0)) {
                                    Uint32 itemID = NONE;
                                    if(getHouse()->getProducedPower() - getHouse()->getPowerRequirement() < 50 && pBuilder->isAvailableToBuild(Structure_WindTrap)) {
                                        itemID = Structure_WindTrap;
                                    } else if(getHouse()->getNumItems(Structure_Refinery) < 3 && pBuilder->isAvailableToBuild(Structure_Refinery)) {
                                        itemID = Structure_Refinery;
                                    } else if((getHouse()->hasRadar() == false) && pBuilder->isAvailableToBuild(Structure_Radar)) {
                                        itemID = Structure_Radar;
                                    } else if((getHouse()->getNumItems(Structure_StarPort) <= 0) && pBuilder->isAvailableToBuild(Structure_StarPort)) {
                                        itemID = Structure_StarPort;
                                    } else if((getHouse()->getNumItems(Structure_RocketTurret) < 1) && pBuilder->isAvailableToBuild(Structure_RocketTurret)) {
                                        itemID = Structure_RocketTurret;
                                    } else if((getHouse()->hasLightFactory() == false) && pBuilder->isAvailableToBuild(Structure_LightFactory)) {
                                        itemID = Structure_LightFactory;
                                    } else if((getHouse()->getNumItems(Structure_HeavyFactory) <= 0) && pBuilder->isAvailableToBuild(Structure_HeavyFactory)) {
                                        itemID = Structure_HeavyFactory;
                                    } else if(getHouse()->getCredits() < 1000) {
                                        // we don't need any more buildings if we have such few credits
                                    } else if((getHouse()->getNumItems(Structure_RocketTurret) < 3) && pBuilder->isAvailableToBuild(Structure_RocketTurret)) {
                                        itemID = Structure_RocketTurret;
                                    } else if((getHouse()->getNumItems(Structure_IX) <= 0) && pBuilder->isAvailableToBuild(Structure_IX)) {
                                        itemID = Structure_IX;
                                    } else if((getHouse()->getNumItems(Structure_RepairYard) <= 0) && pBuilder->isAvailableToBuild(Structure_RepairYard)) {
                                        itemID = Structure_RepairYard;
                                    } else if((getHouse()->getNumItems(Structure_Palace) <= 0) && pBuilder->isAvailableToBuild(Structure_Palace)) {
                                        itemID = Structure_Palace;
                                    } else if((getHouse()->getNumItems(Structure_RocketTurret) < 4) && pBuilder->isAvailableToBuild(Structure_RocketTurret)) {
                                        itemID = Structure_RocketTurret;
                                    } else if((getHouse()->getNumItems(Structure_WOR) <= 0) && pBuilder->isAvailableToBuild(Structure_WOR)) {
                                        itemID = Structure_WOR;
                                    } else if((getHouse()->getNumItems(Structure_HighTechFactory) <= 0) && pBuilder->isAvailableToBuild(Structure_HighTechFactory)) {
                                        itemID = Structure_HighTechFactory;
                                    } else if((getHouse()->getNumItems(Structure_RocketTurret) < 5) && pBuilder->isAvailableToBuild(Structure_RocketTurret)) {
                                        itemID = Structure_RocketTurret;
                                    } else if(!pBuilder->isAvailableToBuild(Structure_HeavyFactory) && (getHouse()->getNumItems(Structure_LightFactory) < 2) && pBuilder->isAvailableToBuild(Structure_LightFactory)) {
                                        itemID = Structure_LightFactory;
                                    } else if((getHouse()->getNumItems(Structure_HeavyFactory) < 2) && pBuilder->isAvailableToBuild(Structure_HeavyFactory)) {
                                        itemID = Structure_HeavyFactory;
                                    } else if(getHouse()->getCredits() > 2000 && (getHouse()->getNumItems(Structure_Silo) < 2) && pBuilder->isAvailableToBuild(Structure_Silo)) {
                                        itemID = Structure_Silo;
                                    } else if(getHouse()->getCredits() > 2000 && (getHouse()->getNumItems(Structure_RepairYard) < 2) && pBuilder->isAvailableToBuild(Structure_RepairYard)) {
                                        itemID = Structure_RepairYard;
                                    } else if(((difficulty == AIPlayer::MEDIUM) || (difficulty == AIPlayer::HARD)) && getHouse()->getNumItems(Structure_Refinery) < 4 && pBuilder->isAvailableToBuild(Structure_Refinery)) {
                                        itemID = Structure_Refinery;
                                    } else if((difficulty == AIPlayer::HARD) && getHouse()->getNumItems(Structure_Refinery) < 5 && pBuilder->isAvailableToBuild(Structure_Refinery)) {
                                        itemID = Structure_Refinery;
                                    } else if((getHouse()->getNumItems(Structure_HeavyFactory) < 3) && pBuilder->isAvailableToBuild(Structure_HeavyFactory)) {
                                        itemID = Structure_HeavyFactory;
                                    } else if(getHouse()->getCredits() > 2000 && (getHouse()->getNumItems(Structure_RocketTurret) < 10) && pBuilder->isAvailableToBuild(Structure_RocketTurret)) {
                                        itemID = Structure_RocketTurret;
                                    } else if(getHouse()->getCredits() > 3000 && (getHouse()->getNumItems(Structure_RocketTurret) < 20) && pBuilder->isAvailableToBuild(Structure_RocketTurret)) {
                                        itemID = Structure_RocketTurret;
                                    }

                                    if(itemID != NONE) {
                                        Coord location = findPlaceLocation(itemID);

                                        if(location.isValid()) {
                                            Coord placeLocation = location;
                                            if(getGameInitSettings().getGameOptions().concreteRequired) {
                                                int incI;
                                                int incJ;
                                                int startI;
                                                int startJ;

                                                if(getMap().isWithinBuildRange(location.x, location.y, getHouse())) {
                                                    startI = location.x, startJ = location.y, incI = 1, incJ = 1;
                                                } else if(getMap().isWithinBuildRange(location.x + getStructureSize(itemID).x - 1, location.y, getHouse())) {
                                                    startI = location.x + getStructureSize(itemID).x - 1, startJ = location.y, incI = -1, incJ = 1;
                                                } else if(getMap().isWithinBuildRange(location.x, location.y + getStructureSize(itemID).y - 1, getHouse())) {
                                                    startI = location.x, startJ = location.y + getStructureSize(itemID).y - 1, incI = 1, incJ = -1;
                                                } else {
                                                    startI = location.x + getStructureSize(itemID).x - 1, startJ = location.y + getStructureSize(itemID).y - 1, incI = -1, incJ = -1;
                                                }

                                                for(int i = startI; abs(i - startI) < getStructureSize(itemID).x; i += incI) {
                                                    for(int j = startJ; abs(j - startJ) < getStructureSize(itemID).y; j += incJ) {
                                                        const Tile *pTile = getMap().getTile(i, j);

                                                        if((getStructureSize(itemID).x > 1) && (getStructureSize(itemID).y > 1)
                                                            && pBuilder->isAvailableToBuild(Structure_Slab4)
                                                            && (abs(i - location.x) < 2) && (abs(j - location.y) < 2)) {
                                                            if( (i == location.x) && (j == location.y) && pTile->getType() != Terrain_Slab) {
                                                                placeLocations.push_back(Coord(i,j));
                                                                doProduceItem(pBuilder, Structure_Slab4);
                                                            }
                                                        } else if(pTile->getType() != Terrain_Slab) {
                                                            placeLocations.push_back(Coord(i,j));
                                                            doProduceItem(pBuilder, Structure_Slab1);
                                                        }
                                                    }
                                                }
                                            }

                                            placeLocations.push_back(placeLocation);
                                            doProduceItem(pBuilder, itemID);
                                        } else {
                                            // we havn't found a placing location => build some random slabs
                                            location = findPlaceLocation(Structure_Slab1);
                                            if(location.isValid() && getMap().isWithinBuildRange(location.x, location.y, getHouse())) {
                                                placeLocations.push_back(location);
                                                doProduceItem(pBuilder, Structure_Slab1);
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        if(pBuilder->isWaitingToPlace()) {
                            //find total region of possible placement and place in random ok position
                            int itemID = pBuilder->getCurrentProducedItem();
                            Coord itemsize = getStructureSize(itemID);

                            //see if there is already a spot to put it stored
                            if(!placeLocations.empty()) {
                                Coord location = placeLocations.front();
                                const ConstructionYard* pConstYard = dynamic_cast<const ConstructionYard*>(pBuilder);
                                if(getMap().okayToPlaceStructure(location.x, location.y, itemsize.x, itemsize.y, false, pConstYard->getOwner())) {
                                    doPlaceStructure(pConstYard, location.x, location.y);
                                    placeLocations.pop_front();
                                } else if(itemID == Structure_Slab1) {
                                    //forget about concrete
                                    doCancelItem(pConstYard, Structure_Slab1);
                                    placeLocations.pop_front();
                                } else if(itemID == Structure_Slab4) {
                                    //forget about concrete
                                    doCancelItem(pConstYard, Structure_Slab4);
                                    placeLocations.pop_front();
                                } else {
                                    //cancel item
                                    doCancelItem(pConstYard, itemID);
                                    placeLocations.pop_front();
                                }
                            }
                        }

                    } break;

                    default: {
                        break;
                    }
                }
            }
        }

    }

    buildTimer = getRandomGen().rand(0,3)*50;
}

void AIPlayer::attack() {
    Coord destination;
    const UnitBase* pLeaderUnit = NULL;
    RobustList<const UnitBase*>::const_iterator iter;
    for(iter = getUnitList().begin(); iter != getUnitList().end(); ++iter) {
        const UnitBase *pUnit = *iter;
        if (pUnit->isRespondable()
            && (pUnit->getOwner() == getHouse())
            && pUnit->isActive()
            /*&& !(pUnit->getAttackMode() == HUNT)*/
            && (pUnit->getAttackMode() == AREAGUARD || pUnit->getAttackMode() == GUARD || pUnit->getAttackMode() == AMBUSH)
            && (pUnit->getItemID() != Unit_Harvester)
            && (pUnit->getItemID() != Unit_MCV)
            && (pUnit->getItemID() != Unit_Carryall)
            && (pUnit->getItemID() != Unit_Saboteur)) {

            if(pLeaderUnit == NULL) {
                pLeaderUnit = pUnit;

                //default destination
                destination.x = pLeaderUnit->getX();
                destination.y = pLeaderUnit->getY();

                const StructureBase* closestStructure = pLeaderUnit->findClosestTargetStructure();
                if(closestStructure) {
                    destination = closestStructure->getClosestPoint(pLeaderUnit->getLocation());
                } else {
                    const UnitBase* closestUnit = pLeaderUnit->findClosestTargetUnit();
                    if(closestUnit) {
                        destination.x = closestUnit->getX();
                        destination.y = closestUnit->getY();
                    }
                }
            }

            doMove2Pos(pUnit, destination.x, destination.y, false);
            doSetAttackMode(pUnit, HUNT);
        }
    }

    //reset timer for next attack
    attackTimer = getRandomGen().rand(10000, 20000);
}

void AIPlayer::checkAllUnits() {
    RobustList<const UnitBase*>::const_iterator iter;
    for(iter = getUnitList().begin(); iter != getUnitList().end(); ++iter) {
        const UnitBase* pUnit = *iter;

        if(pUnit->getItemID() == Unit_Sandworm) {
                RobustList<const UnitBase*>::const_iterator iter2;
                for(iter2 = getUnitList().begin(); iter2 != getUnitList().end(); ++iter2) {
                    const UnitBase* pUnit2 = *iter2;

                    if(pUnit2->getOwner() == getHouse() && pUnit2->getItemID() == Unit_Harvester) {
                        const Harvester* pHarvester = dynamic_cast<const Harvester*>(pUnit2);
                        if( pHarvester != NULL
                            && getMap().tileExists(pHarvester->getLocation())
                            && !getMap().getTile(pHarvester->getLocation())->isRock()
                            && blockDistance(pUnit->getLocation(), pHarvester->getLocation()) <= 5) {
                            doReturn(pHarvester);
                            scrambleUnitsAndDefend(pUnit);
                        }
                    }
                }
        }

        if(pUnit->getOwner() != getHouse()) {
            continue;
        }

        switch(pUnit->getItemID()) {
            case Unit_MCV: {
                const MCV* pMCV = dynamic_cast<const MCV*>(pUnit);
                if(!pMCV->isMoving()) {
                    if(pMCV->canDeploy()) {
                        doDeploy(pMCV);
                    } else {
                        Coord pos = findPlaceLocation(Structure_ConstructionYard);
                        doMove2Pos(pMCV, pos.x, pos.y, true);
                    }
                }
            } break;

            case Unit_Harvester: {
                const Harvester* pHarvester = dynamic_cast<const Harvester*>(pUnit);
                if(getHouse()->getNumItems(Unit_Harvester) < 3 && pHarvester->getAmountOfSpice() >= HARVESTERMAXSPICE/2) {
                    doReturn(pHarvester);
                }
            } break;

            default: {
            } break;
        }
    }
}

bool AIPlayer::isAllowedToArm() const {
    int teamScore[NUM_HOUSES+1];

    for(int i = 0; i < NUM_HOUSES+1; i++) {
        teamScore[i] = 0;
    }

    int maxTeamScore = 0;
    for(int i = 0; i < NUM_HOUSES; i++) {
        const House* pHouse = getHouse(i);
        if(pHouse != NULL) {
            teamScore[pHouse->getTeam()] += pHouse->getUnitBuiltValue();

            if(pHouse->getTeam() != getHouse()->getTeam()) {
                maxTeamScore = std::max(maxTeamScore, teamScore[pHouse->getTeam()]);
            }
        }
    }

    int ownTeamScore = teamScore[getHouse()->getTeam()];

    switch(difficulty) {
        case EASY: {
            return (ownTeamScore < maxTeamScore);
        } break;

        case MEDIUM: {
            return (ownTeamScore < 2*maxTeamScore);
        } break;

        case HARD:
        default: {
            return true;
        } break;

    }
}


int AIPlayer::getMaxHarvester() const {
    switch(difficulty) {
        case AIPlayer::EASY: {
            return getHouse()->getNumItems(Structure_Refinery);
        }

        case AIPlayer::MEDIUM: {
            return (2*getHouse()->getNumItems(Structure_Refinery)+1)/3;
        }

        case AIPlayer::HARD:
        default: {
            return 2*getHouse()->getNumItems(Structure_Refinery);
        }
    }
}
