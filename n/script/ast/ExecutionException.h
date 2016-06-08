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
#ifndef N_SCRIPT_EXECUTIONEXCEPTION_H
#define N_SCRIPT_EXECUTIONEXCEPTION_H

#include <n/core/String.h>

namespace n {
namespace script {
namespace ast {

class ExecutionException : std::exception
{
	public:
		virtual const char *what() const noexcept override;
		virtual const char *what(const core::String &code) const noexcept;

		ExecutionException(const core::String &m, uint ind);

	protected:
		core::String locString(const core::String &code) const;

	private:
		core::String msg;
		uint index;
};

}
}
}

#endif // N_SCRIPT_EXECUTIONEXCEPTION_H
