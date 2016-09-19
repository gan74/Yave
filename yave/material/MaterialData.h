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
#ifndef YAVE_MATERIAL_MATERIALDATA_H
#define YAVE_MATERIAL_MATERIALDATA_H

#include <yave/yave.h>
#include <yave/UniformBinding.h>
#include <yave/TextureBinding.h>

#include "SpirVData.h"

namespace yave {

struct MaterialData {

	SpirVData _frag;
	SpirVData _vert;
	SpirVData _geom;

	core::Vector<UniformBinding> _ub_bindings;
	core::Vector<TextureBinding> _tx_bindings;

	MaterialData& set_frag_data(SpirVData&& data);
	MaterialData& set_vert_data(SpirVData&& data);
	MaterialData& set_geom_data(SpirVData&& data);

	MaterialData& set_uniform_buffers(const core::Vector<UniformBinding>& binds);
	MaterialData& set_textures(const core::Vector<TextureBinding>& binds);
};

}

#endif // YAVE_MATERIAL_MATERIALDATA_H
