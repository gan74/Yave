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

template<CmdBufferUsage Usage>
class CmdBufferRecorder : public CmdBufferRecorderBase {

	public:
		CmdBufferRecorder(CmdBuffer<Usage>&& buffer) : CmdBufferRecorderBase(std::move(buffer), Usage) {
		}

		CmdBufferRecorder(CmdBufferRecorder&& other) : CmdBufferRecorderBase() {
			swap(other);
		}

		CmdBufferRecorder& operator=(CmdBufferRecorder&& other) {
			swap(other);
			return *this;
		}

		~CmdBufferRecorder() {
			if(vk_cmd_buffer()) {
				fatal("CmdBufferRecorder destroyed before end() was called.");
			}
		}

		 RecordedCmdBuffer<Usage> end() {
			end_render_pass();
			vk_cmd_buffer().end();
			return std::move(cmd_buffer());
		}

	private:

};

}

#endif // YAVE_COMMANDS_CMDBUFFERRECORDER_H
