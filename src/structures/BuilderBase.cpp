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

#include <structures/BuilderBase.h>

#include <FileClasses/TextManager.h>

#include <globals.h>

#include <SoundPlayer.h>
#include <Map.h>
#include <House.h>
#include <units/UnitBase.h>

#include <players/HumanPlayer.h>

#include <GUI/ObjectInterfaces/BuilderInterface.h>

const int BuilderBase::itemOrder[] = {    Structure_Slab4, Structure_Slab1, Structure_IX, Structure_StarPort,
                                           Structure_HighTechFactory, Structure_HeavyFactory, Structure_RocketTurret,
                                           Structure_RepairYard, Structure_GunTurret, Structure_WOR,
                                           Structure_Barracks, Structure_Wall, Structure_LightFactory,
                                           Structure_Silo, Structure_Radar, Structure_Refinery, Structure_WindTrap,
                                           Structure_Palace,
                                           Unit_SonicTank, Unit_Devastator, Unit_Deviator, Unit_Special,
                                           Unit_Launcher, Unit_SiegeTank, Unit_Tank, Unit_MCV, Unit_Harvester,
                                           Unit_Ornithopter, Unit_Carryall, Unit_Quad, Unit_RaiderTrike,
                                           Unit_Trike, Unit_Troopers, Unit_Trooper, Unit_Infantry, Unit_Soldier,
                                           Unit_Frigate, Unit_Sandworm, Unit_Saboteur, ItemID_Invalid };

BuilderBase::BuilderBase(House* newOwner) : StructureBase(newOwner) {
    BuilderBase::init();

	curUpgradeLev = 0;
	upgradeProgress = 0.0f;
	upgrading = false;

	currentProducedItem = ItemID_Invalid;
	bCurrentItemOnHold = false;
	productionProgress = 0.0f;
	deployTimer = 0;
}

BuilderBase::BuilderBase(InputStream& stream) : StructureBase(stream) {
    BuilderBase::init();

	upgrading = stream.readBool();
	upgradeProgress = stream.readFloat();
    curUpgradeLev = stream.readUint8();

	bCurrentItemOnHold = stream.readBool();
	currentProducedItem = stream.readUint32();
	productionProgress = stream.readFloat();
	deployTimer = stream.readUint32();

	int numProductionQueueItem = stream.readUint32();
	for(int i=0;i<numProductionQueueItem;i++) {
		ProductionQueueItem tmp;
		tmp.load(stream);
		currentProductionQueue.push_back(tmp);
	}

	int numBuildItem = stream.readUint32();
	for(int i=0;i<numBuildItem;i++) {
		BuildItem tmp;
		tmp.load(stream);
		buildList.push_back(tmp);
	}
}

void BuilderBase::init() {
    aBuilder = true;
}

BuilderBase::~BuilderBase() {
}


void BuilderBase::save(OutputStream& stream) const {
	StructureBase::save(stream);

    stream.writeBool(upgrading);
    stream.writeFloat(upgradeProgress);
	stream.writeUint8(curUpgradeLev);

	stream.writeBool(bCurrentItemOnHold);
	stream.writeUint32(currentProducedItem);
	stream.writeFloat(productionProgress);
	stream.writeUint32(deployTimer);

	stream.writeUint32(currentProductionQueue.size());
	std::list<ProductionQueueItem>::const_iterator iter;
	for(iter = currentProductionQueue.begin(); iter != currentProductionQueue.end(); ++iter) {
		iter->save(stream);
	}

	stream.writeUint32(buildList.size());
	std::list<BuildItem>::const_iterator iter2;
	for(iter2 = buildList.begin(); iter2 != buildList.end(); ++iter2) {
		iter2->save(stream);
	}
}

ObjectInterface* BuilderBase::getInterfaceContainer() {
	if((pLocalHouse == owner) || (debug == true)) {
		return BuilderInterface::create(objectID);
	} else {
		return DefaultObjectInterface::create(objectID);
	}
}

void BuilderBase::insertItem(std::list<BuildItem>& buildItemList, std::list<BuildItem>::iterator& iter, Uint32 itemID, int price) {
	if(iter != buildItemList.end()) {
		if(iter->itemID == itemID) {
			if(price != -1) {
				iter->price = price;
			}
			++iter;
			return;
		}
	}

	if(price == -1) {
        price = currentGame->objectData.data[itemID][originalHouseID].price;
	}

	buildItemList.insert(iter, BuildItem(itemID, price));
}

