#ifndef N_CORE_MACHINE_H
#define N_CORE_MACHINE_H

#include "DynamicBytecode.h"
#include <n/core/String.h>

namespace n {
namespace script {

class Machine
{
	public:
		struct RuntimeException : public std::exception
		{
			RuntimeException(const core::String &err) : msg(err) {
			}

			const char *what() const noexcept override {
				return msg.toChar();
			}

			const core::String msg;
		};

		static Nothing typeError(PrimitiveType from, PrimitiveType to) {
			throw RuntimeException(core::String("Conversion from ") + from.name() + " to " + to.name() + " impossible.");
		}

		static Nothing typeError(PrimitiveType from) {
			throw RuntimeException(core::String("Conversion from ") + from.name() + " impossible.");
		}

		static Nothing error() {
			throw RuntimeException("Unknown error");
		}

		static DynamicPrimitive run(DynamicBytecode *code);
	private:
};

}
}

#endif // N_CORE_MACHINE_H
