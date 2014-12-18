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

#ifndef BUILDERBASE_H
#define BUILDERBASE_H

#include <structures/StructureBase.h>

#include <data.h>

#include <list>
#include <string>

class BuildItem {
public:
	BuildItem() {
		itemID = ItemID_Invalid;
		price = 0;
		num = 0;
	}

	BuildItem(int itemID, int price) {
		this->itemID = itemID;
		this->price = price;
		num = 0;
	}

	void save(OutputStream& stream) const {
		stream.writeUint32(itemID);
		stream.writeUint32(price);
		stream.writeUint32(num);
	}

	void load(InputStream& stream) {
		itemID = stream.readUint32();
		price = stream.readUint32();
		num = stream.readUint32();
	}

	Uint32 itemID;
	Uint32 price;
	Uint32 num;
};

class ProductionQueueItem {
public:
    ProductionQueueItem()
     : itemID(0), price(0) {

    }

    ProductionQueueItem(Uint32 _ItemID, Uint32 _price)
     : itemID(_ItemID), price(_price) {

    }

	void save(OutputStream& stream) const {
		stream.writeUint32(itemID);
		stream.writeUint32(price);
	}

	void load(InputStream& stream) {
		itemID = stream.readUint32();
		price = stream.readUint32();
	}

    Uint32 itemID;
    Uint32 price;
};


class BuilderBase : public StructureBase
{
public:
	BuilderBase(House* newOwner);
    BuilderBase(InputStream& stream);
	void init();
	virtual ~BuilderBase();

	virtual void save(OutputStream& stream) const;

	virtual ObjectInterface* getInterfaceContainer();

	void setOwner(House *no);

	virtual void setOriginalHouseID(int i) {
        StructureBase::setOriginalHouseID(i);
        updateBuildList();
    }

    /**
        This method returns the maximum number of upgrades available in this building.
        \return the maximum number of upgrades available (0 if none)
    */
	int getMaxUpgradeLevel() const;

    /**
        This method checks what is available for purchase in this builder. It shall
        modify buildList appropriately.
    */
	virtual void updateBuildList();

	void setWaitingToPlace();
	void unSetWaitingToPlace();

	int getBuildListSize() const { return buildList.size(); };

	int getProductionQueueSize() const { return currentProductionQueue.size(); };

	/**
        Updates this builder.
        \return true if this object still exists, false if it was destroyed
	*/
	virtual bool update();

	virtual void handleUpgradeClick();
	virtual void handleProduceItemClick(Uint32 itemID, bool multipleMode = false);
	virtual void handleCancelItemClick(Uint32 itemID, bool multipleMode = false);
	virtual void handleSetOnHoldClick(bool OnHold);



    /**
        Start upgrading this builder if possible.
        \return true if upgrading was started, false if not possible or already upgrading
    */
	virtual bool doUpgrade();

	/**
        Start production of the specified item.
        \param  itemID          the item to produce
        \param  multipleMode    false = 1 item, true = 5 items
	*/
	virtual void doProduceItem(Uint32 itemID, bool multipleMode = false);

	/**
        Cancel production of the specified item.
        \param  itemID          the item to cancel
        \param  multipleMode    false = 1 item, true = 5 items
	*/
	virtual void doCancelItem(Uint32 itemID, bool multipleMode = false);

    /**
        Sets the currently produced item on hold or continues production.
        \param  bOnHold         true = hold production; false = resume production
    */
	inline void doSetOnHold(bool bOnHold) { bCurrentItemOnHold = bOnHold; };

    /**
        Start building a random item in this builder.
    */
	virtual void doBuildRandom();



	inline bool isUpgrading() const { return upgrading; };
	inline bool isAllowedToUpgrade() const { return (curUpgradeLev < getMaxUpgradeLevel()); };
	inline int getCurrentUpgradeLevel() const { return curUpgradeLev; };
	int getUpgradeCost() const;
	inline float getUpgradeProgress() const { return upgradeProgress; };

	inline Uint32 getCurrentProducedItem() const { return currentProducedItem; };
	inline bool isOnHold() const { return bCurrentItemOnHold; };
	bool isWaitingToPlace() const;
	inline float getProductionProgress() const { return productionProgress; };
	inline const std::list<BuildItem>& getBuildList() const { return buildList; };

	virtual inline bool isAvailableToBuild(Uint32 itemID) const {
		return (getBuildItem(itemID) != NULL);
	}

protected:
	virtual void updateProductionProgress();

	void removeBuiltItemFromProductionQueue();

	virtual void insertItem(std::list<BuildItem>& buildItemList, std::list<BuildItem>::iterator& iter, Uint32 itemID, int price=-1);

	void removeItem(std::list<BuildItem>& buildItemList, std::list<BuildItem>::iterator& iter, Uint32 itemID);

	BuildItem* getBuildItem(Uint32 itemID) {
		std::list<BuildItem>::iterator iter;
		for(iter = buildList.begin(); iter != buildList.end(); ++iter) {
			if(iter->itemID == itemID) {
				return &(*iter);
			}
		}
		return NULL;
	}

    const BuildItem* getBuildItem(Uint32 itemID) const {
		std::list<BuildItem>::const_iterator iter;
		for(iter = buildList.begin(); iter != buildList.end(); ++iter) {
			if(iter->itemID == itemID) {
				return &(*iter);
			}
		}
		return NULL;
	}

	void produceNextAvailableItem();

protected:
	static const int itemOrder[];  ///< the order in which items are in the build list

    // structure state
    bool    upgrading;              ///< Currently upgrading?
	float  upgradeProgress;        ///< The current state of the upgrade progress (measured in money spent)
	Uint8   curUpgradeLev;          ///< Current upgrade level

	bool    bCurrentItemOnHold;      ///< Is the currently produced item on hold?
	Uint32  currentProducedItem;    ///< The ItemID of the currently produced item
	float   productionProgress;     ///< The current state of the production progress (measured in money spent)
	Uint32  deployTimer;            ///< Timer for deploying a unit

	std::list<ProductionQueueItem>  currentProductionQueue;     ///< This list is the production queue (It contains the item IDs of the units/structures to produce)
	std::list<BuildItem>            buildList;                  ///< This list contains all the things that can be produced by this builder
};

#endif //BUILDERBASE_H
