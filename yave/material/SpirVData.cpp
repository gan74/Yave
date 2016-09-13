/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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
#include "SpirVData.h"

namespace yave {

SpirVData SpirVData::from_file(const core::String& file) {
	auto file_data = read_file(file);
	u32* data = new u32[file_data.size() / sizeof(u32) + 1];
	memcpy(data, file_data.begin(), file_data.size());
	return SpirVData(file_data.size(), data);
}

SpirVData::SpirVData() : _len(0), _data(nullptr) {
}

SpirVData::SpirVData(usize l, u32* owned_data) : _len(l), _data(owned_data) {
}

SpirVData::SpirVData(SpirVData&& other) : SpirVData() {
	swap(other);
}

SpirVData& SpirVData::operator=(SpirVData&& other) {
	swap(other);
	return *this;
}

void SpirVData::swap(SpirVData& other) {
	std::swap(_len, other._len);
	std::swap(_data, other._data);
}

SpirVData::~SpirVData() {
	delete[] _data;
}

usize SpirVData::size() const {
	return _len;
}

const u32* SpirVData::data() const {
	return _data;
}

}
