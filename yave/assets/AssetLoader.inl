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
	if(_id == AssetId::invalid_id()) {
		return;
	}

	*this = context.load_async<T>(_id);
	y_debug_assert(_data && _data->id == _id);
	flush_reload();
}



// --------------------------- Loader ---------------------------

template<typename T>
Loader<T>::Loader(AssetLoader* parent) : LoaderBase(parent) {
}


template<typename T>
typename Loader<T>::Result Loader<T>::load(AssetId id) {
	AssetPtr<T> ptr(id);
	load_internal<false>(ptr, false);
	if(ptr.is_failed()) {
		return core::Err(ptr.error());
	}
	Y_TODO(we might already be loading asynchronously, maybe do something to prevent that or to flush)
	return core::Ok(std::move(ptr));
}

template<typename T>
typename Loader<T>::Result Loader<T>::reload(const AssetPtr<T>& ptr) {
	AssetPtr<T> reloaded(ptr.id(), this);
	load_internal<true>(reloaded, false);
	if(reloaded.is_failed()) {
		return core::Err(reloaded.error());
	}
	return core::Ok(std::move(reloaded));
}


template<typename T>
AssetPtr<T> Loader<T>::load_async(AssetId id) {
	AssetPtr<T> ptr(id);
	load_internal<false>(ptr, true);
	return ptr;
}

template<typename T>
AssetPtr<T> Loader<T>::reload_async(const AssetPtr<T>& ptr) {
	AssetPtr<T> reloaded(ptr.id(), this);
	load_internal<true>(reloaded, true);
	return reloaded;
}

template<typename T>
template<bool Reload>
void Loader<T>::load_internal(AssetPtr<T>& ptr, bool async) {
	const AssetId id = ptr.id();
	if(id == AssetId::invalid_id()) {
		return;
	}

	if(!Reload) {
		const auto lock = y_profile_unique_lock(_lock);
		auto& weak = _loaded[id];
		ptr = weak.lock();
		if(ptr.is_empty()) {
			ptr = AssetPtr<T>(id, this);
			weak = ptr;
		} else {
			return;
		}
	}

	core::Function<void()> load = [ptr, async, id, this]() {
		y_debug_assert(ptr.is_loading());
		if(auto reader = store().data(id)) {
			y_profile_zone("loading");

			/*if(async) {
				core::Duration::sleep(core::Duration::seconds(2));
			}*/

			load_from_t load_from;
			AssetLoadingContext loading_ctx(*parent(), id);
			serde3::ReadableArchive arc(std::move(reader.unwrap()));
			if(arc.deserialize(load_from, loading_ctx)) {
				ptr._data->finalize_loading(T(device(), std::move(load_from)));
				ptr.flush();
			} else {
				ptr._data->set_failed(ErrorType::InvalidData);
			}
		} else {
			ptr._data->set_failed(ErrorType::InvalidID);
		}

		if(Reload) {
			const auto lock = y_profile_unique_lock(_lock);
			auto& orig = _loaded[id];
			if(auto data = orig.lock()._data) {
				data->set_reloaded(ptr);
				orig = std::move(ptr);
			}
		}
	};

	if(async) {
		parent()->_threads.schedule(std::move(load));
	} else {
		load();
		ptr.flush();
	}
}



template<typename T>
AssetPtr<T> AssetLoader::load_async(AssetId id) {
	return loader_for_type<T>().load_async(id);
}

template<typename T>
std::future<AssetLoader::Result<T>> AssetLoader::load_future(AssetId id) {
	return _threads.schedule_with_future([this, id]() -> Result<T> { return load<T>(id); });
}

template<typename T>
AssetLoader::Result<T> AssetLoader::load(AssetId id) {
	return loader_for_type<T>().load(id);
}

template<typename T>
AssetLoader::Result<T> AssetLoader::load(std::string_view name) {
	return load<T>(store().id(name));
}

template<typename T>
AssetLoader::Result<T> AssetLoader::import(std::string_view name, std::string_view import_from) {
	return load<T>(load_or_import(name, import_from, AssetTraits<T>::type));
}

template<typename T, typename E>
AssetLoader::Result<T> AssetLoader::load(core::Result<AssetId, E> id) {
	if(id) {
		return load<T>(id.unwrap());
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
