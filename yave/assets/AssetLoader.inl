/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
#ifndef YAVE_ASSETS_ASSETLOADER_INL
#define YAVE_ASSETS_ASSETLOADER_INL

#ifndef YAVE_ASSETS_ASSETLOADER_H
#error this file should not be included directly

// Just to help the IDE
#include "AssetLoader.h"
#endif

#include <y/core/Chrono.h>

namespace yave {

template<typename T>
void AssetPtr<T>::post_deserialize(const AssetLoadingContext& context)  {
	const AssetId id = this->_id;
	if(id == AssetId::invalid_id()) {
		return;
	}

	*this = context.load<T>(id);
	this->flush_reload();
}

template<typename T>
void AsyncAssetPtr<T>::post_deserialize(const AssetLoadingContext& context)  {
	const AssetId id = this->_id;
	if(id == AssetId::invalid_id()) {
		return;
	}

	*this = context.load_async<T>(id);
	this->flush_reload();
}


// --------------------------- Loader ---------------------------

template<typename T>
Loader<T>::Loader(AssetLoader* parent) : LoaderBase(parent) {
}

template<typename T>
Loader<T>::~Loader() {
	y_profile();
	const auto lock = y_profile_unique_lock(_lock);
	for(auto&& [id, ptr] : _loaded) {
		if(!ptr.expired()) {
			y_fatal("Asset is still live.");
		}
	}
}


template<typename T>
AssetPtr<T> Loader<T>::load(AssetId id) {
	AssetPtr<T> ptr(id);
	load_internal<false>(ptr);
	Y_TODO(we might already be loading asynchronously, maybe do something to prevent that or to flush)
	return ptr;
}


template<typename T>
AsyncAssetPtr<T> Loader<T>::load_async(AssetId id) {
	AsyncAssetPtr<T> ptr(id);
	load_internal<true>(ptr);
	ptr.flush();
	return ptr;
}

template<typename T>
AssetPtr<T> Loader<T>::reload(const AssetPtrBase<T>& ptr) {
	const AssetId id = ptr.id();
	if(id == AssetId::invalid_id()) {
		return AssetPtr<T>();
	}

	auto data = std::make_shared<Data>(id, this);
	load_func(data);

	{
		const auto lock = y_profile_unique_lock(_lock);
		auto& weak = _loaded[id];
		if(auto orig = weak.lock()) {
			orig->set_reloaded(data);
		}
		weak = data;
	}
	return data;
}

template<typename T>
void Loader<T>::load_func(const std::shared_ptr<Data>& data) {
	y_debug_assert(data);
	y_debug_assert(!data->is_loaded());
	y_debug_assert(!data->is_failed());

	const AssetId id = data->id;
	y_debug_assert(id != AssetId::invalid_id());
	if(auto reader = store().data(id)) {
		y_profile_zone("loading");

		/*if(async) {
			core::Duration::sleep(core::Duration::seconds(2));
		}*/

		load_from_t load_from;
		AssetLoadingContext loading_ctx(*parent(), id);
		serde3::ReadableArchive arc(std::move(reader.unwrap()));
		if(arc.deserialize(load_from, loading_ctx)) {
			data->finalize_loading(T(device(), std::move(load_from)));
		} else {
			data->set_failed(ErrorType::InvalidData);
		}
	} else {
		data->set_failed(ErrorType::InvalidID);
	}
}

template<typename T>
template<bool Async>
void Loader<T>::load_internal(AssetPtrBase<T>& ptr) {
	const AssetId id = ptr.id();
	if(id == AssetId::invalid_id()) {
		return;
	}

	{
		const auto lock = y_profile_unique_lock(_lock);
		auto& weak = _loaded[id];
		ptr = weak.lock();
		if(ptr._data) {
			return;
		}
		weak = (ptr = std::make_shared<Data>(id, this))._data;
	}

	if constexpr(Async) {
		parent()->_threads.schedule([data = ptr._data, this] { load_func(data); });
	} else {
		load_func(ptr._data);
	}
}



template<typename T>
AssetPtr<T> AssetLoader::load(AssetId id) {
	return loader_for_type<T>().load(id);
}

template<typename T>
AsyncAssetPtr<T> AssetLoader::load_async(AssetId id) {
	return loader_for_type<T>().load_async(id);
}

template<typename T>
std::future<AssetLoader::Result<T>> AssetLoader::load_future(AssetId id) {
	return _threads.schedule_with_future([this, id]() -> Result<T> {
		auto ptr = load<T>(id);
		if(ptr.is_failed()) {
			return core::Err(ptr.error());
		}
		return core::Ok(std::move(ptr));
	});
}

template<typename T>
AssetLoader::Result<T> AssetLoader::load_res(AssetId id) {
	auto ptr = load<T>(id);
	if(ptr.is_failed()) {
		return core::Err(ptr.error());
	}
	return core::Ok(std::move(ptr));
}

template<typename T>
AssetLoader::Result<T> AssetLoader::load_res(std::string_view name) {
	return load<T>(store().id(name));
}

template<typename T>
AssetLoader::Result<T> AssetLoader::import(std::string_view name, std::string_view import_from) {
	return load<T>(load_or_import(name, import_from, AssetTraits<T>::type));
}

template<typename T, typename E>
AssetLoader::Result<T> AssetLoader::load(core::Result<AssetId, E> id) {
	if(id) {
		return load_res<T>(id.unwrap());
	}
	return core::Err(ErrorType::UnknownID);
}

template<typename T, typename Raw>
Loader<Raw>& AssetLoader::loader_for_type() {
	const auto lock = y_profile_unique_lock(_lock);
	auto& loader = _loaders[typeid(Raw)];
	if(!loader) {
		loader = std::make_unique<Loader<Raw>>(this);
	}
	return *dynamic_cast<Loader<Raw>*>(loader.get());
}

}

#endif // YAVE_ASSETS_ASSETLOADER_INL
