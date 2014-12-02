#ifndef N_SCRIPT_PRIMITIVE_H
#define N_SCRIPT_PRIMITIVE_H

namespace n {
namespace script {

union Primitive
{
	Primitive() : Primitive(0) {
	}

	Primitive(int i) : Int(i) {
	}

	Primitive(float f) : Float(f) {
	}

	template<typename T>
	Primitive(T *p) : ptr(p) {
	}

	template<typename T>
	T &to() {
		return *(T *)ptr;
	}

	template<typename T>
	const T &to() const {
		return *(T *)ptr;
	}

	template<typename T>
	operator T() {
		return *(T *)ptr;
	}

	template<typename T>
	operator const T() const {
		return *(T *)ptr;
	}

	int Int;
	float Float;
	void *ptr;
};

}
}

#endif // N_SCRIPT_PRIMITIVE_H
