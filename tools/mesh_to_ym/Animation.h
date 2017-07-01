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
#ifndef ANIMATION_H
#define ANIMATION_H

#include <Skeleton.h>

class AnimationChannel {
	public:
		struct Key {
			float time;
			BoneTransform local_transform;
		};

		static_assert(std::is_trivially_copyable_v<Key>, "Key should be trivially copyable");

		static_assert(sizeof(Key) == sizeof(BoneTransform) + sizeof(float));

		static Result<AnimationChannel> from_assimp(aiNodeAnim* anim);
		io::Writer::Result write(io::WriterRef writer) const;

	private:
		String _name;
		Vector<Key> _keys;
};

class Animation {
	public:
		static Result<Animation> from_assimp(aiAnimation* anim);
		io::Writer::Result write(io::WriterRef writer) const;

		const String& name() const;

	private:
		String _name;
		String _root_name;

		float _duration = 0.0f;

		Vector<AnimationChannel> _channels;
};

#endif // ANIMATION_H
