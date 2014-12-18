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

#ifndef MAPEDITORRADARVIEW_H
#define MAPEDITORRADARVIEW_H

#include <RadarViewBase.h>

#include <DataTypes.h>

#include <SDL.h>

class MapEditor;

/// This class manages the mini map at the top right corner of the screen
class MapEditorRadarView : public RadarViewBase
{
public:
	/**
        Constructor
	*/
	MapEditorRadarView(MapEditor* pMapEditor);

	/**
        Destructor
	*/
    virtual ~MapEditorRadarView();

    /**
        Get the map size in x direction
        \return map width
    */
    virtual int getMapSizeX() const;

    /**
        Get the map size in y direction
        \return map height
    */
    virtual int getMapSizeY() const;

	/**
		Draws the radar to screen. This method is called before drawOverlay().
		\param	screen	Surface to draw on
		\param	Position	Position to draw the radar to
	*/
	virtual void draw(SDL_Surface* screen, Point position);

private:
    MapEditor* pMapEditor;
};

#endif // RADARVIEW_H
