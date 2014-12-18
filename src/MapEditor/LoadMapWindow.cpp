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

#include <MapEditor/LoadMapWindow.h>

#include <FileClasses/GFXManager.h>
#include <FileClasses/TextManager.h>
#include <FileClasses/INIFile.h>

#include <GUI/Spacer.h>
#include <GUI/GUIStyle.h>
#include <GUI/QstBox.h>

#include <config.h>
#include <misc/fnkdat.h>
#include <misc/FileSystem.h>
#include <misc/draw_util.h>
#include <misc/string_util.h>

#include <INIMap/INIMapPreviewCreator.h>

#include <globals.h>


LoadMapWindow::LoadMapWindow(int color) : Window(0,0,0,0), color(color) {

    // set up window
	SDL_Surface *surf;
	surf = pGFXManager->getUIGraphic(UI_NewMapWindow);

	setBackground(surf,false);

	int xpos = std::max(0,(screen->w - surf->w)/2);
	int ypos = std::max(0,(screen->h - surf->h)/2);

	setCurrentPosition(xpos,ypos,surf->w,surf->h);

	setWindowWidget(&mainHBox);

	mainHBox.addWidget(HSpacer::create(16));
	mainHBox.addWidget(&mainVBox);
	mainHBox.addWidget(HSpacer::create(16));

    titleLabel.setTextColor(110, COLOR_TRANSPARENT);
	titleLabel.setAlignment((Alignment_Enum) (Alignment_HCenter | Alignment_VCenter));
	titleLabel.setText(_("Load Map"));
	mainVBox.addWidget(&titleLabel);

    mainVBox.addWidget(VSpacer::create(22));

    mainVBox.addWidget(&centralHBox, 346);


    centralHBox.addWidget(&leftVBox, 0.8);

    leftVBox.addWidget(&mapTypeButtonsHBox, 24);

    singleplayerUserMapsButton.setText(_("SP User Maps"));
    singleplayerUserMapsButton.setTextColor(color);
    singleplayerUserMapsButton.setToggleButton(true);
    singleplayerUserMapsButton.setOnClick(std::bind(&LoadMapWindow::onMapTypeChange, this, 0));
    mapTypeButtonsHBox.addWidget(&singleplayerUserMapsButton);

    multiplayerUserMapsButton.setText(_("MP User Maps"));
    multiplayerUserMapsButton.setTextColor(color);
    multiplayerUserMapsButton.setToggleButton(true);
    multiplayerUserMapsButton.setOnClick(std::bind(&LoadMapWindow::onMapTypeChange, this, 1));
    mapTypeButtonsHBox.addWidget(&multiplayerUserMapsButton);

    mapTypeButtonsHBox.addWidget(Spacer::create(), 5.0);
    mapList.setColor(color);
    mapList.setAutohideScrollbar(false);
    mapList.setOnSelectionChange(std::bind(&LoadMapWindow::onMapListSelectionChange, this, std::placeholders::_1));
    mapList.setOnDoubleClick(std::bind(&LoadMapWindow::onLoad, this));
    leftVBox.addWidget(&mapList, 0.95);

    leftVBox.addWidget(VSpacer::create(10));

    centralHBox.addWidget(HSpacer::create(8));
    centralHBox.addWidget(Spacer::create(), 0.05);

    centralHBox.addWidget(&rightVBox, 180);
    minimap.setSurface( GUIStyle::getInstance().createButtonSurface(130,130,_("Choose map"), true, false), true);
    rightVBox.addWidget(&minimap);

    rightVBox.addWidget(VSpacer::create(10));
    rightVBox.addWidget(&mapPropertiesHBox, 0.01);
    mapPropertiesHBox.addWidget(&mapPropertyNamesVBox, 75);
    mapPropertiesHBox.addWidget(&mapPropertyValuesVBox, 105);
    mapPropertyNamesVBox.addWidget(Label::create(_("Size") + ":", color));
    mapPropertySize.setTextColor(color);
    mapPropertyValuesVBox.addWidget(&mapPropertySize);
    mapPropertyNamesVBox.addWidget(Label::create(_("Players") + ":", color));
    mapPropertyPlayers.setTextColor(color);
    mapPropertyValuesVBox.addWidget(&mapPropertyPlayers);
    mapPropertyNamesVBox.addWidget(Label::create(_("Author") + ":", color));
    mapPropertyAuthors.setTextColor(color);
    mapPropertyValuesVBox.addWidget(&mapPropertyAuthors);
    mapPropertyNamesVBox.addWidget(Label::create(_("License") + ":", color));
    mapPropertyLicense.setTextColor(color);
    mapPropertyValuesVBox.addWidget(&mapPropertyLicense);
    rightVBox.addWidget(Spacer::create());

    mainVBox.addWidget(VSpacer::create(5));

    mainVBox.addWidget(&buttonHBox);

	cancelButton.setText(_("Cancel"));
    cancelButton.setTextColor(color);
	cancelButton.setOnClick(std::bind(&LoadMapWindow::onCancel, this));

	buttonHBox.addWidget(&cancelButton);

	buttonHBox.addWidget(HSpacer::create(8));

    buttonHBox.addWidget(Spacer::create());

	buttonHBox.addWidget(HSpacer::create(8));

    loadButton.setText(_("Load"));
	loadButton.setTextColor(color);
	loadButton.setOnClick(std::bind(&LoadMapWindow::onLoad, this));

	buttonHBox.addWidget(&loadButton);

	mainVBox.addWidget(VSpacer::create(10));

	onMapTypeChange(0);
}

