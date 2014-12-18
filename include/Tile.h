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

#ifndef TILE_H
#define TILE_H

#include <DataTypes.h>
#include <mmath.h>
#include <data.h>

#include <misc/InputStream.h>
#include <misc/OutputStream.h>

#include <list>
#include <vector>


#define DAMAGE_PER_TILE 5


// forward declarations
class House;
class ObjectBase;
class UnitBase;
class AirUnit;
class InfantryBase;


enum deadUnitEnum {
    DeadUnit_Infantry = 1,
    DeadUnit_Infantry_Squashed1 = 2,
    DeadUnit_Infantry_Squashed2 = 3,
    DeadUnit_Carrall = 4,
    DeadUnit_Ornithopter = 5
};

typedef struct
{
	Uint32 damageType;
	int	tile;
	Coord realPos;
} DAMAGETYPE;

typedef struct
{
    Coord   realPos;
    Sint16  timer;
    Uint8   type;
    Uint8   house;
    bool    onSand;
} DEADUNITTYPE;

enum destroyedStructureEnum {
    DestroyedStructure_None             = -1,
    DestroyedStructure_Wall             = 0,
    Destroyed1x1Structure               = 1,

    Destroyed3x3Structure_TopLeft       = 2,
    Destroyed3x3Structure_TopCenter     = 3,
    Destroyed3x3Structure_TopRight      = 4,
    Destroyed3x3Structure_CenterLeft    = 5,
    Destroyed3x3Structure_CenterCenter  = 6,
    Destroyed3x3Structure_CenterRight   = 7,
    Destroyed3x3Structure_BottomLeft    = 8,
    Destroyed3x3Structure_BottomCenter  = 9,
    Destroyed3x3Structure_BottomRight   = 10,

    Destroyed3x2Structure_TopLeft       = 5,
    Destroyed3x2Structure_TopCenter     = 6,
    Destroyed3x2Structure_TopRight      = 7,
    Destroyed3x2Structure_BottomLeft    = 8,
    Destroyed3x2Structure_BottomCenter  = 9,
    Destroyed3x2Structure_BottomRight   = 10,

    Destroyed2x2Structure_TopLeft       = 1,
    Destroyed2x2Structure_TopRight      = 11,
    Destroyed2x2Structure_BottomLeft    = 12,
    Destroyed2x2Structure_BottomRight   = 13
};


class Tile
{
public:

    typedef enum {
        Terrain_RockDamage,
        Terrain_SandDamage
    } TerrainDamage_enum;

    typedef enum {
        SandDamage1 = 0,
        SandDamage2 = 1,
        SandDamage3 = 2,
        SandDamage4 = 3
    } SANDDAMAGETYPE;

    typedef enum {
        RockDamage1 = 0,
        RockDamage2 = 1
    } ROCKDAMAGETYPE;

