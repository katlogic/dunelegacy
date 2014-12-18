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

#ifndef GUISTYLE_H
#define GUISTYLE_H

#include <FileClasses/FontManager.h>        // For Font IDs

#include <stdlib.h>
#include <stdio.h>

#include <SDL.h>
#include <string>
#include <list>

#include "Widget.h"

typedef enum {
    Alignment_VCenter = 1,
    Alignment_Top = 2,
    Alignment_Bottom = 4,
	Alignment_HCenter = 8,
	Alignment_Left = 16,
	Alignment_Right = 32
} Alignment_Enum;

class GUIStyle {
public:
	/// default constructor
	GUIStyle();

	/// destructor
	virtual ~GUIStyle();

	static void setGUIStyle(GUIStyle* newGUIStyle) {
		currentGUIStyle = newGUIStyle;
	}

	static void destroyGUIStyle() {
		delete currentGUIStyle;
		currentGUIStyle = NULL;
	}

	static GUIStyle& getInstance() {
		if(currentGUIStyle == NULL) {
			fprintf(stderr,"GUIStyle::getInstance(): currentGUIStyle == NULL. Call setGUIStyle before using getInstance()!\n");
		}
		return *currentGUIStyle;
	}




	/**
		Returns the minimum size of a label with this text
		\param	text	The text for the label
        \param  fontID  The ID of the font to use
		\return	the mimimum size of this label
	*/
	virtual Point getMinimumLabelSize(std::string text, int fontID) = 0;

	/**
		Creates the surface for a label with TextLines as content.
		\param	width		    the width of the label
		\param	height		    the height of the label
		\param	TextLines	    a list of text lines for this label
        \param  fontID          the ID of the font to use
		\param	alignment	    the alignment for this label
		\param	textcolor	    the color of the text (-1 = default color for this style)
		\param	textshadowcolor	the color of the shadow under the text (-1 = default color for this style)
		\param  backgroundcolor the background color (default is transparent)
		\return	the new surface (has to be freed by the caller)
	*/
	virtual SDL_Surface* createLabelSurface(Uint32 width, Uint32 height, std::list<std::string> textLines, int fontID, Alignment_Enum alignment = Alignment_HCenter, int textcolor = -1, int textshadowcolor = -1, int backgroundcolor = 0) = 0;




	/**
		Returns the minimum size of a checkbox with this text
		\param	text	The text for the checkbox
		\return	the mimimum size of this checkbox
	*/
	virtual Point getMinimumCheckboxSize(std::string text) = 0;

	/**
		Creates the surface for a checkbox with text as content.
		\param	width		    the width of the checkbox
		\param	height		    the height of the checkbox
		\param	text		    the text for this checkbox
		\param	checked         true, if the checkbox is checked, false otherwise
		\param	activated	    true if the checkbox is activated (e.g. mouse hover)
		\param	textcolor	    the color of the text (-1 = default color for this style)
		\param	textshadowcolor	the color of the shadow under the text (-1 = default color for this style)
		\param  backgroundcolor the background color (default is transparent)
		\return	the new surface (has to be freed by the caller)
	*/
	virtual SDL_Surface* createCheckboxSurface(Uint32 width, Uint32 height, std::string text, bool checked, bool activated, int textcolor = -1, int textshadowcolor = -1, int backgroundcolor = 0) = 0;



	/**
		Returns the minimum size of a radio button with this text
		\param	text	The text for the radio button
		\return	the mimimum size of this radio button
	*/
	virtual Point getMinimumRadioButtonSize(std::string text) = 0;

	/**
		Creates the surface for a radio button with text as content.
		\param	width		    the width of the radio button
		\param	height		    the height of the radio button
		\param	text		    the text for this radio button
		\param	checked         true, if the radio button is checked, false otherwise
		\param	activated	    true if the radio button is activated (e.g. mouse hover)
		\param	textcolor	    the color of the text (-1 = default color for this style)
		\param	textshadowcolor	the color of the shadow under the text (-1 = default color for this style)
		\param  backgroundcolor the background color (default is transparent)
		\return	the new surface (has to be freed by the caller)
	*/
	virtual SDL_Surface* createRadioButtonSurface(Uint32 width, Uint32 height, std::string text, bool checked, bool activated, int textcolor = -1, int textshadowcolor = -1, int backgroundcolor = 0) = 0;



	/**
		Creates the surface for a drop down box
		\param	size		the width and height of the drop down button
		\param	pressed		true if the button should be pressed
		\param	activated	true if the button is activated (e.g. mouse hover)
		\param	color	    the color of the text (-1 = default color for this style)
		\return	the new surface (has to be freed by the caller)
	*/
	virtual SDL_Surface* createDropDownBoxButton(Uint32 size, bool pressed, bool activated, int color = -1) = 0;



	/**
		Returns the minumum size of a button with this text
		\param	text	The text for the button
		\return	the mimimum size of this button
	*/
	virtual Point getMinimumButtonSize(std::string text) = 0;

