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

#ifndef DEFAULTOBJECTINTERFACE_H
#define DEFAULTOBJECTINTERFACE_H

#include "ObjectInterface.h"

#include <GUI/PictureLabel.h>

#include <globals.h>

#include <Game.h>

#include <sand.h>

#include <ObjectBase.h>
#include <House.h>
#include <units/UnitBase.h>


class DefaultObjectInterface : public ObjectInterface {
public:
	static DefaultObjectInterface* create(int objectID) {
		DefaultObjectInterface* tmp = new DefaultObjectInterface(objectID);
		tmp->pAllocated = true;
		return tmp;
	}

protected:
	DefaultObjectInterface(int objectID) : ObjectInterface() {
		ObjectBase* pObject = currentGame->getObjectManager().getObject(objectID);
		if(pObject == NULL) {
			fprintf(stderr,"DefaultObjectInterface::DefaultObjectInterface(): Cannot resolve ObjectID %d!\n",objectID);
			exit(EXIT_FAILURE);
		}

		this->objectID = objectID;
		itemID = pObject->getItemID();

		addWidget(&topBox,Point(0,0),Point(SIDEBARWIDTH - 25,80));

		addWidget(&mainHBox,Point(0,80),Point(SIDEBARWIDTH - 25,screen->h - 80 - 148));

		topBox.addWidget(&topBoxHBox,Point(0,22),Point(SIDEBARWIDTH - 25,58));

		topBoxHBox.addWidget(Spacer::create());
		topBoxHBox.addWidget(&objPicture);

		objPicture.setSurface(resolveItemPicture(itemID, (HOUSETYPE) pObject->getOriginalHouseID()),false);

		topBoxHBox.addWidget(Spacer::create());
	};

	virtual ~DefaultObjectInterface() { ; };

	/**
		This method updates the object interface.
		If the object doesn't exists anymore then update returns false.
		\return true = everything ok, false = the object container should be removed
	*/
	virtual bool update() {
		ObjectBase* pObject = currentGame->getObjectManager().getObject(objectID);
		return (pObject != NULL);
	}

	int				objectID;
	int 			itemID;

	StaticContainer	topBox;
	HBox			topBoxHBox;
	HBox			mainHBox;
	PictureLabel	objPicture;
};

#endif // DEFAULTOBJECTINTERFACE_H
