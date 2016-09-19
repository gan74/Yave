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
#ifndef YAVE_MATERIAL_H
#define YAVE_MATERIAL_H


#include <yave/yave.h>
#include <yave/Viewport.h>
#include <yave/UniformBinding.h>
#include <yave/TextureBinding.h>

#include <y/core/AssocVector.h>

#include "GraphicPipeline.h"
#include "SpirVData.h"

namespace yave {

class RenderPass;

class Material : NonCopyable, public DeviceLinked {

	public:
		Material() = default;
		Material(DevicePtr dptr);

		Material& set_frag_data(SpirVData&& data);
		Material& set_vert_data(SpirVData&& data);
		Material& set_geom_data(SpirVData&& data);

		Material& set_uniform_buffers(const core::Vector<UniformBinding>& binds);
		Material& set_textures(const core::Vector<TextureBinding>& binds);

		const SpirVData& get_frag_data() const;
		const SpirVData& get_vert_data() const;
		const SpirVData& get_geom_data() const;

		const auto& get_uniform_bindings() const {
			return _ub_bindings;
		}

		const auto& get_texture_bindings() const {
			return _tx_bindings;
		}

		usize binding_count() const {
			return _ub_bindings.size() + _tx_bindings.size();
		}

		const GraphicPipeline& compile(const RenderPass& render_pass, const Viewport &viewport);
		const MaterialDescriptorSet& get_descriptor_set() const;

	private:
		SpirVData _frag;
		SpirVData _vert;
		SpirVData _geom;

		core::Vector<UniformBinding> _ub_bindings;
		core::Vector<TextureBinding> _tx_bindings;

		mutable MaterialDescriptorSet _d_set;
		core::AssocVector<vk::RenderPass, GraphicPipeline> _compiled;
};

}


#endif // YAVE_MATERIAL_H