    typedef enum {
        TerrainTile_SlabHalfDestroyed   = 0x00,
        TerrainTile_SlabDestroyed       = 0x01,
        TerrainTile_Slab                = 0x02,

        TerrainTile_Sand                = 0x03,

        TerrainTile_Rock                = 0x04,
        TerrainTile_RockIsland          = TerrainTile_Rock + 0x00,
        TerrainTile_RockUp              = TerrainTile_Rock + 0x01,
        TerrainTile_RockRight           = TerrainTile_Rock + 0x02,
        TerrainTile_RockUpRight         = TerrainTile_Rock + 0x03,
        TerrainTile_RockDown            = TerrainTile_Rock + 0x04,
        TerrainTile_RockUpDown          = TerrainTile_Rock + 0x05,
        TerrainTile_RockDownRight       = TerrainTile_Rock + 0x06,
        TerrainTile_RockNotLeft         = TerrainTile_Rock + 0x07,
        TerrainTile_RockLeft            = TerrainTile_Rock + 0x08,
        TerrainTile_RockUpLeft          = TerrainTile_Rock + 0x09,
        TerrainTile_RockLeftRight       = TerrainTile_Rock + 0x0A,
        TerrainTile_RockNotDown         = TerrainTile_Rock + 0x0B,
        TerrainTile_RockDownLeft        = TerrainTile_Rock + 0x0C,
        TerrainTile_RockNotRight        = TerrainTile_Rock + 0x0D,
        TerrainTile_RockNotUp           = TerrainTile_Rock + 0x0E,
        TerrainTile_RockFull            = TerrainTile_Rock + 0x0F,

        TerrainTile_Dunes               = 0x14,
        TerrainTile_DunesIsland         = TerrainTile_Dunes + 0x00,
        TerrainTile_DunesUp             = TerrainTile_Dunes + 0x01,
        TerrainTile_DunesRight          = TerrainTile_Dunes + 0x02,
        TerrainTile_DunesUpRight        = TerrainTile_Dunes + 0x03,
        TerrainTile_DunesDown           = TerrainTile_Dunes + 0x04,
        TerrainTile_DunesUpDown         = TerrainTile_Dunes + 0x05,
        TerrainTile_DunesDownRight      = TerrainTile_Dunes + 0x06,
        TerrainTile_DunesNotLeft        = TerrainTile_Dunes + 0x07,
        TerrainTile_DunesLeft           = TerrainTile_Dunes + 0x08,
        TerrainTile_DunesUpLeft         = TerrainTile_Dunes + 0x09,
        TerrainTile_DunesLeftRight      = TerrainTile_Dunes + 0x0A,
        TerrainTile_DunesNotDown        = TerrainTile_Dunes + 0x0B,
        TerrainTile_DunesDownLeft       = TerrainTile_Dunes + 0x0C,
        TerrainTile_DunesNotRight       = TerrainTile_Dunes + 0x0D,
        TerrainTile_DunesNotUp          = TerrainTile_Dunes + 0x0E,
        TerrainTile_DunesFull           = TerrainTile_Dunes + 0x0F,

        TerrainTile_Mountain            = 0x24,
        TerrainTile_MountainIsland      = TerrainTile_Mountain + 0x00,
        TerrainTile_MountainUp          = TerrainTile_Mountain + 0x01,
        TerrainTile_MountainRight       = TerrainTile_Mountain + 0x02,
        TerrainTile_MountainUpRight     = TerrainTile_Mountain + 0x03,
        TerrainTile_MountainDown        = TerrainTile_Mountain + 0x04,
        TerrainTile_MountainUpDown      = TerrainTile_Mountain + 0x05,
        TerrainTile_MountainDownRight   = TerrainTile_Mountain + 0x06,
        TerrainTile_MountainNotLeft     = TerrainTile_Mountain + 0x07,
        TerrainTile_MountainLeft        = TerrainTile_Mountain + 0x08,
        TerrainTile_MountainUpLeft      = TerrainTile_Mountain + 0x09,
        TerrainTile_MountainLeftRight   = TerrainTile_Mountain + 0x0A,
        TerrainTile_MountainNotDown     = TerrainTile_Mountain + 0x0B,
        TerrainTile_MountainDownLeft    = TerrainTile_Mountain + 0x0C,
        TerrainTile_MountainNotRight    = TerrainTile_Mountain + 0x0D,
        TerrainTile_MountainNotUp       = TerrainTile_Mountain + 0x0E,
        TerrainTile_MountainFull        = TerrainTile_Mountain + 0x0F,

        TerrainTile_Spice               = 0x34,
        TerrainTile_SpiceIsland         = TerrainTile_Spice + 0x00,
        TerrainTile_SpiceUp             = TerrainTile_Spice + 0x01,
        TerrainTile_SpiceRight          = TerrainTile_Spice + 0x02,
        TerrainTile_SpiceUpRight        = TerrainTile_Spice + 0x03,
        TerrainTile_SpiceDown           = TerrainTile_Spice + 0x04,
        TerrainTile_SpiceUpDown         = TerrainTile_Spice + 0x05,
        TerrainTile_SpiceDownRight      = TerrainTile_Spice + 0x06,
        TerrainTile_SpiceNotLeft        = TerrainTile_Spice + 0x07,
        TerrainTile_SpiceLeft           = TerrainTile_Spice + 0x08,
        TerrainTile_SpiceUpLeft         = TerrainTile_Spice + 0x09,
        TerrainTile_SpiceLeftRight      = TerrainTile_Spice + 0x0A,
        TerrainTile_SpiceNotDown        = TerrainTile_Spice + 0x0B,
        TerrainTile_SpiceDownLeft       = TerrainTile_Spice + 0x0C,
        TerrainTile_SpiceNotRight       = TerrainTile_Spice + 0x0D,
        TerrainTile_SpiceNotUp          = TerrainTile_Spice + 0x0E,
        TerrainTile_SpiceFull           = TerrainTile_Spice + 0x0F,

        TerrainTile_ThickSpice          = 0x44,
        TerrainTile_ThickSpiceIsland    = TerrainTile_ThickSpice + 0x00,
        TerrainTile_ThickSpiceUp        = TerrainTile_ThickSpice + 0x01,
        TerrainTile_ThickSpiceRight     = TerrainTile_ThickSpice + 0x02,
        TerrainTile_ThickSpiceUpRight   = TerrainTile_ThickSpice + 0x03,
        TerrainTile_ThickSpiceDown      = TerrainTile_ThickSpice + 0x04,
        TerrainTile_ThickSpiceUpDown    = TerrainTile_ThickSpice + 0x05,
        TerrainTile_ThickSpiceDownRight = TerrainTile_ThickSpice + 0x06,
        TerrainTile_ThickSpiceNotLeft   = TerrainTile_ThickSpice + 0x07,
        TerrainTile_ThickSpiceLeft      = TerrainTile_ThickSpice + 0x08,
        TerrainTile_ThickSpiceUpLeft    = TerrainTile_ThickSpice + 0x09,
        TerrainTile_ThickSpiceLeftRight = TerrainTile_ThickSpice + 0x0A,
        TerrainTile_ThickSpiceNotDown   = TerrainTile_ThickSpice + 0x0B,
        TerrainTile_ThickSpiceDownLeft  = TerrainTile_ThickSpice + 0x0C,
        TerrainTile_ThickSpiceNotRight  = TerrainTile_ThickSpice + 0x0D,
        TerrainTile_ThickSpiceNotUp     = TerrainTile_ThickSpice + 0x0E,
        TerrainTile_ThickSpiceFull      = TerrainTile_ThickSpice + 0x0F,

        TerrainTile_SpiceBloom          = 0x54,
        TerrainTile_SpecialBloom        = 0x55
    } TERRAINTILETYPE;


