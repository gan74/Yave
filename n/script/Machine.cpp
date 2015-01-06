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

#include "Machine.h"

namespace n {
namespace script {

namespace dynamic {
	N_FORCE_INLINE void swapArgOrder(DynamicPrimitive *stackPtr) {
		if(stackPtr->type() < stackPtr[1].type()) {
			std::swap(*stackPtr, stackPtr[1]);
		}
	}

	N_FORCE_INLINE int toInt(DynamicPrimitive *stackPtr) {
		switch(stackPtr->type()) {
			case PrimitiveType::Int:
				return stackPtr->data().Int;
			case PrimitiveType::Float:
				return stackPtr->data().Float;
			case PrimitiveType::String:
				return int(stackPtr->data().to<core::String>());
			default:
				Machine::typeError(stackPtr->type());
		}
		return 0;
	}

	N_FORCE_INLINE void filter(DynamicPrimitive *stackPtr) {
		swapArgOrder(stackPtr);
		if(stackPtr->type() != PrimitiveType::Array) {
			Machine::typeError(stackPtr->type());
		}
		core::Array<DynamicPrimitive> &array = stackPtr->data().to<core::Array<DynamicPrimitive>>();
		if(stackPtr[1].type() != PrimitiveType::Array) {
			array.filter([=](const DynamicPrimitive &t) {
				return !(t == stackPtr[1]);
			});
		} else {
			core::Array<DynamicPrimitive> &filt = stackPtr[1].data().to<core::Array<DynamicPrimitive>>();
			array.filter([&filt](const DynamicPrimitive &t) {
				return !filt.exists(t);
			});
		}

	}

	N_FORCE_INLINE void add(DynamicPrimitive *&stackPtr) {
		stackPtr--;
		uint32 opType = stackPtr->type().coerce(stackPtr[1].type());
		if(opType == stackPtr->type()) {
			switch(opType) {
				case PrimitiveType::Int:
					stackPtr->data().Int += stackPtr[1].toType(opType).Int;
				break;
				case PrimitiveType::Float:
					stackPtr->data().Float += stackPtr[1].toType(opType).Float;
				break;
				case PrimitiveType::String:
					stackPtr->data().to<core::String>() += stackPtr[1].toType(opType).to<core::String>();
				break;
				case PrimitiveType::Array:
					stackPtr->data().to<core::Array<DynamicPrimitive>>() += stackPtr[1];
				break;
				default:
					Machine::typeError(opType);
			}
		} else if(opType == stackPtr[1].type()) {
			switch(opType) {
				case PrimitiveType::Int:
					stackPtr->data().Int += stackPtr[1].toType(opType).Int;
				break;
				case PrimitiveType::Float:
					stackPtr->data().Float += stackPtr[1].toType(opType).Float;
				break;
				case PrimitiveType::String:
					stackPtr[1].data().to<core::String>() = stackPtr->toType(opType).to<core::String>() + stackPtr[1].data().to<core::String>();
					*stackPtr = stackPtr[1];
				break;
				case PrimitiveType::Array:
					stackPtr[1].data().to<core::Array<DynamicPrimitive>>().insert(*stackPtr, stackPtr[1].data().to<core::Array<DynamicPrimitive>>().begin());
					*stackPtr = stackPtr[1];
				default:
					Machine::typeError(opType);
			}
		} else {
			Machine::error();
		}
	}

	N_FORCE_INLINE void sub(DynamicPrimitive *&stackPtr) {
		stackPtr--;
		uint32 opType = stackPtr->type().coerce(stackPtr[1].type());
		Primitive a = stackPtr->toType(opType);
		Primitive b = stackPtr[1].toType(opType);
		switch(opType) {
			case PrimitiveType::Int:
				stackPtr->data().Int = a.Int - b.Int;
			break;
			case PrimitiveType::Float:
				stackPtr->data().Float = a.Float - b.Float;
			break;
			case PrimitiveType::Array:
				filter(stackPtr);
			break;
			default:
				Machine::typeError(opType);
		}
	}

	N_FORCE_INLINE void mul(DynamicPrimitive *&stackPtr) {
		stackPtr--;
		swapArgOrder(stackPtr);
		uint32 opType = stackPtr->type().coerce(stackPtr[1].type());
		Primitive a = stackPtr->toType(opType);
		Primitive b = stackPtr[1].toType(opType);
		stackPtr->type() = opType;
		switch(opType) {
			case PrimitiveType::Int:
				stackPtr->data().Int = a.Int * b.Int;
			break;
			case PrimitiveType::Float:
				stackPtr->data().Float = a.Float * b.Float;
			break;
			default:
				Machine::typeError(opType);
		}
	}

	N_FORCE_INLINE void div(DynamicPrimitive *&stackPtr) {
		stackPtr--;
		uint32 opType = stackPtr->type().coerce(stackPtr[1].type());
		Primitive a = stackPtr->toType(opType);
		Primitive b = stackPtr[1].toType(opType);
		stackPtr->type() = opType;
		switch(opType) {
			case PrimitiveType::Int:
				stackPtr->data().Int = a.Int / b.Int;
			break;
			case PrimitiveType::Float:
				stackPtr->data().Float = a.Float / b.Float;
			break;
			default:
				Machine::typeError(opType);
		}
	}

