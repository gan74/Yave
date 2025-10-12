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
#ifndef YAVE_UTILS_PROFILE_H
#define YAVE_UTILS_PROFILE_H

#include <y/concurrent/concurrent.h>
#include <y/concurrent/Mutexed.h>

#include <source_location>
#include <cstring>

#if defined(TRACY_ENABLE) && !defined(YAVE_PROFILING_DISABLED)
#define YAVE_PROFILING
#define YAVE_GPU_PROFILING
#define YAVE_LOCK_PROFILING
#endif

#if defined(YAVE_PROFILING) && !defined(TRACY_ENABLE)
#error TRACY_ENABLE should be set if YAVE_PROFILING is set
#endif


#ifdef YAVE_PROFILING

#include <external/tracy/public/tracy/Tracy.hpp>


#define y_profile_frame_begin()                 do {} while(false)
#define y_profile_frame_end()                   do { FrameMark; } while(false)

#define y_profile_msg(msg)                      do { const char* y_msg = (msg); TracyMessage(y_msg, std::strlen(y_msg)); } while(false)

#define y_profile()                             ZoneNamed(y_create_name_with_prefix(tracy), true)
#define y_profile_zone(name)                    ZoneNamedN(y_create_name_with_prefix(tracy), name, true)
#define y_profile_dyn_zone(name)                ZoneNamed(y_create_name_with_prefix(tracy), true); ZoneNameV(y_create_name_with_prefix(tracy), name, std::strlen(name))


#define y_profile_alloc(ptr, size)              TracyAlloc(ptr, size)
#define y_profile_free(ptr)                     TracyFree(ptr)

#ifdef YAVE_LOCK_PROFILING

#define y_profile_set_lock_name(varname, name)  do { (varname).CustomName((name), std::strlen(name)); } while(false)

namespace yave {
template<typename T = std::mutex>
struct ProfiledLock : tracy::Lockable<T>, y::NonMovable {
    ProfiledLock() : tracy::Lockable<T>([]() -> const tracy::SourceLocationData* { static constexpr tracy::SourceLocationData srcloc{nullptr, "Unamed lock", __FILE__, __LINE__, 0}; return &srcloc; }()) {
        static_assert(!std::is_same_v<T, std::shared_mutex>);
        static_assert(!std::is_same_v<T, std::shared_timed_mutex>);
    }

    using tracy::Lockable<T>::lock;
    using tracy::Lockable<T>::unlock;
    using tracy::Lockable<T>::try_lock;
};

template<typename T = std::shared_mutex>
struct ProfiledSharedLock : tracy::SharedLockable<T>, y::NonMovable {
    ProfiledSharedLock() : tracy::SharedLockable<T>(([]() -> const tracy::SourceLocationData* { static constexpr tracy::SourceLocationData srcloc{nullptr, "Unamed shared lock", __FILE__, __LINE__, 0}; return &srcloc; }())) {
    }

    using tracy::SharedLockable<T>::lock;
    using tracy::SharedLockable<T>::unlock;
    using tracy::SharedLockable<T>::try_lock;
    using tracy::SharedLockable<T>::lock_shared;
    using tracy::SharedLockable<T>::unlock_shared;
    using tracy::SharedLockable<T>::try_lock_shared;
};

template<typename T, typename Lock = std::mutex>
using ProfiledMutexed = y::concurrent::Mutexed<T, ProfiledLock<Lock>>;
}

#else // YAVE_LOCK_PROFILING

#define y_profile_set_lock_name(varname, name)  do {} while(false)

namespace yave {
template<typename T = std::mutex>
using ProfiledLock = T;

template<typename T = std::shared_mutex>
using ProfiledSharedLock = T;

template<typename T, typename Lock = std::mutex>
using ProfiledMutexed = y::concurrent::Mutexed<T, ProfiledLock<Lock>>;
}

#endif // YAVE_LOCK_PROFILING

#else // YAVE_PROFILING

#define y_profile_frame_begin()                 do {} while(false)
#define y_profile_frame_end()                   do {} while(false)

#define y_profile_msg(msg)                      do {} while(false)

#define y_profile()                             do {} while(false)
#define y_profile_zone(name)                    do {} while(false)
#define y_profile_dyn_zone(name)                do {} while(false)

#define y_profile_alloc(ptr, size)              do {} while(false)
#define y_profile_free(ptr)                     do {} while(false)

#define y_profile_set_lock_name(varname, name)  do {} while(false)

namespace yave {

template<typename T = std::mutex>
using ProfiledLock = T;

template<typename T = std::shared_mutex>
using ProfiledSharedLock = T;

template<typename T, typename Lock = std::mutex>
using ProfiledMutexed = y::concurrent::Mutexed<T, ProfiledLock<Lock>>;

}

#endif // YAVE_PROFILING

#endif // YAVE_UTILS_PROFILE_H