    /**
        Default constructor. Creates a tile of type Terrain_Sand.
    */
	Tile();
	~Tile();

	void load(InputStream& stream);
	void save(OutputStream& stream) const;

	void assignAirUnit(Uint32 newObjectID);
	void assignDeadUnit(Uint8 type, Uint8 house, const Coord& position) {
        DEADUNITTYPE newDeadUnit;
        newDeadUnit.type = type;
        newDeadUnit.house = house;
        newDeadUnit.onSand = isSand() || isDunes();
        newDeadUnit.realPos = position;
        newDeadUnit.timer = 2000;

        deadUnits.push_back(newDeadUnit);
	}

	void assignNonInfantryGroundObject(Uint32 newObjectID);
	int assignInfantry(Uint32 newObjectID, Sint8 currentPosition = INVALID_POS);
	void assignUndergroundUnit(Uint32 newObjectID);

    /**
        This method draws the terrain of this tile
        \param xPos the x position of the left top corner of this tile on the screen
        \param yPos the y position of the left top corner of this tile on the screen
    */
	void blitGround(int xPos, int yPos);

    /**
        This method draws the structures.
        \param xPos the x position of the left top corner of this tile on the screen
        \param yPos the y position of the left top corner of this tile on the screen
    */
	void blitStructures(int xPos, int yPos);

    /**
        This method draws the underground units of this tile.
        \param xPos the x position of the left top corner of this tile on the screen
        \param yPos the y position of the left top corner of this tile on the screen
    */
	void blitUndergroundUnits(int xPos, int yPos);

    /**
        This method draws the dead units of this tile.
        \param xPos the x position of the left top corner of this tile on the screen
        \param yPos the y position of the left top corner of this tile on the screen
    */
	void blitDeadUnits(int xPos, int yPos);

    /**
        This method draws the infantry units of this tile.
        \param xPos the x position of the left top corner of this tile on the screen
        \param yPos the y position of the left top corner of this tile on the screen
    */
    void blitInfantry(int xPos, int yPos);

    /**
        This method draws the ground units of this tile.
        \param xPos the x position of the left top corner of this tile on the screen
        \param yPos the y position of the left top corner of this tile on the screen
    */
	void blitNonInfantryGroundUnits(int xPos, int yPos);

    /**
        This method draws the air units of this tile.
        \param xPos the x position of the left top corner of this tile on the screen
        \param yPos the y position of the left top corner of this tile on the screen
    */
	void blitAirUnits(int xPos, int yPos);

    /**
        This method draws the infantry units of this tile.
        \param xPos the x position of the left top corner of this tile on the screen
        \param yPos the y position of the left top corner of this tile on the screen
    */
	void blitSelectionRects(int xPos, int yPos);


