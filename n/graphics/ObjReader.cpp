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


#include <iostream>
#include <n/io/File.h>
#include "MeshLoader.h"
#include "MaterialLoader.h"
#include <n/core/Timer.h>

namespace n {
namespace graphics {

class ObjReader : public MeshLoader::MeshReader<ObjReader, core::String>
{

	struct HashPair
	{
		HashPair(const math::Vec3ui &v, uint i = 0) : _1(v), _2(i) {
		}

		math::Vec3ui _1;
		uint _2;
		bool operator==(const HashPair &p) const {
			return _1 == p._1;
		}

		uint64 hash() const {
			return core::hash(&_1, sizeof(_1));
		}
	};


	template<typename T, typename Eq = std::equal_to<T>>
	class Hash
	{
		typedef uint64 hashType;

		template<typename W>
		hashType hashFunc(const W &e) {
			return e.hash();
		}

		public:
			class iterator
			{
				public:
					iterator &operator++() { // ++prefix
						++it;
						if(it == bi->end()) {
							it = (++bi)->begin();
						}
						return *this;
					}

					iterator operator++(int) { // postfix++
						iterator it(*this);
						operator++();
						return it;
					}

					bool operator==(const iterator &i) const {
						return it == i.it;
					}

					bool operator!=(const iterator &i) const {
						return it != i.it;
					}

					const T &operator*() const {
						return *it;
					}

					const T *operator->() const {
						return &(*it);
					}

				private:
					friend class Hash;
					iterator(core::Array<T> *b) : bi(b), it(bi->begin()) {
					}

					iterator(core::Array<T> *b, typename core::Array<T>::iterator i) : bi(b), it(i) {
					}

					core::Array<T> *bi;
					typename core::Array<T>::iterator it;
			};

			typedef iterator const_iterator;

			Hash() : bucketCount(7), hSize(0), buckets(new core::Array<T>[bucketCount + 1]) {
			}

			~Hash() {
				delete[] buckets;
			}

			iterator insert(const T &t) {
				uint b1 = bucketCount + 1;
				if(hSize > b1 >> 2) {
					resize((b1 << 2) - 1);
				}
				hashType h = hashFunc(t);
				core::Array<T> &a = buckets[h % bucketCount];
				typename core::Array<T>::iterator it = a.find(t);
				if(it != a.end()) {
					return iterator(&a, it);
				}
				hSize++;
				return iterator(&a, a.insert(t));
			}

			iterator find(const T &t) {
				hashType h = hashFunc(t);
				core::Array<T> &a = buckets[h % bucketCount];
				typename core::Array<T>::iterator it = a.find(t);
				if(it != a.end()) {
					return iterator(&a, it);
				}
				return end();
			}

			iterator begin() {
				return iterator(buckets);
			}

			iterator end() {
				return iterator(buckets + bucketCount);
			}

			const_iterator begin() const {
				return const_iterator(buckets);
			}

			const_iterator end() const {
				return const_iterator(buckets + bucketCount);
			}

			uint size() const {
				return hSize;
			}

			uint getCapacity() const {
				return (bucketCount + 1) >> 2;
			}

		private:
			void resize(uint s) {
				core::Array<T> *b = buckets;
				uint l = bucketCount;
				bucketCount = s;
				buckets = new core::Array<T>[bucketCount + 1];
				for(uint i = 0; i != l; i++) {
					for(const T &e : b[i]) {
						insert(e);
					}
				}
				delete[] b;
			}

			uint bucketCount;
			uint hSize;
			core::Array<T> *buckets;
	};


	public:
		ObjReader() : MeshLoader::MeshReader<ObjReader, core::String>() {
		}

		internal::MeshInstance<> *operator()(core::String name) override {
			if(!name.endsWith(".obj")) {
				return 0;
			}
			io::File file(name);
			return load(file);
		}