bool LoadMapWindow::handleKeyPress(SDL_KeyboardEvent& key) {
	if(pChildWindow != NULL) {
		bool ret = pChildWindow->handleKeyPress(key);
		return ret;
	}

	if(isEnabled() && (pWindowWidget != NULL)) {
		if(key.keysym.sym == SDLK_RETURN) {
			onLoad();
			return true;
		} else if(key.keysym.sym == SDLK_DELETE) {
		    int index = mapList.getSelectedIndex();
            if(index >= 0) {
                QstBox* pQstBox = QstBox::create(   strprintf(_("Do you really want to delete '%s' ?"), mapList.getEntry(index).c_str()),
                                                    _("No"),
                                                    _("Yes"),
                                                    QSTBOX_BUTTON2);

                pQstBox->setTextColor(color);

                openWindow(pQstBox);
            }

            return true;
		} else {
			return pWindowWidget->handleKeyPress(key);
		}
	} else {
		return false;
	}
}


void LoadMapWindow::onChildWindowClose(Window* pChildWindow) {
    QstBox* pQstBox = dynamic_cast<QstBox*>(pChildWindow);
    if(pQstBox != NULL) {
        if(pQstBox->getPressedButtonID() == QSTBOX_BUTTON2) {
            int index = mapList.getSelectedIndex();
            if(index >= 0) {
                std::string file2delete = currentMapDirectory + mapList.getSelectedEntry() + ".ini";

                if(remove(file2delete.c_str()) == 0) {
                    // remove was successful => delete from list
                    mapList.removeEntry(index);
                    if(mapList.getNumEntries() > 0) {
                        if(index >= mapList.getNumEntries()) {
                            mapList.setSelectedItem(mapList.getNumEntries() - 1);
                        } else {
                            mapList.setSelectedItem(index);
                        }
                    }
                }
            }
        }
    }
}

void LoadMapWindow::onCancel() {
	Window* pParentWindow = dynamic_cast<Window*>(getParent());
	if(pParentWindow != NULL) {
		pParentWindow->closeChildWindow();
	}
}

void LoadMapWindow::onLoad() {
    if(mapList.getSelectedIndex() < 0) {
        return;
    }

    loadMapname = mapList.getSelectedEntry();
    loadMapFilepath = currentMapDirectory + loadMapname + ".ini";
    loadMapSingleplayer = singleplayerUserMapsButton.getToggleState();
    getCaseInsensitiveFilename(loadMapFilepath);

	Window* pParentWindow = dynamic_cast<Window*>(getParent());
	if(pParentWindow != NULL) {
		pParentWindow->closeChildWindow();
	}
}

