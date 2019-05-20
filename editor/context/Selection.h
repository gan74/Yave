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
#ifndef EDITOR_CONTEXT_SELECTION_H
#define EDITOR_CONTEXT_SELECTION_H

#include <yave/objects/Light.h>
#include <yave/objects/Renderable.h>

#include <yave/material/Material.h>
#include <yave/ecs/EntityIdPool.h>

namespace editor {

class Selection {
	public:
		void flush_reload() {
			_material.flush_reload();
			if(_renderable) {
				_renderable->flush_reload();
			}
		}



		template<typename T, typename = std::enable_if_t<std::is_polymorphic_v<T>>>
		void set_selected(T* sel) {
			_light = nullptr;
			_renderable = dynamic_cast<Renderable*>(sel);
			_transformable = dynamic_cast<Transformable*>(sel);
		}


		void set_selected(Transformable* sel) {
			_light = nullptr;
			_renderable = nullptr;
			_transformable = sel;
		}

		void set_selected(Light* sel) {
			_light = sel;
			_renderable = nullptr;
			_transformable = sel;
		}

		void set_selected(std::nullptr_t) {
			_light = nullptr;
			_renderable = nullptr;
			_transformable = nullptr;
		}



		Transformable* transformable() const {
			return _transformable;
		}

		Renderable* renderable() const {
			return _renderable;
		}

		Light* light() const {
			return _light;
		}




		bool has_selected() {
			return _light || _renderable || _transformable;
		}




		void set_selected(const AssetPtr<Material>& sel) {
			_material = sel;
		}

		const auto& material() const {
			return _material;
		}


		bool has_selected_entity() const {
			return _id.is_valid();
		}

		ecs::EntityId selected_entity() const {
			return _id;
		}

		void set_selected(ecs::EntityId id) {
			_id = id;
		}

	private:
		NotOwner<Transformable*> _transformable = nullptr;
		NotOwner<Renderable*> _renderable = nullptr;
		NotOwner<Light*> _light = nullptr;
		AssetPtr<Material> _material;
		ecs::EntityId _id;
};

}

#endif // EDITOR_CONTEXT_SELECTION_H
