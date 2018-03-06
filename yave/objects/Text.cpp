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

#include "Text.h"
#include <yave/device/Device.h>
#include <yave/buffers/TypedMapping.h>

namespace yave {

Text::Text(const AssetPtr<Font>& font, const core::String& text) :
		_font(font),
		_text(text),
		_buffer(_font->device(), _text.size()) {

	auto dptr = _font->device();
	CmdBufferRecorder recorder = dptr->create_disposable_cmd_buffer();
	auto map = TypedMapping(_buffer, recorder);
	dptr->queue(QueueFamily::Graphics).submit<SyncSubmit>(RecordedCmdBuffer(std::move(recorder)));

	for(usize i = 0; i != _text.size(); ++i) {
		map[i] = _font->char_data(_text[i]);
	}
}

void Text::render(const FrameToken&, RenderPassRecorder&, const SceneData&) const {
	fatal("text");
}

}

