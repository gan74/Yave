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
#ifndef YAVE_ASSETS_ASSETPTR_INL
#define YAVE_ASSETS_ASSETPTR_INL

#ifndef YAVE_ASSETS_ASSETPTR_H
#error this file should not be included directly

// Just to help the IDE
#include "AssetPtr.h"
#endif


namespace yave {

template<typename T, typename... Args>
AssetPtr<T> make_asset(Args&&... args) {
	return AssetPtr<T>(AssetId::invalid_id(), nullptr, T(y_fwd(args)...));
}

template<typename T, typename... Args>
AssetPtr<T> make_asset_with_id(AssetId id, Args&&... args) {
	return AssetPtr<T>(id, nullptr, T(y_fwd(args)...));
}



namespace detail {
AssetPtrDataBase::AssetPtrDataBase(AssetId i, AssetLoadingState s) : id(i), _state(s) {
}

void AssetPtrDataBase::set_failed(AssetLoadingErrorType error) {
	y_debug_assert(_state == AssetLoadingState::NotLoaded);
	_state = AssetLoadingState(u32(AssetLoadingState::Failed) + u32(error));
	y_debug_assert(is_failed());
}

AssetLoadingErrorType AssetPtrDataBase::error() const {
	y_debug_assert(is_failed());
	const u32 uint_state = u32(_state.load());
	return AssetLoadingErrorType(uint_state - u32(AssetLoadingState::Failed));
}

bool AssetPtrDataBase::is_loaded() const {
	return _state == AssetLoadingState::Loaded;
}

bool AssetPtrDataBase::is_failed() const {
	return _state >= AssetLoadingState::Failed;
}

bool AssetPtrDataBase::is_loading() const {
	return _state == AssetLoadingState::NotLoaded;
}


template<typename T>
AssetPtrData<T>::AssetPtrData(AssetId i, LoaderBase* l) : AssetPtrDataBase(i, AssetLoadingState::NotLoaded), loader(l) {
}

template<typename T>
AssetPtrData<T>::AssetPtrData(AssetId i, LoaderBase* l, T t) : AssetPtrDataBase(i, AssetLoadingState::Loaded), asset(std::move(t)), loader(l) {
}

template<typename T>
void AssetPtrData<T>::finalize_loading(T t) {
	y_debug_assert(_state == AssetLoadingState::NotLoaded);
	asset = std::move(t);
	_state = AssetLoadingState::Loaded;
	y_debug_assert(!is_loading());
}
}





template<typename T>
AssetPtr<T>::AssetPtr(AssetId id) : _id(id) {
}

template<typename T>
AssetPtr<T>::AssetPtr(AssetId id, LoaderBase* loader) : AssetPtr(std::make_shared<Data>(id, loader)) {
}

template<typename T>
AssetPtr<T>::AssetPtr(AssetId id, LoaderBase* loader, T asset) : AssetPtr(std::make_shared<Data>(id, loader, std::move(asset))) {
}

template<typename T>
AssetPtr<T>::AssetPtr(std::shared_ptr<Data> ptr) : _data(std::move(ptr)) {
	if(_data) {
		_id = _data->id;
		if(_data->is_loaded()) {
			flush_reload();
			y_debug_assert(_data->is_loaded());
		}
	}
}

template<typename T>
bool AssetPtr<T>::is_empty() const {
	return !_data;
}

template<typename T>
bool AssetPtr<T>::has_loader() const {
	y_debug_assert(!_data || _data->loader);
	return !!_data;
}

template<typename T>
bool AssetPtr<T>::is_loaded() const {
	return _data && _data->is_loaded();
}

template<typename T>
bool AssetPtr<T>::is_loading() const {
	return _data && _data->is_loading();
}

template<typename T>
bool AssetPtr<T>::is_failed() const {
	return _data && _data->is_failed();
}

template<typename T>
AssetLoadingErrorType AssetPtr<T>::error() const {
	y_debug_assert(is_failed());
	return _data->error();
}

template<typename T>
bool AssetPtr<T>::flush_reload() {
	if(_data) {
		y_debug_assert(_data->id == _id);
		if(auto reloaded = std::atomic_load(&_data->reloaded)) {
			y_debug_assert(reloaded->is_loaded());
			y_debug_assert(reloaded->id == _id);
			*this = reloaded;
			flush_reload(); // in case the we reloaded several time
			return true;
		}
	}
	return false;
}

template<typename T>
AssetId AssetPtr<T>::id() const {
	y_debug_assert(!_data || _data->id == _id);
	return _id;
}

template<typename T>
const T* AssetPtr<T>::get() const {
	return !is_loaded() ? nullptr : &_data->asset;
}

template<typename T>
const T& AssetPtr<T>::operator*() const {
	return *get();
}

template<typename T>
const T* AssetPtr<T>::operator->() const {
	return get();
}

template<typename T>
AssetPtr<T>::operator bool() const {
	return is_loaded();
}

template<typename T>
bool AssetPtr<T>::operator==(const AssetPtr& other) const {
	return _id == other._id;
}

template<typename T>
bool AssetPtr<T>::operator!=(const AssetPtr& other) const {
	return _id != other._id;
}



}

#endif // YAVE_ASSETS_ASSETPTR_INL
