/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#ifndef N_SCRIPT_PRIMITIETYPE_H
#define N_SCRIPT_PRIMITIETYPE_H

#include <n/types.h>
#include <n/defines.h>
#ifndef N_NO_SCRIPT

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

#endif


#endif // N_SCRIPT_PRIMITIETYPE_H
