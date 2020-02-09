/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#ifndef EDITOR_CONTEXT_LOGS_H
#define EDITOR_CONTEXT_LOGS_H

#include <editor/editor.h>

#include <y/core/Vector.h>
#include <y/core/String.h>
#include <y/utils/log.h>

#include <mutex>

namespace editor {

class Logs {
	public:
		struct Message {
			const core::String msg;
			const Log type;
		};

		void clear() {
			const std::unique_lock lock(_lock);
			_msgs.clear();
		}

		void log_msg(core::String msg, Log type = Log::Info) {
			log_msg(Message{std::move(msg), type});
		}

		void log_msg(Message msg) {
			const std::unique_lock lock(_lock);
			_msgs.emplace_back(std::move(msg));
		}

		/*core::Span<Message> messages() const {
			return _msgs;
		}*/

		template<typename F>
		void for_each(F&& func) const {
			const std::unique_lock lock(_lock);
			for(const Message& m : _msgs) {
				func(m);
			}
		}

	private:
		mutable std::mutex _lock;
		core::Vector<Message> _msgs;
};

}

#endif // EDITOR_CONTEXT_LOGS_H
