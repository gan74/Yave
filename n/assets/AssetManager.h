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

#ifndef N_ASSETS_ASSETMANAGER_H
#define N_ASSETS_ASSETMANAGER_H

#include <n/core/Array.h>
#include "AssetLoader.h"
#include <n/concurrent/Async.h>
#include <n/concurrent/LockGuard.h>
#include <n/concurrent/SpinLock.h>

namespace n {
namespace assets {

template<typename T>
class AssetKey
{
	struct KeyStoreBase : NonCopyable
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

		AssetKey &operator=(const AssetKey &k) {
			if(k.base != base) {
				delete base;
				base = k.base->clone();
			}
			types = k.types;
			return *this;
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

template<typename T>
class ImmediateLoadingPolicy
{
	public:
		template<typename... Args>
		AssetPtr<T> operator()(const AssetLoader<T> &loader, Args... args) {
			AssetPtr<T> t(new typename AssetPtr<T>::PtrType(loader(args...)));
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
		AssetPtr<T> operator()(const AssetLoader<T> &loader, Args... args) {
			AssetPtr<T> *ptr = new AssetPtr<T>(new typename AssetPtr<T>::PtrType(0));
			AssetPtr<T> r(*ptr);
			concurrent::Async([=, &loader](Args... args) {
				T *o = loader(args...);
				if(o) {
					**ptr = o;
				} else {
					ptr->invalidate();
				}
				delete ptr;
			}, args...);
			return r;
		}
};

template<typename T>
class AssetBuffer
{
	public:
		AssetBuffer() : buffer(new core::Pair<concurrent::SpinLock, core::Map<AssetKey<T>, AssetPtr<T>>>()) {
		}

		void gc() {
			N_LOG_PERF;
			N_LOCK(*this);
			getBuffer().filter([](const core::Pair<const AssetKey<T>, AssetPtr<T>> &p) -> bool {
				return p._2.getReferenceCount() > 1;
			});
		}

		void lock() {
			buffer->_1.lock();
		}

		void unlock() {
			buffer->_1.unlock();
		}

	private:
		template<typename U, typename P>
		friend class AssetManager;

		core::Map<AssetKey<T>, AssetPtr<T>> &getBuffer() {
			return buffer->_2;
		}

		core::SmartPtr<core::Pair<concurrent::SpinLock, core::Map<AssetKey<T>, AssetPtr<T>>>> buffer;
};

template<typename T, typename LoadPolicy>
class AssetManager : protected LoadPolicy, NonCopyable
{
	public:
		AssetManager(const AssetBuffer<T> &b = AssetBuffer<T>()) : assets(b) {
		}

		template<typename... Args>
		Asset<T> load(Args... args) {
			AssetKey<T> k(args...);
			N_LOCK(assets);
			auto it = assets.getBuffer().find(k);
			if(it == assets.getBuffer().end()) {
				it = assets.getBuffer().insert(k, LoadPolicy::operator()(loader, args...));
			}
			return (*it)._2;
		}

		const AssetBuffer<T> &getAssetBuffer() const {
			return assets;
		}

		template<typename... Args, typename U>
		void addLoader(U u) {
			loader.addLoader<Args...>(u); // NOT T-SAFE with load(...)
		}

		void gc() {
			assets.gc();
		}

	private:
		AssetLoader<T> loader;
		AssetBuffer<T> assets;
};

}
}

#endif // N_ASSETS_ASSETMANAGER_H
