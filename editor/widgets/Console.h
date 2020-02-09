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
#ifndef EDITOR_WIDGETS_CONSOLE_H
#define EDITOR_WIDGETS_CONSOLE_H

#include <editor/ui/Widget.h>

#include <y/utils/log.h>

namespace editor {

class Console : public Widget, public ContextLinked {
	public:
		Console(ContextPtr cptr);

	private:
		static constexpr usize log_type_count = usize(Log::Perf) + 1;

		void paint_ui(CmdBufferRecorder&, const FrameToken&) override;


		std::array<char, 1024> _filter;
		std::array<bool, log_type_count> _log_types;
		std::array<usize, log_type_count> _log_counts;

		bool _auto_scroll = true;
};

}

#endif // EDITOR_WIDGETS_CONSOLE_H
