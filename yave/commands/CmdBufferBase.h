/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#ifndef YAVE_COMMANDS_CMDBUFFERBASE_H
#define YAVE_COMMANDS_CMDBUFFERBASE_H

#include <yave/yave.h>
#include <yave/DeviceLinked.h>

namespace yave {

class CmdBufferPoolData;

struct CmdBufferData {
	vk::CommandBuffer cmd_buffer;
	vk::Fence fence;
};

enum class CmdBufferUsage {
	Normal = uenum(vk::CommandBufferUsageFlagBits::eSimultaneousUse),
	Disposable = uenum(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
};

template<CmdBufferUsage Usage>
class CmdBufferRecorder;

struct CmdBufferBase : NonCopyable {

	public:
		CmdBufferBase() = default;
		CmdBufferBase(const core::Rc<CmdBufferPoolData>& pool);

		~CmdBufferBase();

		const vk::CommandBuffer vk_cmd_buffer() const;
		vk::Fence vk_fence() const;
		DevicePtr device() const;

	protected:
		void swap(CmdBufferBase& other);
		void submit(vk::Queue queue);

	private:
		core::Rc<CmdBufferPoolData> _pool;
		NotOwner<CmdBufferData> _data;
};

}

#endif // YAVE_COMMANDS_CMDBUFFERBASE_H