void BuilderBase::removeItem(std::list<BuildItem>& buildItemList, std::list<BuildItem>::iterator& iter, Uint32 itemID) {
	if(iter != buildItemList.end()) {
		if(iter->itemID == itemID) {
			std::list<BuildItem>::iterator iter2 = iter;
			++iter;
			buildItemList.erase(iter2);

            // is this item currently produced?
            if(currentProducedItem == itemID) {
                owner->returnCredits(productionProgress);
                productionProgress = 0.0f;
                currentProducedItem = ItemID_Invalid;
            }

            // remove from production list
            std::list<ProductionQueueItem>::iterator iter3 = currentProductionQueue.begin();
            while(iter3 != currentProductionQueue.end()) {
                if(iter3->itemID == itemID) {
                    std::list<ProductionQueueItem>::iterator iter4 = iter3;
                    ++iter3;
                    currentProductionQueue.erase(iter4);
                } else {
                    ++iter3;
                }
            }

            produceNextAvailableItem();
		}
	}
}


void BuilderBase::setOwner(House *no) {
	this->owner = no;
}

bool BuilderBase::isWaitingToPlace() const {
	if((currentProducedItem == ItemID_Invalid) || isUnit(currentProducedItem)) {
		return false;
	}

	const BuildItem* tmp = getBuildItem(currentProducedItem);
	if(tmp == NULL) {
		return false;
	} else {
		return (productionProgress >= tmp->price);
	}
}


void BuilderBase::updateProductionProgress() {
    if(currentProducedItem != ItemID_Invalid) {
        BuildItem* tmp = getBuildItem(currentProducedItem);

        if((productionProgress < tmp->price) && (isOnHold() == false) && (owner->getCredits() > 0)) {

            float oldProgress = productionProgress;

            if(currentGame->getGameInitSettings().getGameOptions().instantBuild == true) {
                float totalBuildCosts = currentGame->objectData.data[currentProducedItem][originalHouseID].price;
                float buildCosts = totalBuildCosts - productionProgress;

                productionProgress += owner->takeCredits(buildCosts);
            } else {

                float buildSpeed = getHealth() / (float) getMaxHealth();
                float totalBuildCosts = currentGame->objectData.data[currentProducedItem][originalHouseID].price;
                float totalBuildGameTicks = currentGame->objectData.data[currentProducedItem][originalHouseID].buildtime*15.0f;
                float buildCosts = totalBuildCosts / totalBuildGameTicks;

                productionProgress += owner->takeCredits(buildCosts*buildSpeed);

                /* That was wrong. Build speed does not depend on power production
                if (getOwner()->hasPower() || (((currentGame->gameType == GAMETYPE_CAMPAIGN) || (currentGame->gameType == GAMETYPE_SKIRMISH)) && getOwner()->isAI())) {
                    //if not enough power, production is halved
                    ProductionProgress += owner->takeCredits(0.25f);
                } else {
                    ProductionProgress += owner->takeCredits(0.125f);
                }*/

            }

            if ((oldProgress == productionProgress) && (owner == pLocalHouse)) {
                currentGame->addToNewsTicker(_("Not enough money"));
            }

            if(productionProgress >= tmp->price) {
                setWaitingToPlace();
            }
        }
	}
}

void BuilderBase::doBuildRandom() {
	int randNum = currentGame->randomGen.rand(0, getBuildListSize()-1);
	int i = 0;
	std::list<BuildItem>::iterator iter;
	for(iter = buildList.begin(); iter != buildList.end(); ++iter) {
		if(i == randNum) {
			doProduceItem(iter->itemID);
			break;
		}
	}
}

void BuilderBase::produceNextAvailableItem() {
	if(currentProductionQueue.empty() == true) {
		currentProducedItem = ItemID_Invalid;
	} else {
		currentProducedItem = currentProductionQueue.front().itemID;
	}

	productionProgress = 0.0f;
	bCurrentItemOnHold = false;
}

int BuilderBase::getMaxUpgradeLevel() const {
    int upgradeLevel = 0;

    for(int i = ItemID_FirstID; i <= ItemID_LastID; i++) {
        ObjectData::ObjectDataStruct& objData = currentGame->objectData.data[i][originalHouseID];

        if((objData.builder == (int) itemID) && (objData.techLevel <= currentGame->techLevel)) {
            upgradeLevel = std::max(upgradeLevel, (int) objData.upgradeLevel);
        }
    }

    return upgradeLevel;
}