	inline void update() {

	    // Performance tweak because this function is called alot (every game cycle for every tile)
	    bool bHasTracks = false;
        for(int i=0;i<NUM_ANGLES;i++) {
            if(tracksCounter[i] != 0) {
                bHasTracks = true;
                break;
            }
        }

        if(bHasTracks) {
            for(int i=0;i<NUM_ANGLES;i++) {
                if(tracksCounter[i] > 0) {
                    tracksCounter[i]--;
                }
            }
        }

        for(int i=0 ; i < (int)deadUnits.size() ; i++) {
            if(deadUnits[i].timer == 0) {
                deadUnits.erase(deadUnits.begin()+i);
                i--;
            } else {
                deadUnits[i].timer--;
            }
        }
    }

	void clearTerrain();

	inline void setTrack(Uint8 direction) {
	    if(type == Terrain_Sand || type == Terrain_Dunes
            || type == Terrain_Spice || type == Terrain_ThickSpice) {
            tracksCounter[direction] = 5000;
	    }
    }

	void selectAllPlayersUnits(int houseID, ObjectBase** lastCheckedObject, ObjectBase** lastSelectedObject);
	void selectAllPlayersUnitsOfType(int houseID, int itemID, ObjectBase** lastCheckedObject, ObjectBase** lastSelectedObject);
	void unassignAirUnit(Uint32 objectID);
	void unassignNonInfantryGroundObject(Uint32 objectID);
	void unassignObject(Uint32 objectID);
	void unassignInfantry(Uint32 objectID, int currentPosition);
	void unassignUndergroundUnit(Uint32 objectID);
	void setType(int newType);
	void squash();
	int getInfantryTeam();
	float harvestSpice();
	void setSpice(float newSpice);

	/**
        Returns the center point of this tile
        \return the center point in world coordinates
	*/
	Coord getCenterPoint() const {
	    return Coord( location.x*TILESIZE + (TILESIZE/2), location.y*TILESIZE + (TILESIZE/2) );
	}

    /*!
		returns a pointer to an air unit on this tile (if there's one)
		@return AirUnit* pointer to air unit
	*/
	AirUnit* getAirUnit();

	/*!
		returns a pointer to a non infantry ground object on this tile (if there's one)
		@return ObjectBase*  pointer to non infantry ground object
	*/
	ObjectBase* getNonInfantryGroundObject();
	/*!
		returns a pointer to an underground object on this tile (if there's one)
		@return UnitBase*  pointer to underground object(sandworm?)
	*/
	UnitBase* getUndergroundUnit();

	/*!
		returns a pointer to an ground object on this tile (if there's one)
		@return ObjectBase*  pointer to ground object
	*/
	ObjectBase* getGroundObject();

	/*!
		returns a pointer to infantry object on this tile (if there's one)
		@return InfantryBase*  pointer to infantry object
	*/
	InfantryBase* getInfantry();
	ObjectBase* getObject();
	ObjectBase* getObjectAt(int x, int y);
	ObjectBase* getObjectWithID(Uint32 objectID);


    const std::list<Uint32>& getAirUnitList() const {
        return assignedAirUnitList;
    }

	const std::list<Uint32>& getInfantryList() const {
        return assignedInfantryList;
    }

	const std::list<Uint32>& getUndergroundUnitList() const {
        return assignedUndergroundUnitList;
    }

	const std::list<Uint32>& getNonInfantryGroundObjectList() const {
        return assignedNonInfantryGroundObjectList;
    }

	/**
        This method is called when the spice bloom on this till shall be triggered. If this tile has no spice bloom nothing happens.
        \param  pTrigger    the house that triggered the bloom
	*/
	void triggerSpiceBloom(House* pTrigger);

    /**
        This method is called when the spice bloom on this tile shall be triggered. If this tile has no spice bloom nothing happens.
        \param  pTrigger    the house that triggered the bloom
	*/
	void triggerSpecialBloom(House* pTrigger);

	/**
        Sets this tile as explored for this house.
        \param  houseID the house this tile should be explored for
        \param  cycle   the cycle this happens (normally the current game cycle)
	*/
	inline void setExplored(int houseID, Uint32 cycle) {
        lastAccess[houseID] = cycle;
        explored[houseID] = true;
    }

	inline void setOwner(int newOwner) { owner = newOwner; }
	inline void setSandRegion(int newSandRegion) { sandRegion = newSandRegion; }
	inline void setDestroyedStructureTile(int newDestroyedStructureTile) { destroyedStructureTile = newDestroyedStructureTile; };

	inline bool hasAGroundObject() const { return (hasInfantry() || hasANonInfantryGroundObject()); }
	inline bool hasAnAirUnit() const { return !assignedAirUnitList.empty(); }
	inline bool hasAnUndergroundUnit() const { return !assignedUndergroundUnitList.empty(); }
	inline bool hasANonInfantryGroundObject() const { return !assignedNonInfantryGroundObjectList.empty(); }
	bool hasAStructure() const;
	inline bool hasInfantry() const { return !assignedInfantryList.empty(); }
    inline bool hasAnObject() { return (hasAGroundObject() || hasAnAirUnit() || hasAnUndergroundUnit()); }

