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
#ifndef YAVE_ECS_ENTITY_H
#define YAVE_ECS_ENTITY_H

#include "ecs.h"

#include <typeindex>
#include <bitset>

namespace yave {
namespace ecs {

class Entity final {
	public:
		Entity() = default;

		const auto& components() const {
			return _components;
		}

	private:
		friend EntityWorld;

		usize component_index(TypeIndex type) const;

		bool has_component(TypeIndex type) const;
		ComponentId component_id(TypeIndex type) const;
		void add_component(TypeIndex type, ComponentId id);
		void remove_component(TypeIndex type);


		std::bitset<max_entity_component_types> _component_type_bits;
		core::Vector<ComponentId> _components;
};

}
}

#endif // YAVE_ECS_ENTITY_H
