/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#ifndef N_MATH_RANDOMDISTRIBUTION_H
#define N_MATH_RANDOMDISTRIBUTION_H

#include "Matrix.h"

namespace n {
namespace math {

template<typename T>
class RandomDistribution
{
	public:
		virtual ~RandomDistribution() {
		}

		virtual T eval() const = 0;

		T operator()() const {
			return eval();
		}

		/**virtual PrecomputedRange<T> toPrecomputedDistribution(uint pCount = 0) const {
			pCount = pCount ? pCount : getOptimalSampleCount();
			core::Array<T> arr(pCount);
			for(uint i = 0; i != pCount; i++) {
				arr.append(eval());
			}
			return PrecomputedRange<T>(arr);
		}*/

	protected:
		/*virtual uint getOptimalSampleCount() const {
			return 512;
		}*/

};

template<typename T>
class UniformDistribution : public RandomDistribution<T>
{
	public:
		UniformDistribution(T max, T min) : mi(min), ma(max) {
		}

		UniformDistribution(T max) : mi(-max), ma(max) {
		}

		virtual ~UniformDistribution() {
		}

		virtual T eval() const final override {
			float w = random();
			return mi * w + ma * (1.0 - w);
		}

	private:
		T mi;
		T ma;

};

template<typename T = float>
class UniformVec3Distribution : public RandomDistribution<Vec<3, T>>
{
	public:
		UniformVec3Distribution(Vec<3, T> direction, T maxAngle = T(pi * 0.5), T minLen = T(1), T maxLen = T(1)) : angle(maxAngle * 0.5), minLength(minLen), maxLength(maxLen) {
			direction.normalize();
			Vec<3, T> up = fabs(direction.dot(Vec<3, T>(0, 0, 1))) > 0.999 ? Vec<3, T>(1, 0, 0) : Vec<3, T>(0, 0, 1);
			Vec<3, T> side = direction.cross(up).normalized();
			up = direction.cross(side);
			matrix = Matrix3<T>(up, side, direction).transposed();
		}

		virtual ~UniformVec3Distribution() {
		}

		virtual Vec<3, T> eval() const final override {
			float ma = (random() * 2.0 * pi);
			float mx = random() * angle;
			float smx = sin(mx);
			Vec<3, T> local(cos(ma) * smx, sin(ma) * smx, cos(mx));
			float w = random();
			T l = minLength * w + maxLength * (1.0 - w);
			return (matrix * local) * l;
		}

	private:
		Matrix3<T> matrix;
		T angle;
		T minLength;
		T maxLength;


};

}
}

#endif // N_MATH_RANDOMDISTRIBUTION_H

