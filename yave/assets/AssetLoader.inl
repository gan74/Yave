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
void AssetPtr<T>::post_deserialize(AssetLoadingContext& context)  {
	const AssetId id = _id;
	if(id == AssetId::invalid_id()) {
		return;
	}

	*this = context.load_async<T>(id);
	flush_reload();
}





// --------------------------- Loader ---------------------------

template<typename T>
Loader<T>::Loader(AssetLoader* parent) : LoaderBase(parent) {
	y_always_assert(parent, "Parent should not be null.");
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
bool Loader<T>::find_ptr(AssetPtr<T>& ptr) {
	const AssetId id = ptr.id();
	if(id == AssetId::invalid_id()) {
		return true;
	}

	const auto lock = y_profile_unique_lock(_lock);
	auto& weak = _loaded[id];
	ptr = weak.lock();
	if(ptr._data) {
		return true;
	}
	weak = (ptr = std::make_shared<Data>(id, this))._data;
	return false;
}


template<typename T>
AssetPtr<T> Loader<T>::load(AssetId id) {
	y_profile();
	auto ptr = load_async(id);
	parent()->wait_until_loaded(ptr);
	y_debug_assert(!ptr.is_loading());
	return ptr;

}

template<typename T>
AssetPtr<T> Loader<T>::load_async(AssetId id) {
	y_profile();
	AssetPtr<T> ptr(id);
	if(!find_ptr(ptr)) {
		parent()->_thread_pool.add_loading_job(ptr, read_func(ptr._data));
	}
	return ptr;
}

template<typename T>
AssetPtr<T> Loader<T>::reload(const AssetPtr<T>& ptr) {
	y_profile();

	const AssetId id = ptr.id();
	if(id == AssetId::invalid_id()) {
		return AssetPtr<T>();
	}

	y_always_assert(!ptr.is_empty(), "Can not reload empty asset");

	auto data = std::make_shared<Data>(id, this);
	AssetLoadingContext loading_ctx(parent());
	read_func(data)(loading_ctx)();

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
typename Loader<T>::ReadFunc Loader<T>::read_func(const std::shared_ptr<Data>& data) {
	return [data, this](AssetLoadingContext& loading_ctx) -> CreateFunc {
		y_always_assert(data && data->is_loading(), "Asset is not in a loading state");
		y_always_assert(parent() == loading_ctx.parent(), "Mismatched AssetLoaders");

		const AssetId id = data->id;

		y_always_assert(id != AssetId::invalid_id(), "Invalid asset ID");
		if(auto reader = store().data(id)) {
			y_profile_zone("loading");

			LoadFrom load_from;
			serde3::ReadableArchive arc(std::move(reader.unwrap()));
			if(arc.deserialize(load_from, loading_ctx)) {
				struct { mutable LoadFrom data; } box { std::move(load_from) };
				return [from = std::move(box), data, this] {
					data->finalize_loading(T(device(), std::move(from.data)));
					y_debug_assert(!data->is_loading());
				};
			} else {
				data->set_failed(ErrorType::InvalidData);
			}
		}
		data->set_failed(ErrorType::InvalidID);
		y_debug_assert(!data->is_loading());
		return []() {};
	};
}








// --------------------------- AssetLoader ---------------------------

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
AssetPtr<T> AssetLoader::load(AssetId id) {
	return loader_for_type<T>().load(id);
}

template<typename T>
AssetPtr<T> AssetLoader::load_async(AssetId id) {
	return loader_for_type<T>().load_async(id);
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

template<typename T>
Loader<T>& AssetLoader::loader_for_type() {
	const auto lock = y_profile_unique_lock(_lock);
	auto& loader = _loaders[typeid(T)];
	if(!loader) {
		loader = std::make_unique<Loader<T>>(this);
	}
	return *dynamic_cast<Loader<T>*>(loader.get());
}








// --------------------------- AssetLoadingContext ---------------------------

template<typename T>
AssetPtr<T> AssetLoadingContext::load(AssetId id) {
	auto ptr = _parent->load<T>(id);
	_dependencies.add_dependency(ptr);
	return ptr;
}

template<typename T>
AssetPtr<T> AssetLoadingContext::load_async(AssetId id) {
	auto ptr = _parent->load_async<T>(id);
	_dependencies.add_dependency(ptr);
	return ptr;
}


}

#endif // YAVE_ASSETS_ASSETLOADER_INL
