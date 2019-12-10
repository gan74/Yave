/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
#ifndef YAVE_GRAPHICS_COMMANDS_POOL_CMDBUFFERPOOLBASE_H
#define YAVE_GRAPHICS_COMMANDS_POOL_CMDBUFFERPOOLBASE_H

#include <y/concurrent/SpinLock.h>

#include <yave/yave.h>
#include <yave/device/DeviceLinked.h>

#include <yave/graphics/commands/CmdBufferUsage.h>
#include <yave/graphics/commands/data/CmdBufferData.h>

#include <yave/utils/traits.h>

namespace yave {

class CmdBufferDataProxy;

class CmdBufferPoolBase : NonMovable, public DeviceLinked {

	public:
		~CmdBufferPoolBase();

		vk::CommandPool vk_pool() const;

	protected:
		friend class CmdBufferDataProxy;
		friend class LifetimeManager;

		CmdBufferPoolBase() = default;
		CmdBufferPoolBase(DevicePtr dptr, CmdBufferUsage preferred);

		void release(CmdBufferData&& data);
		std::unique_ptr<CmdBufferDataProxy> alloc();

		void join_all();

	private:
		CmdBufferData create_data();

		concurrent::SpinLock _lock;
		vk::CommandPool _pool;
		CmdBufferUsage _usage;
		core::Vector<CmdBufferData> _cmd_buffers;
		core::Vector<vk::Fence> _fences;
};

static_assert(is_safe_base<CmdBufferPoolBase>::value);

}

#endif // YAVE_GRAPHICS_COMMANDS_POOL_CMDBUFFERPOOLBASE_H
