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
#ifndef YAVE_UTILS_PROFILE_H
#define YAVE_UTILS_PROFILE_H

#include <y/defines.h>


#if defined(TRACY_ENABLE) && !defined(YAVE_PROFILING_DISABLED)
#define YAVE_PROFILING
#endif

#if defined(YAVE_PROFILING) && !defined(TRACY_ENABLE)
#error TRACY_ENABLE should be set if YAVE_PROFILING is set
#endif


#ifdef YAVE_PROFILING


#include <external/tracy/TracyC.h>


#define y_profile_internal_capturing() (true)

#define y_profile_internal(name)                                                                                                                \
    static constexpr const char* y_create_name_with_prefix(static_name) = (name);                                                               \
    static constexpr auto y_create_name_with_prefix(sloc) = ___tracy_source_location_data {                                                     \
        y_create_name_with_prefix(static_name), __FUNCTION__, __FILE__, __LINE__, 0};                                                           \
    const auto y_create_name_with_prefix(ctx) = ___tracy_emit_zone_begin(&y_create_name_with_prefix(sloc), y_profile_internal_capturing());     \
    y_defer(___tracy_emit_zone_end(y_create_name_with_prefix(ctx)))

#define y_profile_internal_set_name(name)                                                                                                       \
    const char* y_create_name_with_prefix(zone) = (name);                                                                                       \
    ___tracy_emit_zone_name(y_create_name_with_prefix(ctx), y_create_name_with_prefix(zone), strlen(y_create_name_with_prefix(zone)));

#define y_profile_internal_set_arg(arg)                                                                                                         \
    const char* y_create_name_with_prefix(args) = (arg);                                                                                        \
    ___tracy_emit_zone_text(y_create_name_with_prefix(ctx), y_create_name_with_prefix(args), strlen(y_create_name_with_prefix(args)));          \




#define y_profile_frame_end()       do { ___tracy_emit_frame_mark(nullptr); } while(false)



#define y_profile_msg(msg)          do { const char* _y_msg = (msg); TracyCMessage(_y_msg, strlen(_y_msg)); } while(false)



#define y_profile_plot(name, val)   do { static constexpr const char* _y_plot = (name); TracyCPlot(_y_plot, (val)); } while(false)



#define y_profile()  y_profile_internal(nullptr)

#define y_profile_zone(name) y_profile_internal(name)

#define y_profile_dyn_zone(name)                \
    y_profile();                                \
    y_profile_internal_set_name(name)



#define y_profile_arg(arg)                      \
    y_profile();                                \
    y_profile_internal_set_arg(arg)


#define y_profile_zone_arg(name, arg)           \
    y_profile_zone(name);                       \
    y_profile_internal_set_arg(arg)

#define y_profile_dyn_zone_arg(name, arg)       \
    y_profile_dyn_zone(name);                   \
    y_profile_internal_set_arg(arg)



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

#define y_profile_frame_end()               do {} while(false)

#define y_profile_msg(msg)                  do {} while(false)

#define y_profile_plot(name, val)           do {} while(false)

#define y_profile()                         do {} while(false)
#define y_profile_zone(name)                do {} while(false)
#define y_profile_dyn_zone(name)            do {} while(false)

#define y_profile_arg(arg)                  do {} while(false)
#define y_profile_zone_arg(name, arg)       do {} while(false)
#define y_profile_dyn_zone_arg(name, arg)   do {} while(false)


#define y_profile_unique_lock(lock)         std::unique_lock(lock)

#endif // YAVE_PROFILING

#endif // YAVE_PROFILING

