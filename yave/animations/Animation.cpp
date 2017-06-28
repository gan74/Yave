/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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

#include "Animation.h"

namespace yave {

const core::Vector<AnimationChannel>& Animation::channels() const {
	return _channels;
}

float Animation::duration() const {
	return _duration;
}

static core::Result<AnimationChannel> read_channel(io::ReaderRef reader) {
	auto len_res = reader->read_one<u32>();
	if(len_res.is_error()) {
		return core::Err();
	}

	core::String name(nullptr, len_res.unwrap());
	auto name_res = reader->read(name.data(), name.size());
	name[name.size()] = 0;

	if(name_res.is_error()) {
		return core::Err();
	}

	auto key_len_res = reader->read_one<u32>();
	if(key_len_res.is_error()) {
		return core::Err();
	}

	core::Vector<BoneKey> keys(key_len_res.unwrap(), BoneKey{});
	auto key_res = reader->read(keys.begin(), keys.size() * sizeof(BoneKey));
	if(key_res.is_error()) {
		return core::Err();
	}

	return core::Ok(AnimationChannel{std::move(name), std::move(keys)});
}

Animation Animation::from_file(io::ReaderRef reader) {
	const char* err_msg = "Unable to read animation.";

	struct Header {
		u32 magic;
		u32 type;
		u32 version;

		u32 channels;
		float duration;

		bool is_valid() const {
			return magic == 0x65766179 &&
				   type == 3 &&
				   version == 1 &&
				   channels > 0 &&
				   duration > 0.0f;
		}
	};

	Header header = reader->read_one<Header>().expected(err_msg);
	if(!header.is_valid()) {
		fatal(err_msg);
	}

	Animation anim;
	anim._duration = header.duration;
	for(usize i = 0; i != header.channels; ++i) {
		anim._channels << read_channel(reader).expected(err_msg);
	}

	sort(anim._channels.begin(), anim._channels.end(), [](const auto& a, const auto& b) { return a.name < b.name; });

	for(const auto& b : anim._channels.first().keys) {
		log_msg(core::str(b.time));
	}

	return anim;
}

}
