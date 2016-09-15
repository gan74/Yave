/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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
#ifndef YAVE_CMDBUFFERPOOL_H
#define YAVE_CMDBUFFERPOOL_H

#include "CmdBufferState.h"

namespace yave {

class CmdBufferPool : NonCopyable, public DeviceLinked {

	public:
		CmdBufferPool() = default;
		CmdBufferPool(DevicePtr dptr);
		~CmdBufferPool();

		CmdBufferPool(CmdBufferPool&& other);
		CmdBufferPool &operator=(CmdBufferPool&& other);

		core::Rc<CmdBufferState> get_buffer	();

	private:
		void swap(CmdBufferPool& other);

		vk::CommandPool _pool;
		core::Vector<core::Rc<CmdBufferState>> _cmd_buffers;
};

}

#endif // YAVE_CMDBUFFERPOOL_H
