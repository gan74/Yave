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
#ifndef YAVE_ASSETS_LOADER_INL
#define YAVE_ASSETS_LOADER_INL

#ifndef YAVE_ASSETS_LOADER_H
#error this file should not be included directly

// Just to help the IDE
#include "Loader.h"
#endif

namespace yave {

template<typename T>
void AssetPtr<T>::Data::set_reloaded(const AssetPtr<T>& other) {
	if(!other._data || other._data->id != id) {
		y_fatal("Invalid reload");
	}
	y_debug_assert(!other.is_loading());
	std::atomic_store(&reloaded, other._data);
}

template<typename T>
void AssetPtr<T>::reload() {
	y_debug_assert(!_data || _data->id == _id);
	if(_data && _data->loader) {
		flush_reload();
		dynamic_cast<Loader<T>*>(_data->loader)->reload(*this).ignore();
		flush_reload();
	}
}

template<typename T>
void AssetPtr<T>::reload_async() {
	y_debug_assert(!_data || _data->id == _id);
	if(_data && _data->loader) {
		flush_reload();
		dynamic_cast<Loader<T>*>(_data->loader)->reload_async(*this);
	}
}

template<typename T>
AssetType Loader<T>::type() const {
	return traits::type;
}

}

#endif // YAVE_ASSETS_LOADER_INL
