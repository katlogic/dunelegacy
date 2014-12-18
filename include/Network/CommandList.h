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

#ifndef COMMANDLIST_H
#define COMMANDLIST_H

#include <misc/InputStream.h>
#include <misc/OutputStream.h>

#include <Command.h>

#include <SDL.h>
#include <vector>

class CommandList {
public:
    class CommandListEntry {
    public:
        CommandListEntry(Uint32 cycle, std::vector<Command> commands)
         : cycle(cycle), commands(commands) {

        }

        CommandListEntry(InputStream& stream) {
            cycle = stream.readUint32();
            Uint32 numCommands = stream.readUint32();
            for(Uint32 i = 0; i < numCommands; i++) {
                commands.push_back(Command(stream));
            }
        }

        void save(OutputStream& stream) const {
            stream.writeUint32(cycle);

            stream.writeUint32((Uint32) commands.size());
            std::vector<Command>::const_iterator iter;
            for(iter = commands.begin(); iter != commands.end(); ++iter) {
                iter->save(stream);
            }
        }

        Uint32      cycle;
        std::vector<Command> commands;
    };

    CommandList() {
    }

    CommandList(InputStream& stream) {
        Uint32 numCommandListEntries = stream.readUint32();
        for(Uint32 i = 0; i < numCommandListEntries; i++) {
            commandList.push_back(CommandListEntry(stream));
        }
    }

    ~CommandList() {

    }

    void save(OutputStream& stream) const {
        stream.writeUint32((Uint32) commandList.size());

        std::vector<CommandListEntry>::const_iterator iter;
        for(iter = commandList.begin(); iter != commandList.end(); ++iter) {
            iter->save(stream);
        }
    }

    std::vector<CommandListEntry> commandList;
};

#endif //COMMANDLIST_H
