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
#include "Material.h"

namespace yave {

Material::Material() {
}

const SpirVData& Material::get_frag_data() const {
	return _frag;
}

const SpirVData& Material::get_vert_data() const {
	return _vert;
}

const SpirVData& Material::get_geom_data() const {
	return _geom;
}



Material& Material::set_frag_data(SpirVData&& data) {
	_frag = std::move(data);
	return *this;
}

Material& Material::set_vert_data(SpirVData&& data) {
	_vert = std::move(data);
	return *this;
}

Material& Material::set_geom_data(SpirVData&& data) {
	_geom = std::move(data);
	return *this;
}

Material& Material::set_uniform_buffers(const core::Vector<UniformBinding>& binds) {
	_ub_bindings = binds;
	return *this;
}

Material& Material::set_textures(const core::Vector<TextureBinding>& binds) {
	_tx_bindings = binds;
	return *this;
}

}
