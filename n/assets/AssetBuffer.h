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

#ifndef N_ASSETS_ASSETBUFFER_H
#define N_ASSETS_ASSETBUFFER_H

#include <n/core/Array.h>
#include "AssetLoader.h"
#include <n/concurent/Async.h>
#include <n/concurent/Mutex.h>

namespace n {
namespace assets {

template<typename T>
class ImmediateLoadingPolicy
{
	public:
		template<typename... Args>
		AssetPtr<T> operator()(AssetLoader<T> &loader, Args... args) {
			AssetPtr<T> t = AssetPtr<T>(new const T*(loader(args...)));
			if(t.isNull()) {
				t.invalidate();
			}
			return t;
		}
};

template<typename T>
class AsyncLoadingPolicy
{
	public:
		template<typename... Args>
		AssetPtr<T> operator()(AssetLoader<T> &loader, Args... args) {
			AssetPtr<T> ptr(new const T*(0));
			concurent::Async([=](Args... args) {
				T *o = loader(args...);
				if(o) {
					*ptr = o;
				} else {
					ptr.invalidate();
				}
				return Nothing();
			}, args...);
			return ptr;
		}
};

template<typename T, typename LoadPolicy>
class AssetBuffer : protected LoadPolicy, core::NonCopyable
{
	public:
		AssetBuffer() {
		}

		template<typename... Args>
		Asset<T> load(Args... args) {
			AssetPtr<T> asset(LoadPolicy::operator()(loader, args...));
			mutex.lock();
			assets.append(asset);
			mutex.unlock();
			return Asset<T>(asset);
		}

		template<typename... Args, typename U>
		void addLoader(U u) {
			mutex.lock();
			loader.addLoader<Args...>(u);
			mutex.unlock();
		}

		void gc() {
			mutex.lock();
			assets.filter([](const AssetPtr<T> &ptr) {
				return ptr.getReferenceCount() > 1;
			});
			mutex.unlock();
		}

	private:
		concurent::Mutex mutex;
		AssetLoader<T> loader;
		core::Array<AssetPtr<T>> assets;
};

}
}

#endif // N_ASSETS_ASSETBUFFER_H