	/**
		Creates the surface for a button with text as content.
		\param	width		    the width of the button
		\param	height		    the height of the button
		\param	text		    the text for this button
		\param	pressed		    true if the button should be pressed
		\param	activated	    true if the button is activated (e.g. mouse hover)
		\param	textcolor	    the color of the text (-1 = default color for this style)
		\param	textshadowcolor	the color of the shadow under the text (-1 = default color for this style)
		\return	the new surface (has to be freed by the caller)
	*/
	virtual SDL_Surface* createButtonSurface(Uint32 width, Uint32 height, std::string text, bool pressed, bool activated, int textcolor = -1, int textshadowcolor = -1) = 0;




	/**
		Returns the minumum size of a text box
		\param  fontID  The ID of the font to use
		\return	the mimimum size of a text box
	*/
	virtual Point getMinimumTextBoxSize(int fontID) = 0;

	/**
		Creates the surface for a text box with text as content.
		\param	width		    the width of the text box
		\param	height		    the height of the text box
		\param	text		    the text for this text box
		\param	carret		    true if a carret should be shown
		\param  fontID          the ID of the font to use
		\param	alignment	    the alignment for this text box
		\param	textcolor	    the color of the text (-1 = default color for this style)
		\param	textshadowcolor	the color of the shadow under the text (-1 = default color for this style)
		\return	the new surface (has to be freed by the caller)
	*/
	virtual SDL_Surface* createTextBoxSurface(Uint32 width, Uint32 height, std::string text, bool carret, int fontID, Alignment_Enum alignment = Alignment_Left, int textcolor = -1, int textshadowcolor = -1) = 0;




	/**
		Returns the minumum size of a scroll bar arrow button.
		\return	the mimimum size of a scroll bar arrow
	*/
	virtual Point getMinimumScrollBarArrowButtonSize() = 0;

	/**
		Creates the surface for a scroll bar arrow button.
		\param	down		true = downward arrow, false = upward arrow
		\param	pressed		true if the button should be pressed
		\param	activated	true if the button is activated (e.g. mouse hover)
		\param	color	    the color of the text (-1 = default color for this style)
		\return	the new surface (has to be freed by the caller)
	*/
	virtual SDL_Surface* createScrollBarArrowButton(bool down, bool pressed, bool activated, int color = -1) = 0;




	/**
		Returns the minumum height of a list box entry.
		\return	the mimimum height of a list box entry
	*/
	virtual Uint32 getListBoxEntryHeight() = 0;

	/**
		Creates the surface for a list box entry with text as content.
		\param	width		the width of the entry
		\param	text		the text for this entry
		\param	selected	true if a entry should be highlighted
		\param	color	    the color of the text (-1 = default color for this style)
		\return	the new surface (has to be freed by the caller)
	*/
	virtual SDL_Surface* createListBoxEntry(Uint32 width, std::string text, bool selected, int color = -1) = 0;




	/**
		Creates the overlay surface for a progress bar widget. This surface is then drawn
		above the progress bar widget.
		\param	width		the width of the progress bar
		\param	height		the height of the progress bar
		\param	percent		a value between 0.0 and 100.0
		\param	color		the color of the overlay
		\return	the new surface (has to be freed by the caller)
	*/
	virtual SDL_Surface* createProgressBarOverlay(Uint32 width, Uint32 height, double percent, int color = -1) = 0;

	/**
		Creates a tool tip surface.
		\param	text		the tool tip text
		\return	the new surface (has to be freed by the caller)
	*/
	virtual SDL_Surface* createToolTip(std::string text) = 0;

	/**
		Creates a simple background for e.g. a window
		\param	width		the width of the surface
		\param	height		the height of the surface
		\return	the new surface (has to be freed by the caller)
	*/
	virtual SDL_Surface* createBackground(Uint32 width, Uint32 height) = 0;

	/**
		Creates a simple background for widgets
		\param	width		the width of the surface
		\param	height		the height of the surface
		\return	the new surface (has to be freed by the caller)
	*/
	virtual SDL_Surface* createWidgetBackground(Uint32 width, Uint32 height) = 0;

	/**
		Creates an empty surface. This surface is transparent or black.
		\param	width		the width of the surface
		\param	height		the height of the surface
		\param	transparent	true = transparent surface, false = black
		\return	the new surface (has to be freed by the caller)
	*/
	virtual SDL_Surface* createEmptySurface(Uint32 width, Uint32 height, bool transparent);


	/**
		Get the height of the font specified by fontnum
		\param	FontNum		the font
		\return	the height of the font
	*/
	virtual unsigned int getTextHeight(unsigned int FontNum) = 0;

	/**
		Get the weidth of the text with the font specified by fontnum
		\param	text		the text to get the width from
		\param	FontNum		the font
		\return	the width of the text
	*/
	virtual unsigned int getTextWidth(const char* text, unsigned int FontNum) = 0;

private:
	static GUIStyle* currentGUIStyle;
};

#endif //GUISTYLE_H
