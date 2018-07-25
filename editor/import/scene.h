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
#ifndef EDITOR_IMPORT_SCENE_H
#define EDITOR_IMPORT_SCENE_H

#include <editor/editor.h>

#include <yave/meshes/MeshData.h>
#include <yave/animations/Animation.h>
#include <yave/animations/AnimationChannel.h>

#include <y/core/Chrono.h>
#include <y/math/math.h>

#ifndef EDITOR_NO_ASSIMP
class aiAnimation;
class aiMesh;
class aiScene;
#endif

namespace editor {
namespace import {

struct SkeletonData {
	core::Vector<SkinWeights> skin;
	core::Vector<Bone> bones;
};

struct SceneData {
	core::Vector<Named<MeshData>> meshes;
	core::Vector<Named<Animation>> animations;
};

SceneData import_scene(const core::String& path);

core::String supported_extensions();

#ifndef EDITOR_NO_ASSIMP
Animation import_animation(aiAnimation* anim);
MeshData import_mesh(aiMesh* mesh, const aiScene* scene);
SkeletonData import_skeleton(aiMesh* mesh, const aiScene* scene);
#endif

}
}

#endif // EDITOR_IMPORT_SCENE_H
