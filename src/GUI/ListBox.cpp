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

#include <GUI/ListBox.h>

ListBox::ListBox() : Widget() {
	enableResizing(true,true);

	pBackground = NULL;
	pForeground = NULL;
	color = -1;
	bAutohideScrollbar = true;
	bHighlightSelectedElement = true;
	firstVisibleElement = 0;
	selectedElement = -1;
	lastClickTime = 0;

	scrollbar.setOnChange(std::bind(&ListBox::onScrollbarChange, this));
	resize(getMinimumSize().x,getMinimumSize().y);
}

ListBox::~ListBox() {
	if(pBackground != NULL) {
		SDL_FreeSurface(pBackground);
	}

	if(pForeground != NULL) {
		SDL_FreeSurface(pForeground);
	}
}

void ListBox::handleMouseMovement(Sint32 x, Sint32 y, bool insideOverlay) {
	scrollbar.handleMouseMovement(x - getSize().x + scrollbar.getSize().x,y,insideOverlay);
}

bool ListBox::handleMouseLeft(Sint32 x, Sint32 y, bool pressed) {
    int scrollbarWidth = isScrollbarVisible() ? scrollbar.getSize().x : 0;

	if((x>=0) && (x < getSize().x - scrollbarWidth)
		&& (y>=0) && (y < getSize().y)) {

		if(pressed == true) {
			int index = ((y - 1) / GUIStyle::getInstance().getListBoxEntryHeight()) + firstVisibleElement;
			if((index >= 0) && (index < getNumEntries())) {
				selectedElement = index;

				if(SDL_GetTicks() - lastClickTime < 200) {
					if(pOnDoubleClick) {
						pOnDoubleClick();
					}
				} else {
					lastClickTime = SDL_GetTicks();
					updateList();
					if(pOnSingleClick) {
                        pOnSingleClick();
					}
					if(pOnSelectionChange) {
						pOnSelectionChange(true);
					}
					setActive();
				}
			}
		}

		scrollbar.handleMouseLeft(x - getSize().x + scrollbar.getSize().x,y,pressed);
		return true;
	} else {
		return scrollbar.handleMouseLeft(x - getSize().x + scrollbar.getSize().x,y,pressed);
	}
}

bool ListBox::handleMouseWheel(Sint32 x, Sint32 y, bool up)  {
	// forward mouse wheel event to scrollbar
	return scrollbar.handleMouseWheel(0,0,up);
}

bool ListBox::handleKeyPress(SDL_KeyboardEvent& key) {
	Widget::handleKeyPress(key);
	if(isActive()) {
		switch(key.keysym.sym) {
			case SDLK_UP: {
				if(selectedElement > 0) {
					setSelectedItem(selectedElement-1, true);
				}
			} break;

			case SDLK_DOWN: {
				if(selectedElement < getNumEntries()-1 ) {
					setSelectedItem(selectedElement+1, true);
				}
			} break;

			default: {
			} break;
		}

		scrollbar.handleKeyPress(key);
	}
	scrollbar.handleKeyPress(key);
	return true;
}

void ListBox::draw(SDL_Surface* screen, Point position) {
	if(isVisible() == false) {
		return;
	}

	if(pBackground != NULL) {
		SDL_Rect dest = { position.x, position.y, pBackground->w, pBackground->h };
		SDL_BlitSurface(pBackground,NULL,screen,&dest);
	}

	SDL_Rect dest = { position.x + 2, position.y + 1, pForeground->w, pForeground->h };
	SDL_BlitSurface(pForeground,NULL,screen,&dest);

	Point ScrollBarPos = position;
	ScrollBarPos.x += getSize().x - scrollbar.getSize().x;

    if(isScrollbarVisible()) {
        scrollbar.draw(screen,ScrollBarPos);
    }
}

void ListBox::resize(Uint32 width, Uint32 height) {
	Widget::resize(width,height);

	if(pBackground != NULL) {
		SDL_FreeSurface(pBackground);
	}

	pBackground = GUIStyle::getInstance().createWidgetBackground(width, height);

	scrollbar.resize(scrollbar.getMinimumSize().x,height);

	updateList();
}

void ListBox::setActive() {
	Widget::setActive();

	if((selectedElement == -1) && (getNumEntries() > 0)) {
		selectedElement = 0;
		updateList();
		if(pOnSelectionChange) {
            pOnSelectionChange(false);
        }
	}
}

void ListBox::setSelectedItem(int index, bool bInteractive) {
    bool bChanged = (index != selectedElement);

	if(index <= -1) {
		selectedElement = -1;
		updateList();
	} else if((index >= 0) && (index < getNumEntries())) {
		selectedElement = index;

		int numVisibleElements = ((getSize().y - 2) / GUIStyle::getInstance().getListBoxEntryHeight()) + 1;

		if(selectedElement >= firstVisibleElement+numVisibleElements - 1) {
			firstVisibleElement = selectedElement-(numVisibleElements-1) + 1;
		} else if(selectedElement < firstVisibleElement) {
			firstVisibleElement = selectedElement;
		}

		if(firstVisibleElement > getNumEntries() - numVisibleElements) {
			firstVisibleElement = std::max(0,getNumEntries() - numVisibleElements + 1);
		}

		scrollbar.setCurrentValue(firstVisibleElement);

		updateList();
	}

	if(bChanged && pOnSelectionChange) {
        pOnSelectionChange(bInteractive);
	}
}

void ListBox::updateList() {
	if(pForeground != NULL) {
		SDL_FreeSurface(pForeground);
		pForeground = NULL;
	}

	// create surfaces
	int surfaceHeight = getSize().y - 2;
	if(surfaceHeight < 0) {
        surfaceHeight = 0;
	}
	pForeground = GUIStyle::getInstance().createEmptySurface(getSize().x - 4, surfaceHeight,true);

	int numVisibleElements = surfaceHeight / GUIStyle::getInstance().getListBoxEntryHeight();
	for(int i = firstVisibleElement; i < firstVisibleElement + numVisibleElements; i++) {
		if(i >= getNumEntries()) {
			break;
		}

		SDL_Surface* pSurface = GUIStyle::getInstance().createListBoxEntry(getSize().x - 4, getEntry(i), bHighlightSelectedElement && (i==selectedElement), color);

		SDL_Rect dest = {   0,
                            (i-firstVisibleElement) * GUIStyle::getInstance().getListBoxEntryHeight(),
                            pSurface->w,
                            pSurface->h };
		SDL_BlitSurface(pSurface,NULL,pForeground,&dest);
		SDL_FreeSurface(pSurface);
	}

	scrollbar.setRange(0,std::max(0,getNumEntries() - numVisibleElements));
	scrollbar.setBigStepSize(std::max(1, numVisibleElements-1));
}
