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
#ifndef YAVE_MESHES_AABB_H
#define YAVE_MESHES_AABB_H

#include <yave/yave.h>

namespace yave {

class AABB {
    public:
        AABB() = default;

        AABB(const math::Vec3& min, const math::Vec3& max) : _min(min), _max(max) {
            for(usize i = 0; i != 3; ++i) {
                y_debug_assert(_min[i] <= _max[i]);
            }
        }

        static AABB from_center_extent(const math::Vec3& center, const math::Vec3& extent) {
            const math::Vec3 half_extent =  extent * 0.5f;
            return AABB(center - half_extent, center + half_extent);
        }

        math::Vec3 center() const {
            return (_min + _max) * 0.5f;
        }

        math::Vec3 extent() const {
            return _max - _min;
        }

        math::Vec3 half_extent() const {
            return extent() * 0.5f;
        }

        float radius() const {
            return extent().length() * 0.5f;
        }

        float origin_radius() const {
            return std::sqrt(std::max(_min.length2(), _max.length2()));
        }

        const math::Vec3& min() const {
            return _min;
        }

        const math::Vec3& max() const {
            return _max;
        }

        AABB merged(const AABB& other) const {
            return AABB(_min.min(other._min), _max.max(other._max));
        }

        bool contains(const math::Vec3& point) const {
            for(usize i = 0; i != 3; ++i) {
                if(point[i] < _min[i] || point[i] > _max[i]) {
                    return false;
                }
            }
            return true;
        }

        bool contains(const AABB& other) const {
            for(usize i = 0; i != 3; ++i) {
                if(other._min[i] < _min[i] || other._max[i] > _max[i]) {
                    return false;
                }
            }
            return true;
        }

    private:
        math::Vec3 _min;
        math::Vec3 _max;
};

static_assert(std::is_trivially_copyable_v<AABB>);

}

#endif // YAVE_MESHES_AABB_H

