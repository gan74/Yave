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
#ifndef SCENE_H
#define SCENE_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "utils.h"

class Scene : NonCopyable {
	public:
		static constexpr auto import_flags = aiProcess_Triangulate |
											 aiProcess_GenSmoothNormals |
											 aiProcess_CalcTangentSpace |
											 aiProcess_GenUVCoords |
											 aiProcess_ImproveCacheLocality |
											 aiProcess_OptimizeGraph |
											 aiProcess_OptimizeMeshes |
											 aiProcess_JoinIdenticalVertices;

		static Result<Scene> from_file(const core::String& path);


		Scene(Scene&& other);


		auto meshes() const {
			return Range(_scene->mMeshes, _scene->mMeshes + _scene->mNumMeshes);
		}

	private:
		Scene() = default;

		Unique<Assimp::Importer> _importer;
		NotOwner<const aiScene*> _scene = nullptr;
};

#endif // SCENE_H
