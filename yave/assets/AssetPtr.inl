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
template<typename T>
AssetPtrData<T>::AssetPtrData(AssetId i, LoaderBase* l) : id(i), loader(l), state(AssetLoadingState::NotLoaded) {
}

template<typename T>
AssetPtrData<T>::AssetPtrData(AssetId i, LoaderBase* l, T t) : asset(std::move(t)), id(i), loader(l), state(AssetLoadingState::Loaded) {
}

template<typename T>
void AssetPtrData<T>::finalize_loading(T t) {
	y_debug_assert(state == AssetLoadingState::NotLoaded);
	asset = std::move(t);
	state = AssetLoadingState::Loaded;
}

template<typename T>
void AssetPtrData<T>::set_failed(AssetLoadingErrorType error) {
	y_debug_assert(state == AssetLoadingState::NotLoaded);
	state = AssetLoadingState(u32(AssetLoadingState::Failed) + u32(error));
}

template<typename T>
AssetLoadingErrorType AssetPtrData<T>::error() const {
	y_debug_assert(is_failed());
	const u32 uint_state = u32(state.load());
	return AssetLoadingErrorType(uint_state - u32(AssetLoadingState::Failed));
}

template<typename T>
bool AssetPtrData<T>::is_loaded() const {
	return state == AssetLoadingState::Loaded;
}

template<typename T>
bool AssetPtrData<T>::is_failed() const {
	return state >= AssetLoadingState::Failed;
}
}





template<typename T>
AssetPtrBase<T>::AssetPtrBase(AssetId id) : _id(id) {
}

template<typename T>
AssetPtrBase<T>::AssetPtrBase(std::shared_ptr<Data> ptr) : _data(std::move(ptr)) {
	if(_data) {
		_id = _data->id;
		if(_data->is_loaded()) {
			flush_reload();
			y_debug_assert(_data->is_loaded());
		}
	}
}

template<typename T>
bool AssetPtrBase<T>::is_empty() const {
	return !_data;
}

template<typename T>
bool AssetPtrBase<T>::has_loader() const {
	y_debug_assert(!_data || _data->loader);
	return !_data;
}

template<typename T>
bool AssetPtrBase<T>::is_loading() const {
	return _data && _data->state == AssetLoadingState::NotLoaded;
}

template<typename T>
bool AssetPtrBase<T>::is_failed() const {
	return _data && _data->is_failed();
}

template<typename T>
AssetLoadingErrorType AssetPtrBase<T>::error() const {
	y_debug_assert(_data && _data->is_failed());
	return _data->error();
}

template<typename T>
bool AssetPtrBase<T>::flush_reload() {
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
AssetId AssetPtrBase<T>::id() const {
	y_debug_assert(!_data || _data->id == _id);
	return _id; //_data ? _data->id : AssetId::invalid_id();
}




template<typename T>
AssetPtr<T>::AssetPtr(AssetId id) : AssetPtrBase<T>(id) {
}

template<typename T>
AssetPtr<T>::AssetPtr(AssetId id, LoaderBase* loader, T asset) : AssetPtr(std::make_shared<Data>(id, loader, std::move(asset))) {
}

template<typename T>
AssetPtr<T>::AssetPtr(std::shared_ptr<Data> ptr) : AssetPtrBase<T>(std::move(ptr)) {
	y_debug_assert(!this->is_loading());
}

template<typename T>
const T* AssetPtr<T>::get() const {
	return this->is_failed() ? nullptr : &this->_data->asset;
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
	return bool(this->_data);
}

template<typename T>
bool AssetPtr<T>::operator==(const AssetPtr& other) const {
	return this->_data == other._data;
}

template<typename T>
bool AssetPtr<T>::operator!=(const AssetPtr& other) const {
	return this->_data != other._data;
}







template<typename T>
AsyncAssetPtr<T>::AsyncAssetPtr(const AssetPtr<T>& other) : AsyncAssetPtr(other._data) {
}

template<typename T>
AsyncAssetPtr<T>::AsyncAssetPtr(AssetId id) : AssetPtrBase<T>(id) {
}

template<typename T>
AsyncAssetPtr<T>::AsyncAssetPtr(AssetId id, LoaderBase* loader) : AsyncAssetPtr(std::make_shared<Data>(id, loader)) {
}

template<typename T>
AsyncAssetPtr<T>::AsyncAssetPtr(AssetId id, LoaderBase* loader, T asset) : AsyncAssetPtr(std::make_shared<Data>(id, loader, std::move(asset))) {
}

template<typename T>
AsyncAssetPtr<T>::AsyncAssetPtr(std::shared_ptr<Data> ptr) : AssetPtrBase<T>(std::move(ptr)) {
	if(this->_data) {
		_ptr = &this->_data->asset;
	}
}

template<typename T>
const T* AsyncAssetPtr<T>::flushed() const {
	flush();
	return _ptr;
}

template<typename T>
void AsyncAssetPtr<T>::flush() const {
	if(should_flush()) {
		_ptr = &this->_data->asset;
	}
}

template<typename T>
bool AsyncAssetPtr<T>::is_loaded() const {
	return _ptr || (this->_data && this->_data->is_loaded());
}

template<typename T>
bool AsyncAssetPtr<T>::should_flush() const {
	return !_ptr && this->_data && this->_data->is_loaded();
}


template<typename T>
const T* AsyncAssetPtr<T>::get() const {
	return _ptr;
}

template<typename T>
const T& AsyncAssetPtr<T>::operator*() const {
	return *_ptr;
}

template<typename T>
const T* AsyncAssetPtr<T>::operator->() const {
	return _ptr;
}

template<typename T>
AsyncAssetPtr<T>::operator bool() const {
	return bool(_ptr);
}

template<typename T>
bool AsyncAssetPtr<T>::operator==(const AsyncAssetPtr& other) const {
	return this->_data == other._data;
}

template<typename T>
bool AsyncAssetPtr<T>::operator!=(const AsyncAssetPtr& other) const {
	return this->_data != other._data;
}

}

#endif // YAVE_ASSETS_ASSETPTR_INL
