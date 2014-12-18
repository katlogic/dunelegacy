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

#ifndef TRIGGER_H
#define TRIGGER_H

#include <misc/InputStream.h>
#include <misc/OutputStream.h>

#include <SDL.h>

/**
    This class is the base class for all triggers.
*/
class Trigger {
public:

    /**
        Constructor
        \param  triggerCycleNumber  the game cycle this trigger shall be triggered
    */
    Trigger(Uint32 triggerCycleNumber) : cycleNumber(triggerCycleNumber) { };

    /**
        This constructor constructs the trigger from a stream.
        \param  stream  the stream to read from
    */
    Trigger(InputStream& stream) {
        cycleNumber = stream.readUint32();
    }

    /// destructor
    virtual ~Trigger() { };

    /**
        This method saves this trigger to a stream.
        \param  stream  the stream to save to
    */
    virtual void save(OutputStream& stream) {
        stream.writeUint32(cycleNumber);
    }

    /**
        This method returns the game cycle this trigger shall be triggered.
        \return the cycle number this trigger shall be triggered
    */
    Uint32 getCycleNumber() const { return cycleNumber; };

    /**
        Trigger this trigger. Shall only be called when getCycleNumber() is equal to the current game cycle
    */
    virtual void trigger() = 0;

protected:
    Uint32 cycleNumber;     ///< the game cycle this trigger shall be triggered
};

#endif // TRIGGER_H
