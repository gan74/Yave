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

#ifndef N_GRAPHICS_BSPBUILDER
#define N_GRAPHICS_BSPBUILDER

#include <n/math/Plane.h>
#include "TriangleBuffer.h"

namespace n {
namespace graphics {


template<typename T>
struct BspNode
{
	math::Plane<T> plane;
	BspNode *in;
	BspNode *out;

	~BspNode() {
		delete in;
		delete out;
	}

	BspNode(const math::Plane<T> &p) : plane(p), in(0), out(0) {
	}
};

template<typename T = float>
class BspBuilder
{
	typedef typename TriangleBuffer<T>::FreezedTriangleBuffer TBuffer;
	typedef BspNode<T> Node;
	typedef math::Plane<T> Plane;


	struct Triangle
	{
		math::Vec<3, T> p[3];

		bool operator!=(const Triangle &t) const {
			for(uint i = 0; i != 3; i++) {
				if(p[i] != t.p[i]) {
					return true;
				}
			}
			return false;
		}

		bool operator==(const Triangle &t) const {
			return !operator!=(t);
		}

		int intersection(const Plane &plane) const {
			return math::sign(plane.getSignedDistance(p[0])) + math::sign(plane.getSignedDistance(p[1])) + math::sign(plane.getSignedDistance(p[2]));
		}

		Plane getPlane() const {
			return Plane(math::Edge<T>(p[0], p[1]), math::Edge<T>(p[1], p[2]));
		}
	};

	public:
		BspBuilder(const TBuffer &buffer) : root(build(getTris(buffer))) {
		}

		Node *root;

	private:
		static core::Array<Triangle> getTris(const TBuffer &buffer) {
			core::Array<Triangle> t;
			for(uint i = 0; i != buffer.indexes.size() / 3; i++) {
				t.append(Triangle{buffer.vertices[buffer.indexes[i * 3]].p(), buffer.vertices[buffer.indexes[i * 3 + 1]].p(), buffer.vertices[buffer.indexes[i * 3 + 2]].p()});
			}
			return t;
		}

		static Node *build(const core::Array<Triangle> &tris) {
			if(tris.isEmpty()) {
				return 0;
			}
			if(tris.size() == 1) {
				return new Node(tris.first().getPlane());
			}
			Triangle split = findBest(tris);
			Plane plane = split.getPlane();
			core::Array<Triangle> in;
			core::Array<Triangle> out;
			for(Triangle tr : tris) {
				if(tr == split) {
					continue;
				}
				int s = tr.intersection(plane);
				if(s == 3) {
					out.append(tr);
				} else if(s == -3) {
					in.append(tr);
				} else {
					in.append(tr);
					out.append(tr);
				}
			}
			Node *node = new Node(plane);
			node->in = build(in);
			node->out = build(out);
			return node;
		}

		static Triangle findBest(const core::Array<Triangle> &tris) {
			Triangle best = tris.first();
			T score = tris.size() * 4;
			for(Triangle tr : tris) {
				T tot = 0;
				Plane pl = tr.getPlane();
				for(Triangle t : tris) {
					tot += t.intersection(pl);
				}
				if(abs(tot) < score) {
					best = tr;
					score = abs(tot);
				}
			}
			return best;
		}
};

}
}

#endif // N_GRAPHICS_BSPBUILDER

