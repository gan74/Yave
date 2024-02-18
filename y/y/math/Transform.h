/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#ifndef Y_MATH_TRANSFORM_H
#define Y_MATH_TRANSFORM_H


#include "Matrix.h"
#include "Quaternion.h"

namespace y {
namespace math {

template<typename T = float>
struct Transform : Matrix4<T> {

    using Matrix4<T>::operator*;

    inline constexpr Transform() : Matrix4<T>(identity()) {
    }

    inline constexpr Transform(const Matrix4<T>& m) : Matrix4<T>(m) {
    }

    inline constexpr Transform(const Vec<3, T>& pos) : Matrix4<T>(identity()) {
        position() = pos;
    }

    inline constexpr Transform(const Vec<3, T>& pos, const Quaternion<T>& rotation, const Vec<3, T>& scale = {1, 1, 1}) {
        this->column(0).template to<3>() = rotation({scale.x(), 0, 0});
        this->column(1).template to<3>() = rotation({0, scale.y(), 0});
        this->column(2).template to<3>() = rotation({0, 0, scale.z()});
        this->column(3) = Vec<4, T>(pos, 1);
    }

    inline constexpr operator Matrix4<T>() const {
        return *this;
    }

    inline constexpr const Matrix4<T>& matrix() const {
        return *this;
    }

    inline constexpr math::Vec<3, T> transform_point(const Vec<3, T>& p) const {
        return position() + transform_direction(p);
    }

    inline constexpr math::Vec<3, T> transform_direction(const Vec<3, T>& p) const {
        return
            this->column(0).template to<3>() * p.x() +
            this->column(1).template to<3>() * p.y() +
            this->column(2).template to<3>() * p.z();
    }

    // Y forward
    inline constexpr const auto& forward() const {
        return this->column(1).template to<3>();
    }

    // X right
    inline constexpr const auto& right() const {
        return this->column(0).template to<3>();
    }

    // Z up
    inline constexpr const auto& up() const {
        return this->column(2).template to<3>();
    }


    inline constexpr const auto& position() const {
        return this->column(3).template to<3>();
    }

    inline constexpr auto& position() {
        return this->column(3).template to<3>();
    }

    inline constexpr Transform non_uniformly_scaled(Vec<3, T> scale) const {
        Transform tr = *this;
        tr.non_uniform_scale(scale);
        return tr;
    }

    inline constexpr void non_uniform_scale(Vec<3, T> scale) {
        this->column(0).template to<3>() *= scale[0];
        this->column(1).template to<3>() *= scale[1];
        this->column(2).template to<3>() *= scale[2];
    }

    inline constexpr Transform scaled(T scale) const {
        Transform tr = *this;
        tr.scale(scale);
        return tr;
    }

    inline constexpr void scale(T scale) {
        this->column(0).template to<3>() *= scale;
        this->column(1).template to<3>() *= scale;
        this->column(2).template to<3>() *= scale;
    }

    inline constexpr Vec<3, T> scale() const {
        const auto& x = this->column(0).template to<3>();
        const auto& y = this->column(1).template to<3>();
        const auto& z = this->column(2).template to<3>();
        return Vec<3, T>{x.length(), y.length(), z.length()};
    }

    inline constexpr void set_basis(const Vec<3, T>& forward, const Vec<3, T>& right, const Vec<3, T>& up) {
        this->column(0).template to<3>() = right;
        this->column(1).template to<3>() = forward;
        this->column(2).template to<3>() = up;
    }

    inline constexpr void set_basis(const Vec<3, T>& forward, const Vec<3, T>& up) {
        set_basis(forward, forward.cross(up), up);
    }

    inline constexpr std::tuple<Vec<3, T>,  Quaternion<T>, Vec<3, T>> decompose() const {
        const auto& x = this->column(0).template to<3>();
        const auto& y = this->column(1).template to<3>();
        const auto& z = this->column(2).template to<3>();
        const Vec<3, T> scale = {x.length(), y.length(), z.length()};
        return {position(), Quaternion<T>::from_base(x / scale.x(), y / scale.y(), z / scale.z()), scale};
    }
};


static_assert(sizeof(Transform<>) == sizeof(Matrix4<>), "Transfrom<T> should have the same size as Matrix4<T>");
static_assert(std::is_trivially_copyable_v<Transform<>>, "Transfrom<T> should be trivially copyable");

}
}

#endif // Y_MATH_TRANSFORM_H

