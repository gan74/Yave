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

Animation::Animation(float duration, core::Vector<AnimationChannel>&& channels) : _duration(duration), _channels(std::move(channels)) {
}

const core::Vector<AnimationChannel>& Animation::channels() const {
	return _channels;
}

float Animation::duration() const {
	return _duration;
}

std::optional<math::Transform<>> Animation::bone_transform(const core::String& name, float time) const {
	auto channel = std::find_if(_channels.begin(), _channels.end(), [&](const auto& ch) { return ch.name() == name; });

	if(channel == _channels.end()) {
		return std::optional<math::Transform<>>();
	}

	return std::optional(channel->bone_transform(time));
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
				   version == 3 &&
				   channels > 0 &&
				   duration > 0.0f;
		}
	};

	Header header = reader->read_one<Header>().expected(err_msg);
	if(!header.is_valid()) {
		fatal(err_msg);
	}

	auto channels = core::vector_with_capacity<AnimationChannel>(header.channels);
	for(usize i = 0; i != header.channels; ++i) {
		u32 name_len = reader->read_one<u32>().expected(err_msg);
		core::String name(nullptr, name_len);
		reader->read(name.data(), name.size()).expected(err_msg);
		name[name.size()] = 0;

		u32 key_count = reader->read_one<u32>().expected(err_msg);
		core::Vector<AnimationChannel::BoneKey> keys(key_count, AnimationChannel::BoneKey{});
		reader->read(keys.begin(), keys.size() * sizeof(AnimationChannel::BoneKey)).expected(err_msg);

		channels << AnimationChannel(name, std::move(keys));
	}

	return Animation(header.duration, std::move(channels));
}

void Animation::to_file(io::WriterRef writer) const {
	const char* err_msg = "Unable to write animation.";

	u32 magic = 0x65766179;
	u32 type = 3;
	u32 version = 3;

	u32 channels = _channels.size();
	float duration = _duration;

	writer->write_one(magic).expected(err_msg);
	writer->write_one(type).expected(err_msg);
	writer->write_one(version).expected(err_msg);

	writer->write_one(channels).expected(err_msg);
	writer->write_one(duration).expected(err_msg);


	for(const auto& ch : _channels) {
		writer->write_one(u32(ch.name().size())).expected(err_msg);
		writer->write(ch.name().begin(), ch.name().size()).expected(err_msg);

		writer->write_one(u32(ch.keys().size())).expected(err_msg);
		writer->write(ch.keys().begin(), ch.keys().size() * sizeof(AnimationChannel::BoneKey)).expected(err_msg);
	}
}

}
