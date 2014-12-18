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

#ifndef WINDTRAPINTERFACE_H
#define WINDTRAPINTERFACE_H

#include "DefaultStructureInterface.h"

#include <FileClasses/FontManager.h>
#include <FileClasses/TextManager.h>

#include <House.h>

#include <GUI/Label.h>
#include <GUI/VBox.h>

#include <misc/string_util.h>

class WindTrapInterface : public DefaultStructureInterface {
public:
	static WindTrapInterface* create(int objectID) {
		WindTrapInterface* tmp = new WindTrapInterface(objectID);
		tmp->pAllocated = true;
		return tmp;
	}

protected:
	WindTrapInterface(int objectID) : DefaultStructureInterface(objectID) {
        int color = houseColor[pLocalHouse->getHouseID()];

		mainHBox.addWidget(&textVBox);

        requiredEnergyLabel.setTextFont(FONT_STD10);
        requiredEnergyLabel.setTextColor(color+3);
		textVBox.addWidget(&requiredEnergyLabel, 0.005);
		producedEnergyLabel.setTextFont(FONT_STD10);
		producedEnergyLabel.setTextColor(color+3);
		textVBox.addWidget(&producedEnergyLabel, 0.005);
		textVBox.addWidget(Spacer::create(),0.99);
	}

	/**
		This method updates the object interface.
		If the object doesn't exists anymore then update returns false.
		\return true = everything ok, false = the object container should be removed
	*/
	virtual bool update() {
		ObjectBase* pObject = currentGame->getObjectManager().getObject(objectID);
		if(pObject == NULL) {
			return false;
		}

		House* pOwner = pObject->getOwner();

        requiredEnergyLabel.setText(" " + _("Required") + ": " + stringify(pOwner->getPowerRequirement()));
        producedEnergyLabel.setText(" " + _("Produced") + ": " + stringify(pOwner->getProducedPower()));

		return DefaultStructureInterface::update();
	}

private:
    VBox    textVBox;

	Label   requiredEnergyLabel;
	Label   producedEnergyLabel;
};

#endif // WINDTRAPINTERFACE_H