	inline bool hasSpice() const { return (fixFloat(spice) > 0.0f); }
	inline bool infantryNotFull() const { return (assignedInfantryList.size() < NUM_INFANTRY_PER_TILE); }
	inline bool isConcrete() const { return (type == Terrain_Slab); }
	inline bool isExplored(int houseID) const {return explored[houseID];}

	bool isFogged(int houseID);
	inline bool isMountain() const { return (type == Terrain_Mountain);}
	inline bool isRock() const { return ((type == Terrain_Rock) || (type == Terrain_Slab) || (type == Terrain_Mountain));}

	inline bool isSand() const { return (type == Terrain_Sand); }
	inline bool isDunes() const { return (type == Terrain_Dunes); }
	inline bool isSpiceBloom() const { return (type == Terrain_SpiceBloom); }
	inline bool isSpecialBloom() const { return (type == Terrain_SpecialBloom); }
	inline bool isSpice() const { return ((type == Terrain_Spice) || (type == Terrain_ThickSpice)); }
	inline bool isThickSpice() const { return (type == Terrain_ThickSpice); }

	inline int getSandRegion() const { return sandRegion; }
	inline int getOwner() const { return owner; }
	inline int getType() const {	return type; }
	inline float getSpice() const { return spice; }

	/**
        Returns how fast a unit can move over this tile.
        \return Returns a speed factor. Higher values mean slower.
	*/
	inline float getDifficulty() const {
	    switch(type) {
            case Terrain_Slab:          return 0.7f;
            case Terrain_Sand:          return 1.2f;
            case Terrain_Rock:          return 1.0f;
            case Terrain_Dunes:         return 1.5f;
            case Terrain_Mountain:      return 1.5f;
            case Terrain_Spice:         return 1.2f;
            case Terrain_ThickSpice:    return 1.2f;
            case Terrain_SpiceBloom:    return 1.2f;
            case Terrain_SpecialBloom:  return 1.2f;
            default:                    return 1.0f;
	    }
    };

	inline float getSpiceRemaining() { return fixFloat(spice); }

	inline const Coord& getLocation() const { return location; }

	Uint32 getRadarColor(House* pHouse, bool radar);
	int getTerrainTile() const;
    int getHideTile(int houseID) const;
    int getFogTile(int houseID) const;
    int getDestroyedStructureTile() const { return  destroyedStructureTile; };

    bool isBlocked() const {
        return (isMountain() || hasAGroundObject());
    }


    void addDamage(Uint32 damageType, int tile, Coord realPos) {
        if(damage.size() < DAMAGE_PER_TILE) {
            DAMAGETYPE newDamage;
            newDamage.tile = tile;
            newDamage.damageType = damageType;
            newDamage.realPos = realPos;

            damage.push_back(newDamage);
        }
    }

	Coord	location;   ///< location of this tile in map coordinates

private:

	Uint32  	type;   ///< the type of the tile (Terrain_Sand, Terrain_Rock, ...)

	Uint32      fogColor;       ///< remember last color (radar)

	Sint32      owner;          ///< house ID of the owner of this tile
	Uint32      sandRegion;     ///< used by sandworms to check if can get to a unit

	float       spice;          ///< how much spice on this particular tile is left

    Sint32                          destroyedStructureTile;     ///< the tile drawn for a destroyed structure
	Sint16                          tracksCounter[NUM_ANGLES];  ///< Contains counters for the tracks on sand
	std::vector<DAMAGETYPE>         damage;                     ///< damage positions
	std::vector<DEADUNITTYPE>       deadUnits;                  ///< dead units

	std::list<Uint32>	assignedAirUnitList;                    ///< all the air units on this tile
	std::list<Uint32>	assignedInfantryList;                   ///< all infantry units on this tile
	std::list<Uint32>	assignedUndergroundUnitList;            ///< all underground units on this tile
	std::list<Uint32>	assignedNonInfantryGroundObjectList;    ///< all structures/vehicles on this tile

	SDL_Surface**       sprite;    ///< the graphic to draw


	Uint32      lastAccess[NUM_HOUSES];    ///< contains for every house when this tile was seen last by this house
	bool        explored[NUM_HOUSES];      ///< contains for every house if this tile is explored
};



#endif //TILE_H
