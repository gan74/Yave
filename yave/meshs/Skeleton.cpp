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

#include "Skeleton.h"

namespace yave {

static void debug_bone(usize index, const core::Vector<Bone>& bones, usize tabs = 0) {
	core::String indent;
	for(usize i = 0; i != tabs; ++i) {
		indent += "  ";
	}

	log_msg(indent + bones[index].name + " (" + index + ")", Log::Debug);

	for(usize i = 0; i != bones.size(); ++i) {
		if(bones[i].parent == index) {
			debug_bone(i, bones, tabs + 1);
		}
	}
}

Skeleton::Skeleton(const core::Vector<Bone>& bones) :
		_bones(bones) {

	if(_bones.size() > max_bones) {
		fatal("Bone count exceeds max_bones.");
	}

	for(usize i = 0; i != _bones.size(); ++i) {
		if(!_bones[i].has_parent()) {
			debug_bone(i, _bones);
		}
	}
}

const core::Vector<Bone>& Skeleton::bones() const {
	return _bones;
}

}
