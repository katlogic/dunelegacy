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

#ifndef SANDWORM_H
#define SANDWORM_H

#define SANDWORM_LENGTH 50
#define SANDWORM_SEGMENTS 5	//how many shimmer things will be drawn per worm

#include <units/GroundUnit.h>

class Sandworm : public GroundUnit
{
public:
	Sandworm(House* newOwner);
	Sandworm(InputStream& stream);
	void init();
	virtual ~Sandworm();

	virtual void save(OutputStream& stream) const;

	virtual void deploy(const Coord& newLocation);

	void assignToMap(const Coord& pos);
	void attack();
	void blitToScreen();
	void checkPos();

    inline void setLocation(const Coord& location) { setLocation(location.x, location.y); }
	void setLocation(int xPos, int yPos);

    /**
        Updates this sandworm.
        \return true if this object still exists, false if it was destroyed
	*/
	virtual bool update();

	bool canAttack(const ObjectBase* object) const;
	bool canPass(int xPos, int yPos) const;
	inline int getSleepTimer() const { return sleepTimer; }

	virtual int getCurrentAttackAngle() const;

	void playAttackSound();

	virtual float getTerrainDifficulty(TERRAINTYPE terrainType) const {
	    switch(terrainType) {
            case Terrain_Slab:          return 1.0f;
            case Terrain_Sand:          return 1.25f;
            case Terrain_Rock:          return 1.0f;
            case Terrain_Dunes:         return 1.25f;
            case Terrain_Mountain:      return 1.0f;
            case Terrain_Spice:         return 1.25f;
            case Terrain_ThickSpice:    return 1.25f;
            case Terrain_SpiceBloom:    return 1.25f;
            case Terrain_SpecialBloom:  return 1.25f;
            default:                    return 1.0f;
	    }
    }

    bool isEating() const { return (drawnFrame != INVALID); }

protected:
	const ObjectBase* findTarget() const;
	void engageTarget();
	void sleep();
	bool sleepOrDie();

private:
    // sandworm state
	Sint32      kills;              ///< How many units does this sandworm alreay killed?
    Sint32      attackFrameTimer;   ///< When to show the next attack frame
    Sint32      sleepTimer;         ///< How long has this sandworm slept?

    // drawing information
	Coord lastLocs[SANDWORM_LENGTH];                                    ///< Last locations of the sandworm
	SDL_Surface* shimmerSurface[SANDWORM_SEGMENTS][NUM_ZOOMLEVEL];      ///< Surfaces for doing the sandworm animation
};

#endif // SANDWORM_H
