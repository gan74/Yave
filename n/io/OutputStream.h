/*******************************
Copyright (C) 2009-2010 grégoire ANGERAND

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

#ifndef N_IO_OUTPOUTSTREAM_H
#define N_IO_OUTPOUTSTREAM_H

#include <n/core/String.h>

namespace n {
namespace io {

class OutputStream
{
	public:
		virtual bool canWrite() const = 0;
		virtual uint writeBytes(const char *b, uint len) = 0;
		virtual void flush() = 0;
};

class DataOutputStream : public OutputStream
{
	public:
		DataOutputStream(OutputStream *s);
		bool canWrite() const override;
		void flush() override;
		uint writeBytes(const char *b, uint len) override;

		template<typename T>
		uint write(const T &t) {
			core::String s(t);
			return stream->writeBytes(s.toChar(), s.size());
		}

		template<typename T>
		DataOutputStream &operator<<(const T &t) {
			write(t);
			return *this;
		}

	private:
		OutputStream *stream;
};


} // io
} // n

#endif // N_IO_OUTPOUTSTREAM_H
