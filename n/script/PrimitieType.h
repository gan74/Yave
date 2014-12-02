#ifndef N_SCRIPT_PRIMITIETYPE_H
#define N_SCRIPT_PRIMITIETYPE_H

#include <n/types.h>

namespace n {
namespace script {

class PrimitiveType
{
	public:
		enum Type
		{
			None,
			Int,
			Float,
			String,
			Object,
			Array,

			Bytecode,

			Other
		};

		PrimitiveType(Type t = None) : type(t) {
		}

		PrimitiveType(uint16 t) : type(t) {
		}

		PrimitiveType(const PrimitiveType &c) : type(c.type) {
		}

		operator Type() const {
			return Type(type);
		}


		PrimitiveType coerce(const PrimitiveType &c) const {
			PrimitiveType pp(std::max(type, c.type));
			if(pp.isObject()) {
				return PrimitiveType();
			}
			return pp;
		}

		bool isObject() const {
			return type == Object || type >= Other;
		}

		const char *name() const {
			switch(type) {
				case None:
					return "None";
				case Int:
					return "Int";
				case Float:
					return "Float";
				case String:
					return "String";
				case Object:
					return "Object";
				case Array:
					return "Array";
				default:
					return "Any";
			}
		}

	private:
		uint16 type;

};

}
}


#endif // N_SCRIPT_PRIMITIETYPE_H
