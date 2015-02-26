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

namespace internal {
	template<typename T>
	class AssetKey
	{
		struct KeyStoreBase : core::NonCopyable
		{
			virtual bool operator<(const KeyStoreBase &) const = 0;
			virtual bool operator==(const KeyStoreBase &) const = 0;

			virtual KeyStoreBase *clone() const = 0;

			virtual ~KeyStoreBase() {
			}
		};

		template<typename... Args>
		struct KeyStore : public KeyStoreBase
		{
			KeyStore(Args... args) : tuple(args...) {
			}

			KeyStore(const std::tuple<Args...> &t) : tuple(t) {
			}

			virtual bool operator<(const KeyStoreBase &k) const override {
				const KeyStore<Args...> *p = dynamic_cast<const KeyStore<Args...> *>(&k);
				return tuple < p->tuple;
			}

			virtual bool operator==(const KeyStoreBase &k) const override {
				const KeyStore<Args...> *p = dynamic_cast<const KeyStore<Args...> *>(&k);
				return tuple == p->tuple;
			}


			virtual KeyStoreBase *clone() const override {
				return new KeyStore(tuple);
			}

			std::tuple<Args...> tuple;
		};

		public:
			template<typename... Args>
			AssetKey(Args... args) : types(getArgumentTypes<Args...>()), base(new KeyStore<Args...>(args...)) {
			}

			AssetKey(const AssetKey &k) : types(k.types), base(k.base->clone()) {
			}

			~AssetKey() {
				delete base;
			}

			bool operator<(const AssetKey &k) const {
				if(types.size() == k.types.size()) {
					auto x = types.begin();
					auto y = k.types.begin();
					while(x != types.end()) {
						if(*x++ < *y++) {
							return true;
						}
					}
					return base->operator<(*k.base);
				}
				return types.size() < k.types.size();
			}

			bool operator==(const AssetKey &k) const {
				return types == k.types &&  base->operator==(*k.base);
			}

		private:
			ArgumentTypes types;
			KeyStoreBase *base;
	};
}

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
			}, args...);
			return ptr;
		}
};

template<typename T>
class AssetBuffer
{
	public:
		AssetBuffer() : buffer(new core::Map<internal::AssetKey<T>, AssetPtr<T>>()) {
		}

	private:
		template<typename U, typename P>
		friend class AssetManager;

		core::SmartPtr<core::Map<internal::AssetKey<T>, AssetPtr<T>>> buffer;
};

template<typename T, typename LoadPolicy>
class AssetManager : protected LoadPolicy, core::NonCopyable
{


	public:
		AssetManager(const AssetBuffer<T> &b = AssetBuffer<T>()) : assets(b) {
		}

		template<typename... Args>
		Asset<T> load(Args... args) {
			internal::AssetKey<T> k(args...);
			mutex.lock();
			auto it = assets.buffer->find(k);
			if(it == assets.buffer->end()) {
				it = assets.buffer->insert(k, LoadPolicy::operator()(loader, args...));
			}
			mutex.unlock();
			return (*it)._2;
		}

		const AssetBuffer<T> &getAssetBuffer() const {
			return assets;
		}

		template<typename... Args, typename U>
		void addLoader(U u) {
			mutex.lock();
			loader.addLoader<Args...>(u);
			mutex.unlock();
		}

		void gc() {
			mutex.lock();
			assets->filter([](const AssetPtr<T> &ptr) {
				return ptr.getReferenceCount() > 1;
			});
			mutex.unlock();
		}

	private:
		concurent::Mutex mutex;
		AssetLoader<T> loader;
		AssetBuffer<T> assets;
};

}
}

#endif // N_ASSETS_ASSETBUFFER_H
