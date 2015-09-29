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

SubMeshInstance::SubMeshInstance(const typename TriangleBuffer<>::FreezedTriangleBuffer &b, const graphics::Material &m) : SubMeshInstance(new TriangleBuffer<>::FreezedTriangleBuffer(b), m) {
}

SubMeshInstance::SubMeshInstance(const SubMeshInstance &s, const graphics::Material &m) : buffer(s.buffer), material(m), vao(s.vao) {
}

SubMeshInstance::SubMeshInstance(const core::SmartPtr<typename TriangleBuffer<>::FreezedTriangleBuffer> &b, const graphics::Material &m) : buffer(b), material(m) {
}

void SubMeshInstance::draw(const VertexAttribs &attribs, uint renderFlags, uint instances) const {
	alloc();
	if(!vao.isNull()) {
		vao.draw(material, attribs, renderFlags, instances);
	}

}

const Material &SubMeshInstance::getMaterial() const {
	return material;
}

float SubMeshInstance::getRadius() const {
	return buffer->radius;
}

const typename TriangleBuffer<>::FreezedTriangleBuffer &SubMeshInstance::getTriangleBuffer() const {
	return *buffer;
}

const VertexArraySubObject<> &SubMeshInstance::getVertexArrayObject() const {
	alloc();
	return vao;
}

void SubMeshInstance::alloc() const {
	if(vao.isNull()) {
		if(future.isSuccess()) {
			vao = future.unsafeGet();
		} else if(future.isUninitialized()) {
			vao = VertexArraySubObject<>(new VertexArrayObject<>(*buffer));
		}
	}
}

static void optimizedAlloc(const core::Array<core::Pair<SubMeshInstance *, concurrent::Promise<VertexArraySubObject<>> *>> &toAlloc, float radius) {
	core::Array<uint> indexes;
	core::Array<Vertex<>> vertices;
	for(core::Pair<SubMeshInstance *, concurrent::Promise<VertexArraySubObject<>> *> p : toAlloc) {
		indexes += p._1->getTriangleBuffer().indexes;
		vertices += p._1->getTriangleBuffer().vertices;
	}
	core::SmartPtr<VertexArrayObject<>> vao(new VertexArrayObject<>(TriangleBuffer<>::FreezedTriangleBuffer(indexes, vertices, radius)));
	uint iCount = 0;
	uint vCount = 0;
	for(core::Pair<SubMeshInstance *, concurrent::Promise<VertexArraySubObject<>> *> p : toAlloc)  {
		uint num = p._1->getTriangleBuffer().indexes.size() / 3;
		p._2->success(VertexArraySubObject<>(vao, iCount, num, vCount, p._1->getRadius()));
		iCount += num;
		vCount += p._1->getTriangleBuffer().vertices.size();
		delete p._2;
	}
}


internal::MeshInstance::MeshInstance(const core::Array<SubMeshInstance *> &b, uint opt) : subs(b), radius(0) {
	core::Array<SubMeshInstance *> toAlloc;
	for(SubMeshInstance *sub : subs) {
		radius = std::max(radius, sub->getRadius());
		if(sub->future.isUninitialized()) {
			toAlloc.append(sub);
		}
	}
	if(opt & MeshOptimisationOptions::MergeVAO) {
		GLContext::getContext()->addGLTask([=]() {
			optimizedAlloc(toAlloc.mapped([](SubMeshInstance *sub) {
				concurrent::Promise<VertexArraySubObject<>> *promise = new concurrent::Promise<VertexArraySubObject<>>();
				sub->future = promise->getFuture();
				return core::Pair<SubMeshInstance *, concurrent::Promise<VertexArraySubObject<>> *>(sub, promise);
			}), radius);
		});
	} else {
		for(SubMeshInstance *sub : toAlloc) {
			concurrent::Promise<VertexArraySubObject<>> *promise = new concurrent::Promise<VertexArraySubObject<>>();
			sub->future = promise->getFuture();
			GLContext::getContext()->addGLTask([=]() {
				promise->success(VertexArraySubObject<>(new VertexArrayObject<>(*sub->buffer)));
				delete promise;
			});
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

bool MeshInstance::isValid() const {
	return assets::Asset<internal::MeshInstance>::isValid();
}

bool MeshInstance::isNull() const {
	return assets::Asset<internal::MeshInstance>::isNull();
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
