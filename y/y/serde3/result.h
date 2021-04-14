/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#ifndef Y_SERDE3_RESULT_H
#define Y_SERDE3_RESULT_H

#include <y/core/Result.h>

namespace y {
namespace serde3 {

enum class Success : u32 {
    Full,
    Partial
};

inline Success operator|(Success a, Success b) {
    return a == Success::Full && b == Success::Full
        ? Success::Full
        : Success::Partial;
}



enum class ErrorType : u32 {
    UnknownError,
    IOError,
    VersionError,
    SignatureError,
    MemberTypeError,
    UnknownPolyError,
    SizeError,
};

struct Error {
    Error(ErrorType t, const char* m = nullptr) : type(t), member(m) {
    }

    Error with_name(const char* name) const {
        return Error(type, name);
    }

    const ErrorType type = ErrorType::UnknownError;
    const char* const member = nullptr;
};

using Result = core::Result<Success, Error>;

inline const char* error_msg(ErrorType tpe) {
    static const char* msg[] = {
        "Unknown error",
        "IO error",
        "Incompatible version error",
        "Signature mismatch error",
        "Type mismatch error",
        "Polymorphic ID error",
        "Range size mismatch error",
    };
    y_debug_assert(usize(tpe) < sizeof(msg) / sizeof(msg[0]));
    return msg[usize(tpe)];
}

inline const char* error_msg(const Error& err) {
    return error_msg(err.type);
}

inline const char* error_msg(const Result& res) {
    if(res.is_error()) {
        return error_msg(res.error());
    }
    return "Partial success: format probably mismatch";
}

}
}

#endif // Y_SERDE3_RESULT_H

