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

#include <units/Saboteur.h>

#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <House.h>
#include <Map.h>
#include <Game.h>
#include <ScreenBorder.h>
#include <Explosion.h>
#include <SoundPlayer.h>


Saboteur::Saboteur(House* newOwner) : InfantryBase(newOwner)
{
    Saboteur::init();

    setHealth(getMaxHealth());

	setVisible(VIS_ALL, false);
	setVisible(getOwner()->getTeam(), true);
    attackMode = GUARD;
}

Saboteur::Saboteur(InputStream& stream) : InfantryBase(stream)
{
    Saboteur::init();
}

void Saboteur::init()
{
	itemID = Unit_Saboteur;
	owner->incrementUnits(itemID);

	graphicID = ObjPic_Saboteur;
	graphic = pGFXManager->getObjPic(graphicID,getOwner()->getHouseID());

	numImagesX = 4;
	numImagesY = 3;

	numWeapons = 0;
}

Saboteur::~Saboteur()
{
}


void Saboteur::checkPos()
{
	bool	canBeSeen[NUM_HOUSES+1];

	InfantryBase::checkPos();
	if(active) {
		for(int i = 1; i <= NUM_HOUSES; i++) {
			canBeSeen[i] = false;
		}

		for(int x = location.x - 2; (x <= location.x + 2); x++) {
			for(int y = location.y - 2; (y <= location.y + 2); y++) {
				if(currentGameMap->tileExists(x, y) && currentGameMap->getTile(x, y)->hasAnObject()) {
					canBeSeen[currentGameMap->getTile(x, y)->getObject()->getOwner()->getTeam()] = true;
				}
			}
		}

		for(int i = 1; i <= NUM_HOUSES; i++) {
			setVisible(i, canBeSeen[i]);
		}

		setVisible(getOwner()->getTeam(), true);	//owner team can always see it
		//setVisible(pLocalHouse->getTeam(), true);
	}
}

bool Saboteur::update() {
	if(active) {
		if(!moving) {
			//check to see if close enough to blow up target
			if(target.getObjPointer() != NULL && target.getObjPointer()->isAStructure()
				&& (getOwner()->getTeam() != target.getObjPointer()->getOwner()->getTeam()))
			{
				Coord	closestPoint;
				closestPoint = target.getObjPointer()->getClosestPoint(location);


				if(blockDistance(location, closestPoint) <= 1.5f)	{
					if(isVisible(getOwner()->getTeam())) {
                        screenborder->shakeScreen(18);
					}

				    ObjectBase* pObject = target.getObjPointer();
				    destroy();
                    pObject->setHealth(0.0f);
					pObject->destroy();
					return false;
				}
			}
		}
	}

	return InfantryBase::update();
}

void Saboteur::deploy(const Coord& newLocation) {
	UnitBase::deploy(newLocation);

	setVisible(VIS_ALL, false);
	setVisible(getOwner()->getTeam(), true);
}


bool Saboteur::canAttack(const ObjectBase* object) const {
	if ((object != NULL) && object->isAStructure()
		&& (object->getOwner()->getTeam() != owner->getTeam())
		&& object->isVisible(getOwner()->getTeam()))
		return true;
	else
		return false;
}

void Saboteur::destroy()
{
    Coord realPos(lround(realX), lround(realY));
    Uint32 explosionID = currentGame->randomGen.getRandOf(2,Explosion_Medium1, Explosion_Medium2);
    currentGame->getExplosionList().push_back(new Explosion(explosionID, realPos, owner->getHouseID()));

    if(isVisible(getOwner()->getTeam())) {
        soundPlayer->playSoundAt(Sound_ExplosionLarge,location);
    }

    InfantryBase::destroy();
}
