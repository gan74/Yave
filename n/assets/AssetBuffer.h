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
			//static_assert(IsThreadSafe<Args...>::value, "Only thread-safe types should be used with AssetBuffer<T, AsyncLoadingPolicy>");
			AssetPtr<T> ptr(new const T*(0));
			concurent::Async([=, &loader](Args... args) {
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

template<typename T, typename LoadPolicy, bool Memo = true>
class AssetBuffer : protected LoadPolicy, core::NonCopyable
{
	struct MapOp
	{
		public:
			bool operator()(const typename AssetLoader<T>::ArgumentTypes &a, const typename AssetLoader<T>::ArgumentTypes &b) const {
				if(a.size() == b.size()) {
					auto x = a.begin();
					auto y = b.begin();
					while(x != a.end()) {
						if(*x++ < *y++) {
							return true;
						}
					}
					return false;
				}
				return a.size() < b.size();
			}
	};

	public:
		AssetBuffer() {
		}

		template<typename... Args>
		Asset<T> load(Args... args) {
			return load(BoolToType<Memo>(), args...);
		}

		template<typename... Args, typename U>
		void addLoader(U u) {
			mutex.lock();
			loader.addLoader<Args...>(u);
			mutex.unlock();
		}

		/*void gc() {
			mutex.lock();
			assets.filter([](const AssetPtr<T> &ptr) {
				return ptr.getReferenceCount() > 1;
			});
			mutex.unlock();
		}*/

	private:

		template<typename... Args>
		Asset<T> load(TrueType, Args... args) {
			typename AssetLoader<T>::ArgumentTypes tps = AssetLoader<T>::template getArgumentTypes<Args...>();
			mutex.lock();
			auto it = assets.find(tps);
			if(it == assets.end()) {
				it = assets.insert(tps, LoadPolicy::operator()(loader, args...));
			}
			mutex.unlock();
			return (*it)._2;
		}

		template<typename... Args>
		Asset<T> load(FalseType, Args... args) {
			AssetPtr<T> asset(LoadPolicy::operator()(loader, args...));
			mutex.lock();
			assets.append(asset);
			mutex.unlock();
			return asset;
		}

		concurent::Mutex mutex;
		AssetLoader<T> loader;
		typename If<Memo, core::Map<typename AssetLoader<T>::ArgumentTypes, AssetPtr<T>, MapOp>, core::Array<AssetPtr<T>>>::type assets;
};

}
}

#endif // N_ASSETS_ASSETBUFFER_H