	private:
		internal::MeshInstance<> *load(io::File &file) {
			if(!file.open(io::IODevice::Read)) {
				std::cerr<<file.getName()<<" not found"<<std::endl;
				return 0;
			}
			//core::Timer timer;
			uint fs = file.size();
			char *data = new char[fs + 1];
			data[file.readBytes(data)] = 0;
			core::Array<core::String> lines = core::String(data).split("\n");
			delete[] data;
			file.close();
			core::Array<math::Vec3> positions;
			positions.append(math::Vec3());
			core::Array<math::Vec3> normals;
			normals.append(math::Vec3());
			core::Array<math::Vec2> coords;
			coords.append(math::Vec2());

			for(const core::String &l : lines) {
				if(l.size() < 4) {
					continue;
				}
				if(l.beginsWith("v ")) {
					/*core::Array<float> fl = l.subString(2).split(" ");//.mapped([](const core::String &s) { return s.to<float>(); });
					if(fl.size() != 3) {
						std::cerr<<"Invalid position"<<std::endl;
						return 0;
					}*/
					float fl[3] = {0};
					sscanf(l.toChar(), "v %f %f %f", &fl[0], &fl[1], &fl[2]);
					positions.append(math::Vec3(fl[0], fl[1], fl[2]));
				} else if(l.beginsWith("vn ")) {
					/*core::Array<float> fl = l.subString(3).split(" ");//.mapped([](const core::String &s) { return s.to<float>(); });
					if(fl.size() != 3) {
						std::cerr<<"Invalid normal"<<std::endl;
						return 0;
					}*/
					float fl[3] = {0};
					sscanf(l.toChar(), "vn %f %f %f", &fl[0], &fl[1], &fl[2]);
					normals.append(math::Vec3(fl[0], fl[1], fl[2]));
				} else if(l.beginsWith("vt ")) {
					/*core::Array<float> fl = l.subString(3).split(" ");//.mapped([](const core::String &s) { return s.to<float>(); });
					if(fl.size() != 2) {
						std::cerr<<"Invalid texture coord"<<std::endl;
						return 0;
					}*/
					float fl[2] = {0};
					sscanf(l.toChar(), "vt %f %f", &fl[0], &fl[1]);
					coords.append(math::Vec2(fl[0], fl[1]));
				}
			}
			core::String mtllib;
			Hash<HashPair> vmap;
			bool smooth = false;
			TriangleBuffer<> tr;
			Material<> mat;
			core::Array<MeshInstanceBase<> *> bases;
			for(const core::String &l : lines) {
				if(l.beginsWith("f ")) {
					core::Array<core::String> fl = l.subString(2).split(" ");
					uint face[] = {0, 0, 0};
					if(fl.size() != 3) {
						std::cerr<<"Invalid (non triangle) face. \""<<l<<"\""<<std::endl;
						return 0;
					}
					for(uint i = 0; i != 3; i++) {
						math::Vec3ui v;
						sscanf(fl[i].toChar(), "%u/%u/%u", &v[0], &v[1], &v[2]);
						if(smooth) {
							v[2] = 0;
						}
						Hash<HashPair>::const_iterator it = vmap.find(HashPair(v));
						if(it == vmap.end()) {
							if(v.x() >= positions.size() || v.z() >= normals.size() || v.y() >= coords.size()) {
								std::cerr<<"Index out of bound : "<<core::String(v)<<std::endl;
								return 0;
							}
							Vertex<> vert = Vertex<>(positions[v.x()], normals[v.z()], coords[v.y()]);
							uint index = tr.append(vert);
							vmap.insert(HashPair(v, index));
							face[i] = index;
						} else {
							face[i] = (*it)._2;
						}
					}
					tr.append(face[0], face[1], face[2]);
				} else if(l.beginsWith("s ")) {
					core::String sm = l.subString(2).toLower().filtered([](char c) { return !isspace(c); });
					smooth = !(sm == "off" || sm == "0");
				} else if(l.beginsWith("usemtl ")) {
					if(!tr.getTriangles().isEmpty()) {
						bases.append(new MeshInstanceBase<>(std::move(tr.freezed()), mat));
						tr = TriangleBuffer<>();
					}
					mat = MaterialLoader::load<core::String, core::String>(mtllib, l.subString(7));
				} else if(l.beginsWith("mtllib ")) {
					mtllib = l.subString(7);
				}
			}
			if(!tr.getTriangles().isEmpty()) {
				bases.append(new MeshInstanceBase<>(std::move(tr.freezed()), mat));
			}
			return new internal::MeshInstance<>(bases);
		}
};

}
}

