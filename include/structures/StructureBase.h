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

#ifndef STRUCTUREBASE_H
#define STRUCTUREBASE_H

#define ANIMATIONTIMER 25
#define ANIMMOVETIMER 50

#include <ObjectBase.h>

#include <list>

struct StructureSmoke {
    StructureSmoke(const Coord& pos, Uint32 gameCycle)
     : realPos(pos), startGameCycle(gameCycle) {
    };

    StructureSmoke(InputStream& stream) {
        realPos.x = stream.readSint32();
        realPos.y = stream.readSint32();
        startGameCycle = stream.readUint32();
    };

    void save(OutputStream& stream) const {
        stream.writeSint32(realPos.x);
        stream.writeSint32(realPos.y);
        stream.writeUint32(startGameCycle);
    };

    Coord   realPos;
    Uint32  startGameCycle;
};

class StructureBase : public ObjectBase
{
public:
	StructureBase(House* newOwner);
	StructureBase(InputStream& stream);
	void init();
	virtual ~StructureBase();

	virtual void save(OutputStream& stream) const;

	void assignToMap(const Coord& pos);
	virtual void blitToScreen();

	virtual ObjectInterface* getInterfaceContainer();

	void destroy();
	virtual void drawSelectionBox();
	virtual void drawOtherPlayerSelectionBox();

	virtual Coord getCenterPoint() const;
	virtual Coord getClosestCenterPoint(const Coord& objectLocation) const;
	void setDestination(int newX, int newY);
	void setJustPlaced();
	void setFogged(bool bFogged) { fogged = bFogged; };

	void playConfirmSound() { ; };
	void playSelectSound() { ; };

	/**
		This method is called when a structure is ordered by a right click
		\param	xPos	the x position on the map
		\param	yPos	the y position on the map
	*/
	virtual void handleActionClick(int xPos, int yPos);

	/**
		This method is called when the user clicks on the repair button for this building
	*/
	virtual void handleRepairClick();

    /**
        Set the deploy position of this structure. Units produced in this structure will move directly to
        this position x,y after being deployed.
        \param  x           the x coordinate (in tile coordinates)
        \param  y           the y coordinate (in tile coordinates)
    */
	virtual void doSetDeployPosition(int xPos, int yPos);

    /**
        Start repairing this structure.
    */
	void doRepair();

	/**
        Updates this object.
        \return true if this object still exists, false if it was destroyed
	*/
	virtual bool update();

	/**
        Can this structure be captured by infantry units?
        \return true, if this structure can be captured, false otherwise
	*/
	virtual bool canBeCaptured() const { return true; };

	bool isRepairing() const { return repairing; }

	virtual Coord getClosestPoint(const Coord& objectLocation) const;

	inline short getStructureSizeX() const { return structureSize.x; }
	inline short getStructureSizeY() const { return structureSize.y; }
	inline const Coord& getStructureSize() const { return structureSize; }

	inline void addSmoke(const Coord& pos, Uint32 gameCycle) {
	    std::list<StructureSmoke>::iterator iter;
	    for(iter = smoke.begin(); iter != smoke.end(); ++iter) {
            if(iter->realPos == pos) {
                iter->startGameCycle = gameCycle;
                return;
            } else if(iter->realPos.y > pos.y) {
                smoke.insert(iter, StructureSmoke(pos, gameCycle));
            }
        }

        smoke.push_back(StructureSmoke(pos, gameCycle));
    };
	inline size_t getNumSmoke() const { return smoke.size(); };

protected:
    /**
        Used for updating things that are specific to that particular structure. Is called from
        StructureBase::update() before the check if this structure is still alive.
    */
	virtual void updateStructureSpecificStuff() { };


	// constant for all structures of the same type
    Coord	structureSize;      ///< The size of this structure in tile coordinates (e.g. (3,2) for a refinery)

    // structure state
	bool    repairing;          ///< currently repairing?
    int     degradeTimer;       ///< after which time of insufficient power should we degrade this building again

    // TODO: fogging is currently broken
	bool        fogged;             ///< Currently fogged?
	int         lastVisibleFrame;   ///< store picture drawn before fogged

    // drawing information
    int		justPlacedTimer;          ///< When the structure is justed placed, we draw some special graphic
    std::list<StructureSmoke> smoke;  ///< A vector containing all the smoke for this structure

	int		firstAnimFrame;     ///< First frame of the current animation
	int		lastAnimFrame;      ///< Last frame of the current animation
	int		curAnimFrame;       ///< The current frame of the current animation
	int     animationCounter;   ///< When to show the next animation frame?
};

#endif //STRUCTUREBASE_H