	N_FORCE_INLINE void isBool(DynamicPrimitive *stackPtr) {
		switch(stackPtr->type()) {
			case PrimitiveType::Int:
			case PrimitiveType::Float:
			break;
			default:
				Machine::typeError(stackPtr->type());
		}
	}

	N_FORCE_INLINE void toBool(DynamicPrimitive *stackPtr) {
		isBool(stackPtr);
		stackPtr->type() = PrimitiveType::Int;
		stackPtr->data().Int = !!stackPtr->data().Int;
	}

	N_FORCE_INLINE DynamicPrimitive ret(DynamicPrimitive *stack, DynamicPrimitive *stackPtr) {
		DynamicPrimitive rt = *stackPtr;
		delete[] stack;
		return rt;
	}
}


DynamicPrimitive Machine::run(DynamicBytecode *code) {
	DynamicPrimitive *stack = new DynamicPrimitive[65536];
	DynamicPrimitive *stackPtr = stack;


	DynamicBytecode *pc = code;

	core::Array<DynamicPrimitive *> slices;
	DynamicPrimitive *tmpPtr = 0;
	DynamicPrimitive tmp;

	while(true) {
		switch(pc->op) {
			case DynamicBytecode::Nop:
			break;

			case DynamicBytecode::End:
				return  dynamic::ret(stack, stackPtr);
			break;

			case DynamicBytecode::Push:
				stackPtr++;
				*stackPtr = DynamicPrimitive(pc->data);
			break;

			case DynamicBytecode::Pop:
				stackPtr--;
			break;

			case DynamicBytecode::Rot2:
				std::swap(*stackPtr, stackPtr[-1]);
			break;

			case DynamicBytecode::Rot3:
				std::swap(*stackPtr, stackPtr[-2]);
			break;

			case DynamicBytecode::Cpy:
				stackPtr[1] = *stackPtr;
				stackPtr++;
			break;

			case DynamicBytecode::CpyN:
				*stackPtr = stackPtr[-1 - dynamic::toInt(stackPtr)];
			break;

			case DynamicBytecode::Add:
				dynamic::add(stackPtr);
			break;

			case DynamicBytecode::Sub:
				dynamic::sub(stackPtr);
			break;

			case DynamicBytecode::Mul:
				dynamic::mul(stackPtr);
			break;

			case DynamicBytecode::Div:
				dynamic::div(stackPtr);
			break;

			case DynamicBytecode::BitNot:
				dynamic::isBool(stackPtr);
				stackPtr->data().Int = ~stackPtr->data().Int;
			break;

			case DynamicBytecode::Not:
				dynamic::toBool(stackPtr);
				stackPtr->data().Int = !stackPtr->data().Int;
			break;

			case DynamicBytecode::Or:
				dynamic::toBool(stackPtr);
				stackPtr--;
				dynamic::toBool(stackPtr);
				stackPtr->type() = PrimitiveType::Int;
				stackPtr->data().Int |= stackPtr[1].data().Int;
			break;

			case DynamicBytecode::And:
				dynamic::toBool(stackPtr);
				stackPtr--;
				dynamic::toBool(stackPtr);
				stackPtr->type() = PrimitiveType::Int;
				stackPtr->data().Int &= stackPtr[1].data().Int;
			break;

			case DynamicBytecode::Xor:
				dynamic::isBool(stackPtr);
				stackPtr--;
				dynamic::isBool(stackPtr);
				stackPtr->type() = PrimitiveType::Int;
				stackPtr->data().Int ^= stackPtr[1].data().Int;
			break;

			case DynamicBytecode::SlB:
				slices.append(stackPtr);
			break;

			case DynamicBytecode::SlE:
				tmpPtr = slices.last();
				slices.pop();
				if(tmpPtr > stackPtr) {
					tmpPtr = stackPtr;
				}
				tmp.type() = PrimitiveType::Array;
				tmp.data().ptr = new core::Array<DynamicPrimitive>(stackPtr - tmpPtr);
				for(DynamicPrimitive *it = tmpPtr + 1; it != stackPtr + 1; it++) {
					tmp.data().to<core::Array<DynamicPrimitive>>().append(*it);
				}
				*tmpPtr = tmp;
				stackPtr = tmpPtr;
			break;

			case DynamicBytecode::Ex:
				stackPtr++;
				*stackPtr = run((DynamicBytecode *)stackPtr->toType(PrimitiveType::Bytecode));
			break;

			case DynamicBytecode::Cast:
				stackPtr->data() = stackPtr->toType(pc->data.type());
				stackPtr->type() = pc->data.type();
			break;

			case DynamicBytecode::Expect:
				if(stackPtr->type() != pc->data.type()) {
					throw RuntimeException(core::String("Expected ") + pc->data.type().name() + ", got " + stackPtr->type().name());
				}
			break;


			default:
				throw RuntimeException("Unknown operation");
		}
		pc++;
	}

	return dynamic::ret(stack, stackPtr);
}

}
}

#undef MATCH_ERROR
