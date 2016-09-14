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
#ifndef YAVE_CMDBUFFERSTATE_H
#define YAVE_CMDBUFFERSTATE_H

#include "yave.h"
#include "DeviceLinked.h"

namespace yave {

struct CmdBufferState : NonCopyable, public DeviceLinked {

	public:
		CmdBufferState(DevicePtr dptr, vk::CommandPool pool);
		~CmdBufferState();


		bool is_done() const;
		void wait() const;

		void submit(vk::Queue queue);

		const vk::CommandBuffer& get_vk_cmd_buffer() const;
		vk::Fence get_vk_fence() const;

	private:
		NotOwned<vk::CommandPool> _pool;
		vk::CommandBuffer _cmd_buffer;
		vk::Fence _fence;

};

}

#endif // YAVE_CMDBUFFERSTATE_H
