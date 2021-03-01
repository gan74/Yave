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
#ifndef YAVE_UTILS_PERF_H
#define YAVE_UTILS_PERF_H

#include <y/utils.h>

#ifdef Y_DEBUG
#ifndef YAVE_PERF_LOG_DISABLED
#define YAVE_PERF_LOG_ENABLED
#endif
#endif


#ifdef YAVE_PERF_LOG_ENABLED
#ifndef TRACY_ENABLE
#error TRACY_ENABLE should be set if YAVE_PERF_LOG_ENABLED is set
#endif
#endif

#include <external/tracy/Tracy.hpp>
#include <external/tracy/TracyC.h>

namespace yave {
namespace perf {

using namespace y;

#ifdef YAVE_PERF_LOG_ENABLED

inline void start_capture(const char* out_filename) {

}

inline void end_capture() {

}

inline bool is_capturing() {
    return false;
}

#define y_profile_frame_end() do { TracyCFrameMark } while(false)

#define y_profile()                                                                                                                             \
    static const auto y_create_name_with_prefix(sloc) = ___tracy_source_location_data { nullptr, __FUNCTION__, __FILE__, __LINE__, 0};          \
    const auto y_create_name_with_prefix(ctx) = ___tracy_emit_zone_begin(&y_create_name_with_prefix(sloc), true);                               \
    y_defer(___tracy_emit_zone_end(y_create_name_with_prefix(ctx)))

#define y_profile_zone(name)                                                                                                                    \
    static const auto y_create_name_with_prefix(sloc) = ___tracy_source_location_data { nullptr, __FUNCTION__, __FILE__, __LINE__, 0};          \
    const char* y_create_name_with_prefix(zone) = (name);                                                                                       \
    const auto y_create_name_with_prefix(ctx) = ___tracy_emit_zone_begin(&y_create_name_with_prefix(sloc), true);                               \
    ___tracy_emit_zone_name(y_create_name_with_prefix(ctx), y_create_name_with_prefix(zone), std::strlen(y_create_name_with_prefix(zone)));     \
    y_defer(___tracy_emit_zone_end(y_create_name_with_prefix(ctx)))

#define y_profile_unique_lock(inner) [&]() {                            \
        std::unique_lock l(inner, std::defer_lock);                     \
        if(l.try_lock()) {                                              \
            return l;                                                   \
        }                                                               \
        y_profile_zone("waiting for lock: " #inner);                    \
        l.lock();                                                       \
        return l;                                                       \
    }()

#else

inline void start_capture(const char*) {}
inline void end_capture() {}
inline bool is_capturing() { return false; }

#define y_profile_frame_end()       do {} while(false)
#define y_profile()                 do {} while(false)
#define y_profile_zone(name)        do {} while(false)
#define y_profile_unique_lock(lock) std::unique_lock(lock)

#endif

}
}

#endif // YAVE_UTILS_PERF_H

