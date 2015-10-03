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
#include <n/concurrent/Promise.h>

namespace n {
namespace graphics {

SubMeshInstance::SubMeshInstance(const typename TriangleBuffer<>::FreezedTriangleBuffer &b, const Material &m) : SubMeshInstance(new TriangleBuffer<>::FreezedTriangleBuffer(b), m) {
}

SubMeshInstance::SubMeshInstance(const SubMeshInstance &s, const Material &m) : material(m), buffer(s.buffer), vao(s.vao) {
}

SubMeshInstance::SubMeshInstance(const core::SmartPtr<typename TriangleBuffer<>::FreezedTriangleBuffer> &b, const Material &m) : material(m), buffer(b) {
}

SubMeshInstance::SubMeshInstance(const VertexArraySubObject<> &b, const Material &m) : material(m), vao(b) {
}

void SubMeshInstance::draw(const VertexAttribs &attribs, uint renderFlags, uint instances) const {
	if(vao.isNull() && buffer) {
		vao = VertexArraySubObject<>(new VertexArrayObject<>(buffer));
		buffer = 0;
	}
	vao.draw(material, attribs, renderFlags, instances);
}

const Material &SubMeshInstance::getMaterial() const {
	return material;
}

float SubMeshInstance::getRadius() const {
	return buffer ? buffer->radius : vao.getRadius();
}

const VertexArraySubObject<> &SubMeshInstance::getVertexArrayObject() const {
	return vao;
}





internal::MeshInstance::MeshInstance(const core::Array<SubMeshInstance *> &b, uint opt) : subs(b), radius(0) {
	for(SubMeshInstance *sub : subs) {
		radius = std::max(radius, sub->getRadius());
	}
	if(opt & MeshOptimisationOptions::MergeVAO) {
		core::Array<uint> indexes;
		core::Array<Vertex<>> vertices;
		for(SubMeshInstance * p : subs) {
			indexes += p->buffer->indexes;
			vertices += p->buffer->vertices;
		}
		core::SmartPtr<VertexArrayObject<>> vao(new VertexArrayObject<>(TriangleBuffer<>::FreezedTriangleBuffer(indexes, vertices, radius)));
		uint iCount = 0;
		uint vCount = 0;

		for(SubMeshInstance *p : subs) {
			core::SmartPtr<typename TriangleBuffer<>::FreezedTriangleBuffer> buffer = p->buffer;
			uint num = buffer->indexes.size() / 3;
			p->vao = VertexArraySubObject<>(vao, iCount, num, vCount, buffer->radius);
			iCount += num;
			vCount += buffer->vertices.size();
		}
	}
}

internal::MeshInstance::MeshInstance(const typename TriangleBuffer<>::FreezedTriangleBuffer &&b, const graphics::Material &m) : MeshInstance(core::Array<SubMeshInstance *>({new SubMeshInstance(b, m)})) {
}

internal::MeshInstance::~MeshInstance() {
	for(const SubMeshInstance *b : subs) {
		delete b;
	}
}

void internal::MeshInstance::draw(const VertexAttribs &attribs, uint instances) const {
	for(const SubMeshInstance *b : subs) {
		b->draw(attribs, instances);
	}
}

float internal::MeshInstance::getRadius() const {
	return radius;
}

internal::MeshInstance::const_iterator internal::MeshInstance::begin() const {
	return subs.begin();
}

internal::MeshInstance::const_iterator internal::MeshInstance::end() const {
	return subs.end();
}

const core::Array<SubMeshInstance *> &internal::MeshInstance::getBases() const {
	return subs;
}






MeshInstance::MeshInstance() : assets::Asset<internal::MeshInstance>() {
}

MeshInstance::MeshInstance(const core::Array<SubMeshInstance *> &subs) : MeshInstance(new internal::MeshInstance(subs)) {
}

MeshInstance::MeshInstance(const typename TriangleBuffer<>::FreezedTriangleBuffer &&b, const Material &m) : MeshInstance(new internal::MeshInstance(std::move(b), m)) {
}

float MeshInstance::getRadius() const {
	const internal::MeshInstance *i = getInternal();
	return i ? i->getRadius() : -1;
}

void MeshInstance::draw(const VertexAttribs &attribs, uint instances) const {
	const internal::MeshInstance *i = getInternal();
	if(i) {
		i->draw(attribs, instances);
	}
}

MeshInstance::const_iterator MeshInstance::MeshInstance::begin() const {
	const internal::MeshInstance *i = getInternal();
	return i ? i->begin() : 0;
}

MeshInstance::const_iterator MeshInstance::end() const {
	const internal::MeshInstance *i = getInternal();
	return i ? i->end() : 0;
}

core::Array<SubMeshInstance *> MeshInstance::getBases() const {
	const internal::MeshInstance *i = getInternal();
	return i ? i->getBases() : core::Array<SubMeshInstance *>();
}

MeshInstance::MeshInstance(const assets::Asset<internal::MeshInstance> &t) : assets::Asset<internal::MeshInstance>(t) {
}

MeshInstance::MeshInstance(internal::MeshInstance *i) : assets::Asset<internal::MeshInstance>(std::move(i)) {
}

const internal::MeshInstance *MeshInstance::getInternal() const {
	return isValid() ? this->operator->() : (const internal::MeshInstance *)0;
}

}
}
