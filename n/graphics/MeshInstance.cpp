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

#include "MeshInstance.h"
#include "VertexArrayFactory.h"
#include <n/concurrent/Promise.h>

namespace n {
namespace graphics {

SubMeshInstance::SubMeshInstance() {
}

SubMeshInstance::SubMeshInstance(const typename TriangleBuffer<>::FreezedTriangleBuffer &b, const Material &m) : SubMeshInstance(VertexArrayObject<>(GLContext::getContext()->getVertexArrayFactory()(b)), m) {
}

SubMeshInstance::SubMeshInstance(const SubMeshInstance &s, const Material &m) : material(m), vao(s.vao) {
}

SubMeshInstance::SubMeshInstance(const VertexArrayObject<> &b, const Material &m) : material(m), vao(b) {
}

const Material &SubMeshInstance::getMaterial() const {
	return material;
}

float SubMeshInstance::getRadius() const {
	return vao.getRadius();
}

const VertexArrayObject<> &SubMeshInstance::getVertexArrayObject() const {
	return vao;
}





MeshInstance::MeshInstance() : assets::Asset<AssetType>() {
}

MeshInstance::MeshInstance(const assets::Asset<AssetType> &a) : assets::Asset<AssetType>(a) {
}

MeshInstance::MeshInstance(const AssetType &subs) : assets::Asset<AssetType>(new AssetType(subs)) {
}

MeshInstance::MeshInstance(const typename TriangleBuffer<>::FreezedTriangleBuffer &&b, const Material &m) : MeshInstance(SubMeshInstance(std::move(b), m)) {
}

MeshInstance::MeshInstance(const SubMeshInstance &sub) : MeshInstance(new AssetType({sub})) {
}

MeshInstance::MeshInstance(AssetType *a) : assets::Asset<AssetType>(a) {
}

float MeshInstance::getRadius() const {
	const AssetType *i = getInternal();
	float r = -1;
	if(i) {
		for(const SubMeshInstance &s : *i) {
			if(s.getRadius() > r) {
				r = s.getRadius();
			}
		}
	}
	return r;

}

MeshInstance::const_iterator MeshInstance::MeshInstance::begin() const {
	const AssetType *i = getInternal();
	return i ? i->begin() : 0;
}

MeshInstance::const_iterator MeshInstance::end() const {
	const AssetType *i = getInternal();
	return i ? i->end() : 0;
}

MeshInstance::AssetType MeshInstance::getSubs() const {
	return isValid() ? *this->operator->() : AssetType();
}

const MeshInstance::AssetType *MeshInstance::getInternal() const {
	return isValid() ? this->operator->() : (AssetType *)0;
}

}
}
