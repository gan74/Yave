#ifndef N_SCRIPT_DYNAMIC_PRIMITIVE_H
#define N_SCRIPT_DYNAMIC_PRIMITIVE_H

#include "PrimitieType.h"
#include "Primitive.h"

namespace n {
namespace script {

class DynamicPrimitive
{

	public:
		DynamicPrimitive();
		DynamicPrimitive(const DynamicPrimitive &t);
		DynamicPrimitive(PrimitiveType t, Primitive value = Primitive());

		bool operator==(const DynamicPrimitive &o) const;
		bool operator!=(const DynamicPrimitive &o) const;

		Primitive toType(PrimitiveType t) const;

		PrimitiveType &type() {
			return *(PrimitiveType *)raw;
		}

		Primitive &data() {
			return *(Primitive *)(raw + 1);
		}

		const PrimitiveType &type() const {
			return *(PrimitiveType *)raw;
		}

		const Primitive &data() const {
			return *(Primitive *)(raw + 1);
		}

	private:
		uint16 raw[3];

		bool equals(const Primitive &o) const;
};

}
}


#endif // N_SCRIPT_DYNAMIC_PRIMITIVE_H
