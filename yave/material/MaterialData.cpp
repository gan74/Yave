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

#include "MaterialData.h"

namespace yave {

MaterialData& MaterialData::set_frag_data(SpirVData&& data) {
	_frag = std::move(data);
	return *this;
}

MaterialData& MaterialData::set_vert_data(SpirVData&& data) {
	_vert = std::move(data);
	return *this;
}

MaterialData& MaterialData::set_geom_data(SpirVData&& data) {
	_geom = std::move(data);
	return *this;
}

MaterialData& MaterialData::set_bindings(const core::Vector<Binding>& binds) {
	_bindings = binds;
	return *this;
}


}
