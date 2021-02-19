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
#ifndef Y_MATH_VOLUME_H
#define Y_MATH_VOLUME_H

#include "Vec.h"

namespace y {
namespace math {

template<typename T = float>
class Volume : NonCopyable
{
    public:
        virtual ~Volume() = default;

        virtual bool intersects(const Vec<3, T> &, T) const = 0;
};

template<typename T = float>
class Ray final : public Volume<T> {
    public:
        Ray(const Vec<3, T>& start, const Vec<3, T>& direction) : _start(start), _direction(direction.normalized()) {
        }

        bool intersects(const Vec<3, T>& v, T r) const override {
            const Vec<3, T> p(v - _start);
            const T dot = _direction.dot(p);
            return dot > T(0) && p.length2() - dot * dot < (r * r);
        }

        T distance(const Vec<3, T>& v) const {
            const Vec<3, T> p(v - _start);
            const T dot = _direction.dot(p);
            return std::sqrt(p.length2() - dot * dot);
        }

        const Vec<3, T>& direction() const {
            return _direction;
        }

        const Vec<3, T>& start() const {
            return _start;
        }

    private:
        Vec<3, T> _start;
        Vec<3, T> _direction;
};

template<typename T>
class Sphere final : public Volume<T> {

    public:
        Sphere(const Vec<4, T>& v) : _position(v.template to<3>()), _radius(v.w()) {
        }

        Sphere(const Vec<3, T>& pos, T radius) : _position(pos), _radius(radius) {
        }

        Sphere(T radius) : _radius(radius) {
        }

        bool intersects(const Vec<3, T>& v, T rad) const override {
            return (_position - v).length2() < ((_radius + rad) * (_radius + rad));
        }

        const Vec<3, T>& position() const {
            return _position;
        }

        T radius() {
            return _radius;
        }

    private:
        Vec<3, T> _position;
        T _radius;
};

}
}

#endif // Y_MATH_VOLUME_H

