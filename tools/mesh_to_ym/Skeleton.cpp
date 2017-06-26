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

struct BoneRef {
	u32 index;
	float weight;

	bool operator<(const BoneRef& other) const {
		return weight < other.weight;
	}
};

static void add_bone_refs(aiBone* bone, u32 index, Vector<Vector<BoneRef>>& refs) {
	for(usize i = 0; i != bone->mNumWeights; ++i) {
		auto w = bone->mWeights[i];
		refs[w.mVertexId] << BoneRef{index, w.mWeight};
	}
}

static SkinWeights compute_skin(Vector<BoneRef>& refs) {
	std::sort(refs.begin(), refs.end());
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

Result<Skeleton> Skeleton::from_assimp(aiMesh* mesh) {
	if(!mesh->mNumBones) {
		return Err();
	}

	Vector<Vector<BoneRef>> bone_per_vertex(mesh->mNumVertices, Vector<BoneRef>());
	auto bones = vector_with_capacity<Bone>(mesh->mNumBones);

	for(usize i = 0; i != mesh->mNumBones; ++i) {
		auto bone = mesh->mBones[i];
		add_bone_refs(bone, bones.size(), bone_per_vertex);

		bones << Bone {
				str(bone->mName.C_Str()),
				{bone->mOffsetMatrix[0][0], bone->mOffsetMatrix[0][1], bone->mOffsetMatrix[0][2]},
				{bone->mOffsetMatrix[1][0], bone->mOffsetMatrix[1][1], bone->mOffsetMatrix[1][2]},
				{bone->mOffsetMatrix[2][0], bone->mOffsetMatrix[2][1], bone->mOffsetMatrix[2][2]},
				{bone->mOffsetMatrix[3][0], bone->mOffsetMatrix[3][1], bone->mOffsetMatrix[3][2]},
			};
	}

	auto skin_points = vector_with_capacity<SkinWeights>(mesh->mNumVertices);
	std::transform(bone_per_vertex.begin(), bone_per_vertex.end(), std::back_inserter(skin_points), [](auto& bones) { return compute_skin(bones); });


	Skeleton skeleton;
	skeleton._bones = std::move(bones);
	skeleton._skin = std::move(skin_points);
	return Ok(std::move(skeleton));
}

