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

#ifndef DEFAULTSTRUCTUREINTERFACE_H
#define DEFAULTSTRUCTUREINTERFACE_H

#include "DefaultObjectInterface.h"

#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <FileClasses/TextManager.h>

#include <structures/StructureBase.h>

class DefaultStructureInterface : public DefaultObjectInterface {
public:
	static DefaultStructureInterface* create(int objectID) {
		DefaultStructureInterface* tmp = new DefaultStructureInterface(objectID);
		tmp->pAllocated = true;
		return tmp;
	}

protected:
	DefaultStructureInterface(int objectID) : DefaultObjectInterface(objectID) {
		SDL_Surface* surf, *surfPressed;

		surf = pGFXManager->getUIGraphic(UI_Repair);
		surfPressed = pGFXManager->getUIGraphic(UI_Repair_Pressed);

		repairButton.setSurfaces(surf, false, surfPressed,false);
		repairButton.setToggleButton(true);
		repairButton.setVisible(false);
		repairButton.setTooltipText(_("Repair this structure (Hotkey: R)"));
		repairButton.setOnClick(std::bind(&DefaultStructureInterface::OnRepair, this));

		topBox.addWidget(&repairButton,Point(2,2),Point(surf->w,surf->h));
	}

	void OnRepair() {
		ObjectBase* pObject = currentGame->getObjectManager().getObject(objectID);
		StructureBase* pStructure = dynamic_cast<StructureBase*>(pObject);
		if(pStructure != NULL) {
			pStructure->handleRepairClick();
		}
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

		StructureBase* pStructure = dynamic_cast<StructureBase*>(pObject);
		if(pStructure != NULL) {
			if(pStructure->getHealth() >= pStructure->getMaxHealth()) {
				repairButton.setVisible(false);
			} else {
				repairButton.setVisible(true);
				repairButton.setToggleState(pStructure->isRepairing());
			}
		}

		return true;
	}

	PictureButton	repairButton;
};

#endif // DEFAULTSTRUCTUREINTERFACE_H
