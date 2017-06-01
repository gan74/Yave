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
#ifndef YAVE_COMMANDS_CMDBUFFERRECORDER_H
#define YAVE_COMMANDS_CMDBUFFERRECORDER_H

#include "CmdBufferRecorderBase.h"
#include "RecordedCmdBuffer.h"

namespace yave {

namespace detail {

template<CmdBufferUsage Usage>
class RecorderConstructor : public CmdBufferRecorderBase {
	public:
		RecorderConstructor() = default;

		RecorderConstructor(CmdBuffer<Usage>&& buffer) : CmdBufferRecorderBase(std::move(buffer), Usage) {
		}
};

template<>
class RecorderConstructor<CmdBufferUsage::Secondary> : public CmdBufferRecorderBase {
	public:
		RecorderConstructor() = default;

		RecorderConstructor(CmdBuffer<CmdBufferUsage::Secondary>&& buffer, const Framebuffer& framebuffer) : CmdBufferRecorderBase(std::move(buffer), framebuffer) {
		}

};

}

template<CmdBufferUsage Usage>
class CmdBufferRecorder : public detail::RecorderConstructor<Usage> {

	public:
		using detail::RecorderConstructor<Usage>::RecorderConstructor;

		CmdBufferRecorder(CmdBufferRecorder&& other) : detail::RecorderConstructor<Usage>() {
			this->swap(other);
		}

		CmdBufferRecorder& operator=(CmdBufferRecorder&& other) {
			this->swap(other);
			return *this;
		}

		~CmdBufferRecorder() {
			if(this->vk_cmd_buffer()) {
				fatal("CmdBufferRecorder destroyed before end() was called.");
			}
		}

		 RecordedCmdBuffer<Usage> end() {
			this->end_render_pass();
			this->vk_cmd_buffer().end();
			return std::move(this->cmd_buffer());
		}

	private:

};

}

#endif // YAVE_COMMANDS_CMDBUFFERRECORDER_H