void BuilderBase::updateBuildList()
{
    std::list<BuildItem>::iterator iter = buildList.begin();

    for(int i = 0; itemOrder[i] != ItemID_Invalid; i++) {

        int itemID2Add = itemOrder[i];

        ObjectData::ObjectDataStruct& objData = currentGame->objectData.data[itemID2Add][originalHouseID];

        if((objData.builder != (int) itemID) || (objData.upgradeLevel > curUpgradeLev) || (objData.techLevel > currentGame->techLevel)) {
            // first simple checks have rejected this item as being available for built in this builder
            removeItem(buildList, iter, itemID2Add);
        } else {

            // check if prerequisites are met
            bool bPrerequisitesMet = true;
            for(int itemID2Test = Structure_FirstID; itemID2Test <= Structure_LastID; itemID2Test++) {
                if(objData.prerequisiteStructuresSet[itemID2Test] && (owner->getNumItems(itemID2Test) <= 0)) {
                    bPrerequisitesMet = false;
                    break;
                }
            }

            if(bPrerequisitesMet) {
                insertItem(buildList, iter, itemID2Add);
            } else {
                removeItem(buildList, iter, itemID2Add);
            }
        }
    }

}

void BuilderBase::setWaitingToPlace() {
	if (currentProducedItem != ItemID_Invalid)	{
		if (owner == pLocalHouse) {
			soundPlayer->playVoice(ConstructionComplete,getOwner()->getHouseID());
		}

		if (isUnit(currentProducedItem)) {
			//if its a unit
            deployTimer = MILLI2CYCLES(750);
		} else {
			//its a structure
			if (owner == pLocalHouse) {
				currentGame->addToNewsTicker(_("@DUNE.ENG|51#Construction is complete"));
			}
		}
	}
}

void BuilderBase::unSetWaitingToPlace() {
    removeBuiltItemFromProductionQueue();
}

int BuilderBase::getUpgradeCost() const {
    return currentGame->objectData.data[itemID][originalHouseID].price / 2.0f;
}

bool BuilderBase::update() {
	if(StructureBase::update() == false) {
        return false;
	}

	if(isUnit(currentProducedItem) && (productionProgress >= getBuildItem(currentProducedItem)->price)) {
        deployTimer--;
        if(deployTimer == 0) {
            int finishedItemID = currentProducedItem;
            removeBuiltItemFromProductionQueue();

			int num2Place = 1;

			if(finishedItemID == Unit_Infantry) {
                // make three
                finishedItemID = Unit_Soldier;
                num2Place = 3;
            } else if(finishedItemID == Unit_Troopers) {
                // make three
                finishedItemID = Unit_Trooper;
                num2Place = 3;
            }

            for(int i = 0; i < num2Place; i++) {
                UnitBase* newUnit = getOwner()->createUnit(finishedItemID);

                if(newUnit != NULL) {
                    Coord spot = currentGameMap->findDeploySpot(newUnit, location, destination, structureSize);
                    newUnit->deploy(spot);

                    if(getOwner()->isAI()
                        && (newUnit->getItemID() != Unit_Carryall)
                        && (newUnit->getItemID() != Unit_Harvester)
                        && (newUnit->getItemID() != Unit_MCV)) {
                        newUnit->doSetAttackMode(AREAGUARD);
                    }

                    if(destination.isValid()) {
                        newUnit->setGuardPoint(destination);
                        newUnit->setDestination(destination);
                        newUnit->setAngle(lround(8.0f/256.0f*destinationAngle(newUnit->getLocation(), newUnit->getDestination())));
                    }

                    // inform owner of its new unit
                    newUnit->getOwner()->informWasBuilt(finishedItemID);
                }
            }
        }
	}

	if(upgrading == true) {
	    float totalUpgradePrice = getUpgradeCost();

	    if(currentGame->getGameInitSettings().getGameOptions().instantBuild == true) {
	        float upgradePriceLeft = totalUpgradePrice - upgradeProgress;
	        upgradeProgress += owner->takeCredits(upgradePriceLeft);
	    } else {
            float totalUpgradeGameTicks = 30.0f * 100.0f / 5.0f;
            upgradeProgress += owner->takeCredits(totalUpgradePrice / totalUpgradeGameTicks);
	    }

		if(upgradeProgress >= totalUpgradePrice) {
			upgrading = false;
			curUpgradeLev++;
			updateBuildList();

			upgradeProgress = 0.0f;
		}
	} else {
		updateProductionProgress();
	}

	return true;
}

