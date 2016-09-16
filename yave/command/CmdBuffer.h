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
#ifndef YAVE_CMDBUFFER_H
#define YAVE_CMDBUFFER_H

#include <yave/yave.h>
#include <yave/DeviceLinked.h>

namespace yave {

class CmdBufferPoolData;

struct CmdBufferData {
	vk::CommandBuffer cmd_buffer;
	vk::Fence fence;
};

struct CmdBuffer : NonCopyable {

	public:
		// we need to have a complete definition of CmdBufferPoolData to make this = default
		CmdBuffer();
		CmdBuffer(const core::Rc<CmdBufferPoolData>& pool);

		CmdBuffer(CmdBuffer&& other);
		CmdBuffer& operator=(CmdBuffer&& other);

		~CmdBuffer();

		const vk::CommandBuffer get_vk_cmd_buffer() const;
		vk::Fence get_vk_fence() const;
		DevicePtr get_device() const;

	protected:
		void swap(CmdBuffer& other);
		void submit(vk::Queue queue);

	private:
		core::Rc<CmdBufferPoolData> _pool;
		NotOwned<CmdBufferData> _data;
};

}

#endif // YAVE_CMDBUFFERSTATE_H
