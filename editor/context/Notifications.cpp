/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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

#include "Notifications.h"

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace editor {

void Notifications::Tqdm::update(u32 id, usize value) {
	std::unique_lock lock(_lock);

	if(id < _first_id || id - _first_id >= _progress.size()) {
		return;
	}
	Data& data = _progress[id - _first_id];
	data.it = value;

	if(id == _first_id) {
		usize remove = 0;
		while(remove < _progress.size() && _progress[remove].is_over()) {
			++remove;
		}
		if(remove) {
			_progress.assign(_progress.begin() + remove, _progress.end());
		}
		_first_id += remove;
	}
}

u32 Notifications::Tqdm::create(usize size, core::String msg) {
	if(!size) {
		return u32(-1);
	}

	std::unique_lock lock(_lock);
	const u32 id = _progress.size();
	_progress.emplace_back(Data{0, size, std::move(msg)});
	return id;
}

core::Vector<Notifications::Tqdm::Data> Notifications::Tqdm::progress_items() const {
	std::unique_lock lock(_lock);
	return _progress;
}

Notifications::Notifications(ContextPtr ctx) : ContextLinked(ctx), _tqdm(std::make_shared<Tqdm>()) {
}


}
