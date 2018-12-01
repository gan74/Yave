/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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
#ifndef YAVE_GRAPHICS_COMMANDS_DATA_CMDBUFFERDATA_H
#define YAVE_GRAPHICS_COMMANDS_DATA_CMDBUFFERDATA_H

#include <yave/graphics/commands/CmdBufferUsage.h>

namespace yave {

class CmdBufferDataProxy;

class CmdBufferData : NonCopyable {
	struct KeepAlive : NonCopyable {
		virtual ~KeepAlive() {}
	};

	public:
		CmdBufferData(vk::CommandBuffer buf, vk::Fence fen, CmdBufferPoolBase* p);

		CmdBufferData() = default;

		CmdBufferData(CmdBufferData&& other);
		CmdBufferData& operator=(CmdBufferData&& other);

		~CmdBufferData();

		DevicePtr device() const;

		CmdBufferPoolBase* pool() const;

		vk::CommandBuffer vk_cmd_buffer() const;
		vk::Fence vk_fence() const;

		void reset();
		bool try_reset();

		template<typename T>
		void keep_alive(T&& t) {
			struct Box : KeepAlive {
				Box(T&& t) : _t(y_fwd(t)) {}
				~Box() override {}
				std::remove_reference_t<T> _t;
			};
			_keep_alive.emplace_back(new Box(y_fwd(t)));
		}


	protected:
		void swap(CmdBufferData& other);

	private:
		vk::CommandBuffer _cmd_buffer;
		vk::Fence _fence;

		core::Vector<std::unique_ptr<KeepAlive>> _keep_alive;
		CmdBufferPoolBase* _pool = nullptr;
};


class CmdBufferDataProxy {

	public:
		CmdBufferDataProxy() = default;
		CmdBufferDataProxy(CmdBufferDataProxy&& other);
		CmdBufferDataProxy& operator=(CmdBufferDataProxy&& other);

		CmdBufferDataProxy(CmdBufferData&& d);

		~CmdBufferDataProxy();

		CmdBufferData& data();

	private:
		CmdBufferData _data;

};


}

#endif // YAVE_GRAPHICS_COMMANDS_DATA_CMDBUFFERDATA_H