void LoadMapWindow::onMapTypeChange(int buttonID)
{
    singleplayerUserMapsButton.setToggleState(buttonID == 0);
    multiplayerUserMapsButton.setToggleState(buttonID == 1);

    switch(buttonID) {
        case 0: {
            char tmp[FILENAME_MAX];
            fnkdat("maps/singleplayer/", tmp, FILENAME_MAX, FNKDAT_USER | FNKDAT_CREAT);
            currentMapDirectory = tmp;
        } break;
        case 1: {
            char tmp[FILENAME_MAX];
            fnkdat("maps/multiplayer/", tmp, FILENAME_MAX, FNKDAT_USER | FNKDAT_CREAT);
            currentMapDirectory = tmp;
        } break;
    }

    mapList.clearAllEntries();

    std::list<std::string> filesList = getFileNamesList(currentMapDirectory, "ini", true, FileListOrder_Name_CaseInsensitive_Asc);

    std::list<std::string>::iterator iter;
    for(iter = filesList.begin(); iter != filesList.end(); ++iter) {
        mapList.addEntry(iter->substr(0, iter->length() - 4));
    }

    if(filesList.empty() == false) {
        mapList.setSelectedItem(0);
    } else {
        minimap.setSurface( GUIStyle::getInstance().createButtonSurface(130,130,_("No map available"), true, false), true);
        mapPropertySize.setText("");
        mapPropertyPlayers.setText("");
        mapPropertyAuthors.setText("");
        mapPropertyLicense.setText("");
    }
}

void LoadMapWindow::onMapListSelectionChange(bool bInteractive)
{
    loadButton.setEnabled(true);

    if(mapList.getSelectedIndex() < 0) {
        return;
    }

    std::string mapFilename = currentMapDirectory + mapList.getSelectedEntry() + ".ini";
    getCaseInsensitiveFilename(mapFilename);

    std::shared_ptr<INIFile> map(new INIFile(mapFilename));

    int sizeX = 0;
    int sizeY = 0;

    if(map->hasKey("MAP","Seed")) {
        // old map format with seed value
        int mapscale = map->getIntValue("BASIC", "MapScale", -1);

        switch(mapscale) {
            case 0: {
                sizeX = 62;
                sizeY = 62;
            } break;

            case 1: {
                sizeX = 32;
                sizeY = 32;
            } break;

            case 2: {
                sizeX = 21;
                sizeY = 21;
            } break;

            default: {
                sizeX = 64;
                sizeY = 64;
            }
        }
    } else {
        // new map format with saved map
        sizeX = map->getIntValue("MAP","SizeX", 0);
        sizeY = map->getIntValue("MAP","SizeY", 0);
    }

    mapPropertySize.setText(stringify(sizeX) + " x " + stringify(sizeY));

    SDL_Surface* pMapSurface = GUIStyle::getInstance().createButtonSurface(130,130,"", true, false);

    try {
        INIMapPreviewCreator mapPreviewCreator(map);

        SDL_Surface* pMinimap = mapPreviewCreator.createMinimapImageOfMap();
        SDL_Rect dest = { 1, 1, pMinimap->w, pMinimap->h};
        SDL_BlitSurface(pMinimap, NULL, pMapSurface, &dest);
        SDL_FreeSurface(pMinimap);
    } catch(...) {
        SDL_FreeSurface(pMapSurface);
        pMapSurface = GUIStyle::getInstance().createButtonSurface(130,130,"Error", true, false);
        loadButton.setEnabled(false);
    }

    minimap.setSurface(pMapSurface, true);

    int numPlayers = 0;
    if(map->hasSection("Atreides")) numPlayers++;
    if(map->hasSection("Ordos")) numPlayers++;
    if(map->hasSection("Harkonnen")) numPlayers++;
    if(map->hasSection("Fremen")) numPlayers++;
    if(map->hasSection("Mercenary")) numPlayers++;
    if(map->hasSection("Sardaukar")) numPlayers++;
    if(map->hasSection("Player1")) numPlayers++;
    if(map->hasSection("Player2")) numPlayers++;
    if(map->hasSection("Player3")) numPlayers++;
    if(map->hasSection("Player4")) numPlayers++;
    if(map->hasSection("Player5")) numPlayers++;
    if(map->hasSection("Player6")) numPlayers++;

    mapPropertyPlayers.setText(stringify(numPlayers));



    std::string authors = map->getStringValue("BASIC","Author", "-");
    if(authors.size() > 11) {
        authors = authors.substr(0,9) + "...";
    }
    mapPropertyAuthors.setText(authors);


    mapPropertyLicense.setText(map->getStringValue("BASIC","License", "-"));

}
