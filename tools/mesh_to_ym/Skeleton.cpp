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

#include <unordered_map>
#include <optional>

struct BoneRef {
	u32 index;
	float weight;

	bool operator<(const BoneRef& other) const {
		return std::tie(weight, index) > std::tie(other.weight, other.index);
	}
};

static void add_bone_refs(aiBone* bone, u32 index, Vector<Vector<BoneRef>>& refs) {
	for(usize i = 0; i != bone->mNumWeights; ++i) {
		auto w = bone->mWeights[i];
		refs[w.mVertexId] << BoneRef{index, w.mWeight};
	}
}

static SkinWeights compute_skin(Vector<BoneRef>& refs) {
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


const Vector<Bone>& Skeleton::bones() const {
	return _bones;
}

const Vector<SkinWeights>& Skeleton::skin() const {
	return _skin;
}


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

Result<Skeleton> Skeleton::from_assimp(aiMesh* mesh, const aiScene* scene) {
	if(!mesh || !mesh->mNumBones) {
		return Err();
	}


	Range mesh_bones(mesh->mBones, mesh->mBones + mesh->mNumBones);


	Vector<aiNode*> bone_nodes;
	bone_nodes << nullptr;
	for(aiBone* bone : mesh_bones) {
		aiNode* node = scene->mRootNode->FindNode(bone->mName);
		bone_nodes[0] = common_parent(bone_nodes[0], node->mParent);
		bone_nodes << node;
	}
	log_msg("Skeleton has " + str(bone_nodes.size()) + " bones.");


	std::unordered_map<std::string, std::pair<usize, aiNode*>> bone_map;
	for(aiNode* node : bone_nodes) {
		usize index = bone_map.size();
		bone_map[node->mName.C_Str()] = {index, node};
	}
	if(bone_map.size() != bone_nodes.size()) {
		log_msg("Bones are duplicated.", Log::Error);
		return Err();
	}



	Vector<Bone> bones;
	for(usize index = 0; index != bone_nodes.size(); ++index) {
		aiNode* node = bone_nodes[index];

		u32 parent_index = u32(-1);
		if(auto parent = bone_map.find(node->mParent->mName.C_Str()); parent != bone_map.end()) {
			parent_index = u32(parent->second.first);
			if(parent_index >= index) {
				log_msg("Parent serialized after children.", Log::Error);
				return Err();
			}
		}

		aiVector3D position;
		aiQuaternion rotation;
		aiVector3D scale;
		node->mTransformation.Decompose(scale, rotation, position);

		bones << Bone {
				str(node->mName.C_Str()),
				parent_index,
				BoneTransform {
					{position.x, position.y, position.z},
					{scale.x, scale.y, scale.z},
					{rotation.x, rotation.y, rotation.z, rotation.w}
				}
			};
	}


	Vector<Vector<BoneRef>> bone_per_vertex(mesh->mNumVertices, Vector<BoneRef>());
	for(usize i = 0; i != mesh->mNumBones; ++i) {
		aiBone* bone = mesh->mBones[i];
		usize index = bone_map[std::string(bone->mName.C_Str())].first;
		add_bone_refs(bone, index, bone_per_vertex);
	}


	auto skin_points = vector_with_capacity<SkinWeights>(mesh->mNumVertices);
	std::transform(bone_per_vertex.begin(), bone_per_vertex.end(), std::back_inserter(skin_points), [](auto& bones) { return compute_skin(bones); });


	Skeleton skeleton;
	skeleton._bones = std::move(bones);
	skeleton._skin = std::move(skin_points);
	return Ok(std::move(skeleton));
}

