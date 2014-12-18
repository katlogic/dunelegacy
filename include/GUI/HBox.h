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

#ifndef HBOX_H
#define HBOX_H

#include "Container.h"

class HBox_WidgetData {
public:
	HBox_WidgetData() : pWidget(NULL), fixedWidth(0), weight(0.0) { };
	HBox_WidgetData(Widget* _pWidget, Sint32 _fixedWidth) : pWidget(_pWidget), fixedWidth(_fixedWidth), weight(0.0) { };
	HBox_WidgetData(Widget* _pWidget, double _weight) : pWidget(_pWidget), fixedWidth(-1), weight(_weight) { };

	Widget* pWidget;
	Sint32 fixedWidth;
	double weight;
};

/// A container class for horizontal aligned widgets.
class HBox : public Container<HBox_WidgetData> {
public:
	/// default constructor
	HBox() : Container<HBox_WidgetData>() {
		;
	}

	/// destructor
	virtual ~HBox() {
		;
	}

	/**
		This method adds a new widget to this container.
		\param newWidget	Widget to add
		\param fixedWidth	a fixed width for this widget (must be greater than the minimum size)
	*/
	virtual void addWidget(Widget* newWidget, Sint32 fixedWidth) {
		if(newWidget != NULL) {
			containedWidgets.push_back(HBox_WidgetData(newWidget, fixedWidth));
			newWidget->setParent(this);
			Widget::resizeAll();
		}
	}

	/**
		This method adds a new widget to this container.
		\param newWidget	Widget to add
		\param weight		The weight for this widget (default=1.0)
	*/
	virtual void addWidget(Widget* newWidget, double weight = 1.0) {
		if(newWidget != NULL) {
			containedWidgets.push_back(HBox_WidgetData(newWidget, weight));
			newWidget->setParent(this);
			Widget::resizeAll();
		}
	}

	/**
		Returns the minimum size of this container. The container should not
		resized to a size smaller than this. If the container is not resizeable
		in a direction this method returns the size in that direction.
		\return the minimum size of this container
	*/
	virtual Point getMinimumSize() const {
		Point p(0,0);
		WidgetList::const_iterator iter;
		for(iter = containedWidgets.begin(); iter != containedWidgets.end(); ++iter) {
			Widget* curWidget = iter->pWidget;
			if(iter->fixedWidth > 0) {
                p.x += iter->fixedWidth;
			} else {
                p.x += curWidget->getMinimumSize().x;
			}
			p.y = std::max(p.y,curWidget->getMinimumSize().y);
		}
		return p;
	}

	/**
		This method resizes the container to width and height. This method should only be
		called if the new size is a valid size for this container (See resizingXAllowed,
		resizingYAllowed, getMinumumSize). It also resizes all child widgets.
		\param	width	the new width of this container
		\param	height	the new height of this container
	*/
	virtual void resize(Uint32 width, Uint32 height) {
		Sint32 availableWidth = width;

		int numRemainingWidgets = containedWidgets.size();

		// Find objects that are not allowed to be resized or have a fixed width
		// also find the sum of all weights
		double weightSum = 0.0;
		WidgetList::const_iterator iter;
		for(iter = containedWidgets.begin(); iter != containedWidgets.end(); ++iter) {
			Widget* curWidget = iter->pWidget;
			if(curWidget->resizingXAllowed() == false) {
				availableWidth = availableWidth - curWidget->getSize().x;
				numRemainingWidgets--;
			} else if(iter->fixedWidth > 0) {
				availableWidth = availableWidth - iter->fixedWidth;
				numRemainingWidgets--;
			} else {
				weightSum += iter->weight;
			}
		}

		// Under the resizeable widgets find all objects that are oversized (minimum size > availableWidth*weight)
		// also calculate the weight sum of all the resizeable widgets that are not oversized
		Sint32 neededOversizeWidth = 0;
		double notOversizedWeightSum = 0.0;
		for(iter = containedWidgets.begin(); iter != containedWidgets.end(); ++iter) {
			Widget* curWidget = iter->pWidget;
			if(curWidget->resizingXAllowed() == true && iter->fixedWidth <= 0) {
				if((double) curWidget->getMinimumSize().x > availableWidth * (iter->weight/weightSum)) {
					neededOversizeWidth += curWidget->getMinimumSize().x;
				} else {
					notOversizedWeightSum += iter->weight;
				}
			}
		}


		Sint32 totalAvailableWidth = availableWidth;
		for(iter = containedWidgets.begin(); iter != containedWidgets.end(); ++iter) {
			Widget* curWidget = iter->pWidget;
			Sint32 widgetHeight;
			if(curWidget->resizingYAllowed() == true) {
				widgetHeight = height;
			} else {
				widgetHeight = curWidget->getMinimumSize().y;
			}

			if(curWidget->resizingXAllowed() == true) {
				Sint32 widgetWidth = 0;

				if(iter->fixedWidth <= 0) {
					if(numRemainingWidgets <= 1) {
						widgetWidth = availableWidth;
					} else if((double) curWidget->getMinimumSize().x > totalAvailableWidth * (iter->weight/weightSum)) {
						widgetWidth = curWidget->getMinimumSize().x;
					} else {
						widgetWidth = (Sint32) ((totalAvailableWidth-neededOversizeWidth) * (iter->weight/notOversizedWeightSum));
					}
					availableWidth -= widgetWidth;
					numRemainingWidgets--;
				} else {
					widgetWidth = iter->fixedWidth;
				}

				curWidget->resize(widgetWidth,widgetHeight);
			} else {
				curWidget->resize(curWidget->getSize().x,widgetHeight);
			}
		}

		Container<HBox_WidgetData>::resize(width,height);
	}

    /**
		This static method creates a dynamic HBox object.
		The idea behind this method is to simply create a new HBox on the fly and
		add it to a container. If the container gets destroyed also this HBox will be freed.
		\return	The new created HBox (will be automatically destroyed when it's parent widget is destroyed)
	*/
	static HBox* create() {
		HBox* hbox = new HBox();
		hbox->pAllocated = true;
		return hbox;
	}

protected:
	/**
		This method must be overwritten by all container classes. It should return
		the position of the specified widget.
		\param widgetData	the widget data to get the position from.
		\return The position of the left upper corner
	*/
	virtual Point getPosition(const HBox_WidgetData& widgetData) const {
		Point p(0,0);
		WidgetList::const_iterator iter;
		for(iter = containedWidgets.begin(); iter != containedWidgets.end(); ++iter) {
			Widget* curWidget = iter->pWidget;
			if(widgetData.pWidget == curWidget) {
			    p.y = (getSize().y - curWidget->getSize().y)/2;
				return p;
			} else {
				p.x = p.x + curWidget->getSize().x;
			}
		}

		//should not happen
		return Point(0,0);
	}
};

#endif // HBOX_H

