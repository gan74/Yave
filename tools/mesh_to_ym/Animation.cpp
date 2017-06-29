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

#include <unordered_map>

#include <iostream>


const String& Animation::name() const {
	return _name;
}

/*static void debug_node(aiNode* node, usize tabs = 0) {
	core::String indent;
	for(usize i = 0; i != tabs; ++i) {
		indent += "  ";
	}

	log_msg(indent + node->mName.C_Str(), Log::Debug);

	if(node->mParent) {
		debug_node(node->mParent, tabs + 1);
	}
}*/

/*static aiNode* common_parent(aiNode* a, aiNode* b) {
	for(; a; a = a->mParent) {
		for(auto bn = b; bn; bn = bn->mParent) {
			if(a == bn) {
				return a;
			}
		}
	}
	return nullptr;
}*/



Result<AnimationChannel> AnimationChannel::from_assimp(aiNodeAnim* anim) {

	static_assert(sizeof(aiVector3D) == sizeof(math::Vec3), "aiVector3 should be 3 floats");
	static_assert(sizeof(aiQuaternion) == sizeof(math::Vec4), "aiQuaterniont should be 4 floats");

	struct Pose {
		aiVector3D position = aiVector3D(0, 0, 0);
		aiVector3D scaling = aiVector3D(1, 1, 1);
		aiQuaternion quaternion = aiQuaternion(0, 0, 0);
	};

	std::unordered_map<float, Pose> keys;
	for(usize i = 0; i != anim->mNumPositionKeys; ++i) {
		aiVectorKey key = anim->mPositionKeys[i];
		keys[key.mTime].position = key.mValue;
	}
	for(usize i = 0; i != anim->mNumScalingKeys; ++i) {
		aiVectorKey key = anim->mScalingKeys[i];
		keys[key.mTime].scaling = key.mValue;
	}
	for(usize i = 0; i != anim->mNumRotationKeys; ++i) {
		aiQuatKey key = anim->mRotationKeys[i];
		keys[key.mTime].quaternion = key.mValue;
	}



	if(anim->mNumPositionKeys != keys.size()) {
		log_msg("Invalid positions.", Log::Error);
		return Err();
	}
	if(anim->mNumRotationKeys != keys.size()) {
		log_msg("Invalid rotations.", Log::Error);
		return Err();
	}
	if(anim->mNumScalingKeys != keys.size()) {
		log_msg("Invalid scales.", Log::Error);
		return Err();
	}
	log_msg("channel for " + str(anim->mNodeName.C_Str()) + " has " +  str(keys.size()) + " keys");


	AnimationChannel channel;
	channel._name = anim->mNodeName.C_Str();
	std::transform(keys.begin(), keys.end(), std::back_inserter(channel._keys), [](const auto& k) {
			aiMatrix4x4 tr(k.second.scaling, k.second.quaternion, k.second.position);
			return Key{k.first, reinterpret_cast<const math::Transform<>&>(tr).transposed()};
		});
	sort(channel._keys.begin(), channel._keys.end(), [](const auto& a, const auto& b) { return a.time < b.time; });

	return Ok(std::move(channel));
}

io::Writer::Result AnimationChannel::write(io::WriterRef writer) const {
	writer->write_one(u32(_name.size()));
	writer->write(_name.data(), _name.size());

	writer->write_one(u32(_keys.size()));
	writer->write(_keys.begin(), _keys.size() * sizeof(Key));

	return Ok();
}

Result<Animation> Animation::from_assimp(aiAnimation* anim) {
	if(!anim || anim->mDuration <= 0.0f || !anim->mNumChannels) {
		return Err();
	}

	Animation an;
	an._duration = anim->mDuration;

	/*aiNode* root_node = scene->mRootNode->FindNode(anim->mChannels[0]->mNodeName);
	for(usize i = 1; i != anim->mNumChannels; ++i) {
		root_node = common_parent(scene->mRootNode->FindNode(anim->mChannels[i]->mNodeName), root_node);
	}
	if(!root_node) {
		log_msg("Unable to root animation.", Log::Error);
		return Err();
	}
	log_msg("Animation root: " + str(root_node->mName.C_Str()));*/


	for(usize i = 0; i != anim->mNumChannels; ++i) {
		auto res = AnimationChannel::from_assimp(anim->mChannels[i]);
		if(res.is_error()) {
			return Err();
		}
		an._channels << res.unwrap();
	}

	return Ok(std::move(an));
}

io::Writer::Result Animation::write(io::WriterRef writer) const {
	u32 magic = 0x65766179;
	u32 type = 3;
	u32 version = 2;

	writer->write_one(magic);
	writer->write_one(type);
	writer->write_one(version);

	writer->write_one(u32(_channels.size()));
	writer->write_one(_duration);

	for(const auto& channel : _channels) {
		channel.write(writer);
	}

	return Ok();
}

