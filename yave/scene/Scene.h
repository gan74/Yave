/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
#ifndef YAVE_SCENE_SCENE_H
#define YAVE_SCENE_SCENE_H

#include <yave/yave.h>

#include <yave/objects/StaticMeshInstance.h>
#include <yave/objects/Renderable.h>
#include <yave/objects/Light.h>

#include <yave/assets/AssetLoader.h>

namespace yave {

// Not used right now
// At some point this should be filled with data from EntityWorld for rendering
class Scene : NonCopyable {

	public:
		template<typename T>
		using Ptr = std::unique_ptr<T>;

		Scene() = default;

		Scene(core::Vector<Ptr<StaticMeshInstance>>&& meshes, core::Vector<Ptr<Renderable>>&& renderables = {}, core::Vector<Ptr<Light>>&& lights = {});

		void flush_reload();

		const auto& static_meshes() const {
			return _statics;
		}

		const auto& renderables() const {
			return _renderables;
		}

		const auto& lights() const {
			return _lights;
		}

		auto& static_meshes() {
			return _statics;
		}

		auto& renderables() {
			return _renderables;
		}

		auto& lights() {
			return _lights;
		}

	private:
		core::Vector<Ptr<StaticMeshInstance>> _statics;
		core::Vector<Ptr<Renderable>> _renderables;
		core::Vector<Ptr<Light>> _lights;
};


}

#endif // YAVE_SCENE_SCENE_H
