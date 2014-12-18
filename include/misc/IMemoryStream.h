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

#ifndef IMEMORYSTREAM_H
#define IMEMORYSTREAM_H

#include "InputStream.h"
#include <stdlib.h>
#include <string>

class IMemoryStream : public InputStream
{
public:
	IMemoryStream()
	 : currentPos(0), bufferSize(0), pBuffer(NULL) {
        ;
    }

	IMemoryStream(const char* data, int length)
	 : currentPos(0), bufferSize(length), pBuffer(data) {
        ;
    }

	~IMemoryStream() {
	}

	void open(const char* data, int length) {
        currentPos = 0;
        bufferSize = length;
        pBuffer = data;
	}

	std::string readString() {
	    Uint32 length = readUint32();

		if(currentPos + length > bufferSize) {
            throw InputStream::eof("IMemoryStream::readString(): End-of-File reached!");
	    }

	    std::string resultString(pBuffer + currentPos, length);
	    currentPos += length;
	    return resultString;
	}

	Uint8 readUint8() {
	    if(currentPos + sizeof(Uint8) > bufferSize) {
            throw InputStream::eof("IMemoryStream::readUint8(): End-of-File reached!");
	    }

        Uint8 tmp = *((const Uint8*) (pBuffer + currentPos));
        currentPos += sizeof(Uint8);
        return tmp;
	}

	Uint16 readUint16() {
	    if(currentPos + sizeof(Uint16) > bufferSize) {
            throw InputStream::eof("IMemoryStream::readUint16(): End-of-File reached!");
	    }

        Uint16 tmp = *((const Uint16*) (pBuffer + currentPos));
        currentPos += sizeof(Uint16);
        return SDL_SwapLE16(tmp);
	}

	Uint32 readUint32() {
	    if(currentPos + sizeof(Uint32) > bufferSize) {
            throw InputStream::eof("IMemoryStream::readUint32(): End-of-File reached!");
	    }

        Uint32 tmp = *((const Uint32*) (pBuffer + currentPos));
        currentPos += sizeof(Uint32);
        return SDL_SwapLE32(tmp);
	}

	Uint64 readUint64() {
	    if(currentPos + sizeof(Uint64) > bufferSize) {
            throw InputStream::eof("IMemoryStream::readUint64(): End-of-File reached!");
	    }

        Uint64 tmp = *((const Uint64*) (pBuffer + currentPos));
        currentPos += sizeof(Uint64);
        return SDL_SwapLE64(tmp);
	}

	bool readBool() {
        return (readUint8() == 1 ? true : false);
    }

	float readFloat() {
        Uint32 tmp = readUint32();
        float tmp2;
        memcpy(&tmp2,&tmp,sizeof(Uint32)); // workaround for a strange optimization in gcc 4.1
        return tmp2;
    }

private:
    size_t      currentPos;
    size_t      bufferSize;
	const char* pBuffer;
};

#endif // IMEMORYSTREAM_H
