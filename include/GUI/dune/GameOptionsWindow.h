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

#ifndef GAMEOPTIONSWINDOW_H
#define GAMEOPTIONSWINDOW_H

#include <GUI/Window.h>
#include <GUI/HBox.h>
#include <GUI/VBox.h>
#include <GUI/TextButton.h>
#include <GUI/Label.h>
#include <GUI/Spacer.h>
#include <GUI/Checkbox.h>
#include <GUI/PictureButton.h>
#include <GUI/ProgressBar.h>

#include <DataTypes.h>

#include <algorithm>

class GameOptionsWindow : public Window
{
public:
	GameOptionsWindow(SettingsClass::GameOptionsClass& initialGameOptions);
	virtual ~GameOptionsWindow();

	void onOK();

	const SettingsClass::GameOptionsClass& getGameOptions() const { return gameOptions; };

    /**
		This static method creates a dynamic game options window.
		The idea behind this method is to simply create a new dialog on the fly and
		add it as a child window of some other window. If the window gets closed it will be freed.
		\param	initialGameOptions the game options that will be shown on start of the dialog
		\return	The new dialog box (will be automatically destroyed when it's closed)
	*/
	static GameOptionsWindow* create(SettingsClass::GameOptionsClass& initialGameOptions) {
		GameOptionsWindow* dlg = new GameOptionsWindow(initialGameOptions);
		dlg->pAllocated = true;
		return dlg;
	}

private:
    void onGameSpeedMinus();
    void onGameSpeedPlus();
    void updateGameSpeedBar();

    int currentGameSpeed;
    SettingsClass::GameOptionsClass gameOptions;

	VBox vbox;					                    ///< vertical box
	HBox hbox;					                    ///< horizontal box
	VBox vbox2;					                    ///< inner vertical box;
	Label captionlabel;			                    ///< label that contains the caption
    Checkbox concreteRequiredCheckbox;              ///< If not checked we can build without penalties on the bare rock
	Checkbox structuresDegradeOnConcreteCheckbox;   ///< If checked, structures will degrade on power shortage even when build on concrete
	Checkbox fogOfWarCheckbox;                      ///< If checked explored terrain will become foggy when no unit or structure is next to it
	Checkbox startWithExploredMapCheckbox;          ///< If checked the complete map is unhidden at the beginning of the game
	Checkbox instantBuildCheckbox;                  ///< If checked the building of structures and units does not take any time
	Checkbox onlyOnePalaceCheckbox;                 ///< If checked only one palace can be build per house
	Checkbox rocketTurretsNeedPowerCheckbox;        ///< If checked rocket turrets are dysfunctional on power shortage
	Checkbox sandwormsRespawnCheckbox;              ///< If checked killed sandworms respawn after some time
	Checkbox killedSandwormsDropSpiceCheckbox;      ///< If checked killed sandworms drop some spice
	HBox            gameSpeedHBox;                  ///< The HBox containing the game speed selection
	PictureButton	gameSpeedPlus;                  ///< The button for increasing the game speed
	PictureButton	gameSpeedMinus;                 ///< The button for decreasing the game speed
	TextProgressBar gameSpeedBar;                   ///< The bar showing the current game speed
	TextButton okbutton;                    	    ///< the ok button
};


#endif // GAMEOPTIONSWINDOW_H
