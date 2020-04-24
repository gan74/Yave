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
#ifndef Y_ECS_COMPONENTSERIALIZER_H
#define Y_ECS_COMPONENTSERIALIZER_H

#include "ComponentRuntimeInfo.h"

#include <y/serde3/serde.h>
#include <y/serde3/archives.h>
#include <y/serde3/poly.h>


namespace y {
namespace ecs {

struct ComponentInfoSerializerBase : NonMovable {
	virtual ~ComponentInfoSerializerBase() {
	}

	virtual ComponentRuntimeInfo create_runtime_info() const = 0;

	//y_serde3_poly_base(ComponentInfoSerializerBase)
};

template<typename T>
class ComponentInfoSerializer : public ComponentInfoSerializerBase {
	public:
		ComponentInfoSerializer() {
		}

		ComponentRuntimeInfo create_runtime_info() const override {
			return ComponentRuntimeInfo::from_type<T>();
		}

		//y_serde3_poly(ComponentInfoSerializer)
		//y_serde3(u32(17))

	private:
};

class ComponentSerializerBase : NonMovable {
	public:
		virtual ~ComponentSerializerBase() {
		}

		//y_serde3_poly_base(ComponentSerializerBase)

	protected:
		ComponentSerializerBase(Archetype* arc) : _archetype(arc) {
		}

		Archetype* _archetype = nullptr;
};

template<typename T>
class ComponentSerializer : public ComponentSerializerBase {
	public:
		ComponentSerializer(Archetype* arc) : ComponentSerializerBase(arc) {
		}

		/*y_serde3_poly(ComponentSerializer)
		y_serde3(test())*/

	private:
		u32 test() {
			log_msg("test");
			return 17;
		}
};

class ComponentSerializerWrapper {
	public:
		ComponentSerializerWrapper(ComponentSerializerWrapper&&) = default;

		template<typename T>
		static ComponentSerializerWrapper create(Archetype* arc) {
			return ComponentSerializerWrapper(std::make_unique<ComponentSerializer<T>>(arc));
		}

		y_serde3(_serializer)

	private:
		ComponentSerializerWrapper(std::unique_ptr<ComponentSerializerBase> ptr) : _storage(std::move(ptr)), _serializer(*_storage.get()) {
		}

		std::unique_ptr<ComponentSerializerBase> _storage;
		std::reference_wrapper<ComponentSerializerBase> _serializer;
};


class ComponentListSerializer {
	public:
		ComponentListSerializer() = default;

		void add(ComponentSerializerWrapper wrapper) {
			_wrappers.emplace_back(std::move(wrapper));
		}

		y_serde3(wrappers())

	private:
		core::Range<core::Vector<ComponentSerializerWrapper>::iterator> wrappers() {
			return _wrappers;
		}

		core::Range<core::Vector<ComponentSerializerWrapper>::const_iterator> wrappers() const {
			return _wrappers;
		}

		core::Vector<ComponentSerializerWrapper> _wrappers;
};


namespace detail {
template<typename T>
std::unique_ptr<ComponentInfoSerializerBase> create_info_serializer() {
	return std::make_unique<ComponentInfoSerializer<T>>();
}

template<typename T>
ComponentSerializerWrapper create_component_serializer(Archetype* arc) {
	return ComponentSerializerWrapper::create<T>(arc);
}

}

}
}

#endif // Y_ECS_COMPONENTSERIALIZER_H
