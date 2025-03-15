/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#ifndef YAVE_SCRIPT_HELPERS_H
#define YAVE_SCRIPT_HELPERS_H

#include <yave/yave.h>

#include <y/core/HashMap.h>

#include <angelscript.h>

#include <type_traits>

namespace yave {
namespace as {

using TypeNameMap = core::FlatHashMap<std::string_view, core::String>;

inline void check(int res, const char* err_msg = "AngelScript error") {
    y_always_assert(res >= 0, err_msg);
}

template<typename T>
inline std::string_view type_name(const TypeNameMap& map) {
    static_assert(!std::is_reference_v<T>);
    static_assert(!std::is_const_v<T>);
    if constexpr(std::is_compound_v<T>) {
        if(const auto it = map.find(ct_type_name<T>()); it != map.end()) {
            return it->second;
        }
        y_fatal("Type does not exist");
    } else {
        return ct_type_name<T>();
    }
}

template<typename F, usize I = 0>
inline std::string_view fill_prototype_args(const TypeNameMap& map) {
    using traits = function_traits<F>;
    if constexpr(I < traits::arg_count) {
        using arg_type = typename traits::template arg_type<I>;
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<arg_type>>;
        constexpr bool is_ref = std::is_reference_v<arg_type>;
        static_assert(is_const || !is_ref, "inout reference parameters are not supported");
        return fmt("{}{}{}{}",
            I ? ", " : "",
            is_const ? "const " : "",
            type_name<std::remove_cvref_t<arg_type>>(map),
            is_ref ? (is_const ? " &in" : " &inout") : "",
            fill_prototype_args<F, I + 1>(map)
        );
    } else {
        return "";
    }
}

template<typename F>
inline const char* create_prototype(const TypeNameMap& map, const char* name) {
    using traits = function_traits<F>;
    return fmt_c_str("{} {}({})", type_name<typename traits::return_type>(map), name, fill_prototype_args<F>(map));
}




template<typename T>
void bind_type(asIScriptEngine* engine, TypeNameMap& map, const char* name) {
    constexpr bool is_pod = std::is_trivially_copyable_v<T> && std::is_trivially_default_constructible_v<T> && std::is_trivially_destructible_v<T>;

    // asGetTypeTraits needs to be made constexpr
    constexpr asQWORD flags = asOBJ_VALUE | (is_pod ? asOBJ_POD : asGetTypeTraits<T>());

    check(engine->RegisterObjectType(name, sizeof(T), flags));

    if constexpr(flags & asOBJ_APP_CLASS_CONSTRUCTOR) {
        check(engine->RegisterObjectBehaviour(name, asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(+[](T* self) {
            new(self) T();
        }), asCALL_CDECL_OBJLAST));
    }

    if constexpr(flags & asOBJ_APP_CLASS_DESTRUCTOR) {
        check(engine->RegisterObjectBehaviour(name, asBEHAVE_DESTRUCT, "void f()", asFUNCTION(+[](T* self) {
            self->~T();
        }), asCALL_CDECL_OBJLAST));
    }

    if constexpr(flags & asOBJ_APP_CLASS_COPY_CONSTRUCTOR) {
        check(engine->RegisterObjectBehaviour(name, asBEHAVE_CONSTRUCT, fmt_c_str("void f(const {} &in)", name), asFUNCTION(+[](const T& other, T* self) {
            new(self) T(other);
        }), asCALL_CDECL_OBJLAST));
    }

    if constexpr(flags & asOBJ_APP_CLASS_ASSIGNMENT) {
        check(engine->RegisterObjectMethod(name, fmt_c_str("{}& opAssign(const {} &in)", name, name), asMETHODPR(T, operator=, (const T&), T&), asCALL_THISCALL));
    }


    auto& as_name = map[ct_type_name<T>()];
    y_always_assert(as_name.is_empty(), "Type has already been registered");
    as_name = name;
}


template<auto F, typename T, typename Ret, typename... Args>
void bind_method_inner(asIScriptEngine* engine, const TypeNameMap& map, const char* name, Ret (T::*)(Args...) const) {
    const char* proto = fmt_c_str("{} const", create_prototype<decltype(F)>(map, name));
    check(engine->RegisterObjectMethod(type_name<T>(map).data(), proto, asFUNCTION(+[](const T& object, Args... args) {
        (object.*F)(args...);
    }), asCALL_CDECL_OBJFIRST));
}

template<auto F, typename T, typename Ret, typename... Args>
void bind_method_inner(asIScriptEngine* engine, const TypeNameMap& map, const char* name, Ret (T::*)(Args...)) {
    const char* proto = fmt_c_str("{}", create_prototype<decltype(F)>(map, name));
    check(engine->RegisterObjectMethod(type_name<T>(map).data(), proto, asFUNCTION(+[](T& object, Args... args) {
        (object.*F)(args...);
    }), asCALL_CDECL_OBJFIRST));
}

template<auto F>
void bind_method(asIScriptEngine* engine, const TypeNameMap& map, const char* name) {
    bind_method_inner<F>(engine, map, name, F);
}


template<typename F>
void bind_global_func(asIScriptEngine* engine, const TypeNameMap& map, const char* name, F func) {
    const char* proto = create_prototype<F>(map, name);
    check(engine->RegisterGlobalFunction(proto, asFUNCTION(func), asCALL_CDECL));
}



}
}


#endif // YAVE_SCRIPT_HELPERS_H

