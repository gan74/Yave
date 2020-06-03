/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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
#ifndef Y_ECS_ARCHETYPE_H
#define Y_ECS_ARCHETYPE_H

#include "ecs.h"
#include "ComponentRuntimeInfo.h"

#include <y/core/HashMap.h>

namespace y {
namespace ecs {

class Archetype {
	public:
		template<typename T>
		void add_component_type() {
			if(_infos.find(type_index<T>()) == _infos.end()) {
				_infos[type_index<T>()] = ComponentRuntimeInfo::create<T>();
			}
		}

		auto component_infos() const {
			return _infos.values();
		}

	private:
		core::ExternalHashMap<u32, ComponentRuntimeInfo> _infos;
};

}
}

#endif // Y_ECS_ARCHETYPE_H
