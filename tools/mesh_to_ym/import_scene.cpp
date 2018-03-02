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

#include "import.h"


namespace import {

static constexpr auto import_flags =
									 aiProcess_Triangulate |
									 aiProcess_FindInvalidData |
									 aiProcess_GenSmoothNormals |
									 aiProcess_CalcTangentSpace |
									 aiProcess_GenUVCoords |
									 aiProcess_ImproveCacheLocality |
									 aiProcess_OptimizeGraph |
									 aiProcess_OptimizeMeshes |
									 aiProcess_JoinIdenticalVertices |
									 aiProcess_ValidateDataStructure |
									 0;

SceneData import_scene(const core::String& path) {
	DebugTimer _("import_scene");

	Assimp::Importer importer;
	auto scene = importer.ReadFile(path, import_flags);

	if(!scene) {
		fatal("Unable to load scene.");
	}

	if(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
		log_msg("Scene is incomplete!", Log::Warning);
	}

	auto meshes = Range(scene->mMeshes, scene->mMeshes + scene->mNumMeshes);
	auto animations = Range(scene->mAnimations, scene->mAnimations + scene->mNumAnimations);

	SceneData data;
	std::transform(meshes.begin(), meshes.end(), std::back_inserter(data.meshes), [=](aiMesh* mesh) { return import_mesh(mesh, scene); });
	std::transform(animations.begin(), animations.end(), std::back_inserter(data.animations), [=](aiAnimation* anim) { return import_animation(anim); });

	return data;
}

}

