/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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
#ifndef YAVE_SCENE_SPATIAL_PARTITION_H
#define YAVE_SCENE_SPATIAL_PARTITION_H

#include <yave/yave.h>

#include <yave/meshes/AABB.h>

#include <y/core/Vector.h>
#include <y/core/Span.h>
#include <y/core/HashMap.h>

#include <algorithm>


namespace yave {

template<typename T>
class SpatialPartition : NonMovable {
    static constexpr float min_obj_size = 0.1f;
    static constexpr float cell_margin = 0.25f;
    static constexpr float cell_size_exponent = 4.0f;

    struct ObjectData {
        math::Vec4i cell_id;
        AABB aabb;
    };

    struct Cell {
        core::Vector<usize> objects;
    };

    static AABB compute_cell_aabb(const math::Vec4i& cell_id) {
        const float cell_spacing = std::pow(cell_size_exponent, float(cell_id.w()));
        return AABB::from_center_extent(cell_id.to<3>() * cell_spacing, math::Vec3(cell_spacing * (1.0f + cell_margin)));
    }

    [[maybe_unused]]
    static AABB compute_cell_aabb_no_margin(const math::Vec4i& cell_id) {
        const float cell_spacing = std::pow(cell_size_exponent, float(cell_id.w()));
        return AABB::from_center_extent(cell_id.to<3>() * cell_spacing, math::Vec3(cell_spacing));
    }

    public:
        using value_type = T;
        using iterator = T*;
        using const_iterator = const T*;

        template<typename... Args>
        iterator insert(const AABB& aabb, Args&&... args) {
            const usize index = _objects.size();

            auto& obj = _objects.emplace_back(y_fwd(args)...);
            _datas.emplace_back();

            reinsert(aabb, index);
            return &obj;
        }

        template<typename... Args>
        T& emplace_back(Args&&... args) {
            return *insert(AABB{}, y_fwd(args)...);
        }

        void update(const_iterator it, const AABB& aabb, bool force = false) {
            const usize index = it - _objects.data();
            y_debug_assert(index < _objects.size());

            ObjectData& data = _datas[index];
            if(!force && compute_cell_aabb(data.cell_id).contains(aabb)) {
                data.aabb = aabb;
                return;
            }

            Cell& cell = _cells[data.cell_id];
            const auto obj_it = std::find(cell.objects.begin(), cell.objects.end(), index);
            y_debug_assert(obj_it != cell.objects.end());
            cell.objects.erase_unordered(obj_it);

            reinsert(aabb, index);
        }

        void erase_unordered(iterator it) {
            const usize index = it - _objects.data();
            y_debug_assert(index < _objects.size());

            {
                Cell& cell = _cells[_datas[index].cell_id];
                const auto obj_it = std::find(cell.objects.begin(), cell.objects.end(), index);
                y_debug_assert(obj_it != cell.objects.end());
                cell.objects.erase_unordered(obj_it);
            }

            const usize last = _objects.size() - 1;
            if(index != last) {
                Cell& cell = _cells[_datas[last].cell_id];
                const auto obj_it = std::find(cell.objects.begin(), cell.objects.end(), last);
                y_debug_assert(obj_it != cell.objects.end());
                *obj_it = index;
            }

            _objects.erase_unordered(_objects.begin() + index);
            _datas.erase_unordered(_datas.begin() + index);
        }

        const AABB& aabb(const_iterator it) const {
            const usize index = it - _objects.data();
            y_debug_assert(index < _objects.size());

            return _datas[index].aabb;
        }

        AABB cell_aabb(const_iterator it) const {
            const usize index = it - _objects.data();
            return compute_cell_aabb(_datas[index].cell_id);
        }

        core::MutableSpan<T> values() {
            return _objects;
        }

        core::Span<T> values() const {
            return _objects;
        }

        iterator begin() {
            return _objects.begin();
        }

        iterator end() {
            return _objects.end();
        }

        const_iterator begin() const {
            return _objects.begin();
        }

        const_iterator end() const {
            return _objects.end();
        }

        usize size() const {
            return _objects.size();
        }

        usize cell_count() const {
            return _cells.size();
        }

        T& operator[](usize i) {
            return _objects[i];
        }

        const T& operator[](usize i) const {
            return _objects[i];
        }


    private:
        core::FlatHashMap<math::Vec4i, Cell, RangeHash<math::Vec4i>> _cells;
        core::Vector<T> _objects;
        core::Vector<ObjectData> _datas;

        void reinsert(const AABB& aabb, usize index) {
            const float size = std::max(min_obj_size, aabb.radius());

            Y_TODO(fixme)
            float cell_spacing = 0.0f;
            i32 level = -2;
            for(;; ++level) {
                cell_spacing = std::pow(cell_size_exponent, float(level));
                if(cell_spacing * cell_margin >= size * 2.0f) {
                    break;
                }
            }


            const math::Vec3 cell_center = aabb.center() / cell_spacing;
            const math::Vec4i cell_id(
                i32(std::round(cell_center.x())),
                i32(std::round(cell_center.y())),
                i32(std::round(cell_center.z())),
                level
            );

            y_debug_assert(size <= compute_cell_aabb_no_margin(cell_id).half_extent().x() * cell_margin);
            y_debug_assert(size * cell_size_exponent >= compute_cell_aabb_no_margin(cell_id).half_extent().x() * cell_margin);
            y_debug_assert(compute_cell_aabb(cell_id).contains(aabb));

            _datas[index].cell_id = cell_id;
            _datas[index].aabb = aabb;
            _cells[cell_id].objects.emplace_back(index);
        }
};

}

#endif // YAVE_SCENE_SPATIAL_PARTITION_H

