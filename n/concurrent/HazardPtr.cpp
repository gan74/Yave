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

#include "HazardPtr.h"

namespace n {
namespace concurrent {
namespace internal {

thread_local HazardPtr *hazard = 0;
thread_local core::Array<DeleteNode *> *deleteList = 0;

SpinLock globalHazardLock;
core::Array<const void **> globalHazards;
uint globalHazardCount = 0;

SpinLock globalDeleteLock;
core::Array<DeleteNode *> globalDeletes;
uint globalDeleteCount = 0;

void registerHazard(const void **ptr) {
	globalHazardLock.lock();
	globalHazards.append(ptr);
	globalHazardCount++;
	globalHazardLock.unlock();
}

void unregisterHazard(const void **ptr) {
	globalHazardLock.lock();
	globalHazards.erase(globalHazards.find(ptr));
	globalHazardCount--;
	globalHazardLock.unlock();
}

void closeThreadHazards() {
	delete hazard;
	hazard = 0;
	deleteThreadHazards();
	if(deleteList && !deleteList->isEmpty()) {
		globalDeleteLock.lock();
		globalDeletes.append(*deleteList);
		globalDeleteCount = globalDeletes.size();
		globalDeleteLock.unlock();
	}
	delete deleteList;
	deleteList = 0;
}

core::Array<const void **> getHazards() {
	globalHazardLock.lock();
	core::Array<const void **> h = globalHazards;
	globalHazardLock.unlock();
	return h;
}

uint getHazardCount() {
	return globalHazardCount;
}

void freeIfPossible(core::Array<const void **> &hz, core::Array<DeleteNode*> &nodes) {
	nodes.filter([&hz](DeleteNode *node) {
		for(const void **h : hz) {
			if(node->ptr == *h) {
				return true;
			}
		}
		node->free();
		delete node;
		return false;
	});
}

void deleteThreadHazards() {
	if(!deleteList || deleteList->isEmpty()) {
		return;
	}
	core::Array<const void **> hz = getHazards();
	if(globalDeleteCount && globalDeleteLock.trylock()) {
		freeIfPossible(hz, globalDeletes);
		globalDeleteCount = globalDeletes.size();
		globalDeleteLock.unlock();
	}
	freeIfPossible(hz, *deleteList);
}



}
}
}

