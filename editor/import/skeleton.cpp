/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include "import.h"

#include <unordered_map>

#ifndef EDITOR_NO_ASSIMP

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace editor {
namespace import {

struct BoneRef {
	u32 index;
	float weight;

	bool operator<(const BoneRef& other) const {
		return std::tie(weight, index) > std::tie(other.weight, other.index);
	}
};

static aiNode* common_parent(aiNode* a, aiNode* b) {
	if(!a) {
		return b;
	}
	for(; a; a = a->mParent) {
		for(aiNode* bn = b; bn; bn = bn->mParent) {
			if(a == bn) {
				return a;
			}
		}
	}
	return nullptr;
}

static void add_bone_refs(aiBone* bone, u32 index, core::Vector<core::Vector<BoneRef>>& refs) {
	for(usize i = 0; i != bone->mNumWeights; ++i) {
		auto w = bone->mWeights[i];
		refs[w.mVertexId] << BoneRef{index, w.mWeight};
	}
}

static SkinWeights compute_skin(core::Vector<BoneRef>& refs) {
	sort(refs.begin(), refs.end());
	SkinWeights skin;

	usize max = std::min(refs.size(), skin.size);

	float total = 0.0f;
	for(usize i = 0; i != max; ++i) {
		total += refs[i].weight;
	}

	for(usize i = 0; i != max; ++i) {
		skin.indexes[i] = refs[i].index;
		skin.weights[i] = refs[i].weight / total;
	}

	return skin;
}

SkeletonData import_skeleton(aiMesh* mesh, const aiScene* scene) {
	if(!mesh || !mesh->mNumBones) {
		y_throw("Empty skeleton.");
	}

	auto mesh_bones = core::ArrayView<aiBone*>(mesh->mBones, mesh->mNumBones);

	core::Vector<aiNode*> bone_nodes;
	bone_nodes << nullptr;
	for(aiBone* bone : mesh_bones) {
		aiNode* node = scene->mRootNode->FindNode(bone->mName);
		bone_nodes[0] = common_parent(bone_nodes[0], node->mParent);
		bone_nodes << node;
	}
	log_msg(fmt("Skeleton has % bones.", bone_nodes.size()));


	std::unordered_map<std::string, std::pair<usize, aiNode*>> bone_map;
	for(aiNode* node : bone_nodes) {
		usize index = bone_map.size();
		bone_map[node->mName.C_Str()] = {index, node};
	}
	if(bone_map.size() != bone_nodes.size()) {
		y_throw("Bones are duplicated.");
	}



	core::Vector<Bone> bones;
	for(usize index = 0; index != bone_nodes.size(); ++index) {
		aiNode* node = bone_nodes[index];

		u32 parent_index = u32(-1);
		if(auto parent = bone_map.find(node->mParent->mName.C_Str()); parent != bone_map.end()) {
			parent_index = u32(parent->second.first);
			if(parent_index >= index) {
				y_throw("Parent serialized after children.");
			}
		}

		aiVector3D position;
		aiQuaternion rotation;
		aiVector3D scale;
		node->mTransformation.Decompose(scale, rotation, position);

		bones << Bone {
				core::String(node->mName.C_Str()),
				parent_index,
				BoneTransform {
					{position.x, position.y, position.z},
					{scale.x, scale.y, scale.z},
					{rotation.x, rotation.y, rotation.z, rotation.w}
				}
			};

	}


	core::Vector<core::Vector<BoneRef>> bone_per_vertex(mesh->mNumVertices, core::Vector<BoneRef>());
	for(usize i = 0; i != mesh->mNumBones; ++i) {
		aiBone* bone = mesh->mBones[i];
		usize index = bone_map[std::string(bone->mName.C_Str())].first;
		add_bone_refs(bone, index, bone_per_vertex);
	}

	auto skin_points = core::vector_with_capacity<SkinWeights>(mesh->mNumVertices);
	std::transform(bone_per_vertex.begin(), bone_per_vertex.end(), std::back_inserter(skin_points), [](auto& bns) { return compute_skin(bns); });


	return SkeletonData{std::move(skin_points), std::move(bones)};
}

}
}

#endif



