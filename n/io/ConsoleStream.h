/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#ifndef N_IO_CONSOLESTREAM_H
#define N_IO_CONSOLESTREAM_H

#include "TextInputStream.h"
#include "TextOutputStream.h"

namespace n {
namespace io {

class RawConsoleInputStream : public InputStream
{
	public:

};

class ConsoleInputStream : public TextInputStream
{
	public:
		ConsoleInputStream();

		bool atEnd() const override;
		bool canRead() const override;
		uint readBytes(void *b, uint l = -1) override;
};

class RawConsoleOutputStream;

class ConsoleOutputStream : public TextOutputStream
{
	public:
		enum Channel
		{
			Out,
			Error
		};

		ConsoleOutputStream(Channel ch = Out);

		bool canWrite() const override;
		uint writeBytes(const void *b, uint len) override;
		void flush() override;

	private:
		Channel channel;
};

extern ConsoleInputStream in;
extern ConsoleOutputStream out;
extern ConsoleOutputStream err;

}
}

#endif // CONSOLESTREAM_H
