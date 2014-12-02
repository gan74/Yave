#include "DynamicPrimitive.h"
#include "Machine.h"

namespace n {
namespace script {

DynamicPrimitive::DynamicPrimitive() : DynamicPrimitive(PrimitiveType(), Primitive()) {
}

DynamicPrimitive::DynamicPrimitive(const DynamicPrimitive &t) : DynamicPrimitive(t.type(), t.data()) {
}

DynamicPrimitive::DynamicPrimitive(PrimitiveType t, Primitive value) {
	new(raw) PrimitiveType(t);
	new(raw + 1) Primitive(value);
}

bool DynamicPrimitive::operator==(const DynamicPrimitive &o) const {
	if(type() == o.type()) {

	}
	uint32 ct = type().coerce(o.type());
	if(ct == type()) {
		return equals(o.toType(ct));
	} else if(ct == o.type()) {
		return o.equals(toType(ct));
	}
	Machine::error();
	return false;
}

bool DynamicPrimitive::operator!=(const DynamicPrimitive &o) const {
	return !operator==(o);
}

Primitive DynamicPrimitive::toType(PrimitiveType t) const {
	if(t == type()) {
		return data();
	}
	switch(t) {
		case PrimitiveType::Int:
			switch(type()) {
				case PrimitiveType::Float:
					return data().Float;
				case PrimitiveType::String:
					return data().to<core::String>().to<int>([=]() {
						Machine::typeError(type(), t);
					});
				default:
					Machine::typeError(type(), t);
			}
		case PrimitiveType::Float:
			switch(type()) {
				case PrimitiveType::Int:
					return data().Int;
				case PrimitiveType::String:
					return data().to<core::String>().to<float>([=]() {
						Machine::typeError(type(), t);
					});
				default:
					Machine::typeError(type(), t);
			}
		case PrimitiveType::String:
			switch(type()) {
				case PrimitiveType::Int:
					return new Primitive(new core::String(data().Int));
				case PrimitiveType::Float:
					return new Primitive(new core::String(data().Float));
				default:
					Machine::typeError(type(), t);
			}

		case PrimitiveType::Array: {
			core::Array<DynamicPrimitive> *arr = new core::Array<DynamicPrimitive>();
			arr->append(*this);
			return Primitive(arr);
		}
		default:
			Machine::typeError(type(), t);
	}

	return Machine::error();
}

bool DynamicPrimitive::equals(const Primitive &o) const {
	switch(type()) {
		case PrimitiveType::Int:
		case PrimitiveType::Float:
			return data().Int == o.Int;
		case PrimitiveType::String:
			return data().to<core::String>() == o.to<core::String>();
		case PrimitiveType::Array:
			return data().to<core::Array<DynamicPrimitive>>() == o.to<core::Array<DynamicPrimitive>>();
		default:
			Machine::typeError(type());
	}
	return false;
}

}
}