void BuilderBase::removeBuiltItemFromProductionQueue() {
    productionProgress = 0.0f;
	std::list<BuildItem>::iterator iter;
	for(iter = buildList.begin(); iter != buildList.end(); ++iter) {
		if(iter->itemID == currentProducedItem) {
            if(iter->num > 0) {
                iter->num--;
                break;
            }
        }
	}

    deployTimer = 0;
	currentProductionQueue.pop_front();
    produceNextAvailableItem();
}

void BuilderBase::handleUpgradeClick() {
	currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_BUILDER_UPGRADE, objectID));
}

void BuilderBase::handleProduceItemClick(Uint32 itemID, bool multipleMode) {
    std::list<BuildItem>::iterator iter;
	for(iter = buildList.begin(); iter != buildList.end(); ++iter) {
		if(iter->itemID == itemID) {
            if(currentGame->getGameInitSettings().getGameOptions().onlyOnePalace && itemID == Structure_Palace && (iter->num > 0 || owner->getNumItems(Structure_Palace) > 0)) {
                // only one palace allowed
                soundPlayer->playSound(InvalidAction);
                return;
            }
		}
	}

	currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_BUILDER_PRODUCEITEM, objectID, itemID, (Uint32) multipleMode));
}

void BuilderBase::handleCancelItemClick(Uint32 itemID, bool multipleMode) {
	currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_BUILDER_CANCELITEM, objectID, itemID, (Uint32) multipleMode));
}

void BuilderBase::handleSetOnHoldClick(bool OnHold) {
	currentGame->getCommandManager().addCommand(Command(pLocalPlayer->getPlayerID(), CMD_BUILDER_SETONHOLD, objectID, (Uint32) OnHold));
}


bool BuilderBase::doUpgrade() {
    if(upgrading) {
        return false;
    } else if(isAllowedToUpgrade() && (owner->getCredits() >= getUpgradeCost())) {
		upgrading = true;
		upgradeProgress = 0.0f;
		return true;
	} else {
        return false;
	}
}

void BuilderBase::doProduceItem(Uint32 itemID, bool multipleMode) {
	std::list<BuildItem>::iterator iter;
	for(iter = buildList.begin(); iter != buildList.end(); ++iter) {
		if(iter->itemID == itemID) {
			for(int i = 0; i < (multipleMode ? 5 : 1); i++) {
			    if(currentGame->getGameInitSettings().getGameOptions().onlyOnePalace && itemID == Structure_Palace && (iter->num > 0 || owner->getNumItems(Structure_Palace) > 0)) {
			        // only one palace allowed
                    return;
			    }

				iter->num++;
				currentProductionQueue.push_back( ProductionQueueItem(itemID, iter->price) );
				if(currentProducedItem == ItemID_Invalid) {
					productionProgress = 0;
					currentProducedItem = itemID;
				}
			}
			break;
		}
	}
}

void BuilderBase::doCancelItem(Uint32 itemID, bool multipleMode) {
	std::list<BuildItem>::iterator iter;
	for(iter = buildList.begin(); iter != buildList.end(); ++iter) {
		if(iter->itemID == itemID) {
			for(int i = 0; i < (multipleMode ? 5 : 1); i++) {
				if(iter->num > 0) {
					iter->num--;

                    bool cancelCurrentItem = (itemID == currentProducedItem);
					std::list<ProductionQueueItem>::reverse_iterator iter2;
					for(iter2 = currentProductionQueue.rbegin(); iter2 != currentProductionQueue.rend(); ++iter2) {
						if(iter2->itemID == itemID) {
						    if(iter->num == 0 && cancelCurrentItem == true) {
                                owner->returnCredits(productionProgress);
						    } else {
                                cancelCurrentItem = false;
						    }
                            currentProductionQueue.erase((++iter2).base());

							break;
						}
					}

					if(cancelCurrentItem == true) {
					    deployTimer = 0;
						produceNextAvailableItem();
					}
				}
			}
			break;
		}
	}
}


