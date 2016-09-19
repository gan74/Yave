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
#ifndef YAVE_MATERIAL_MATERIAL_H
#define YAVE_MATERIAL_MATERIAL_H


#include <yave/yave.h>
#include <yave/Viewport.h>
#include <yave/image/Sampler.h>

#include <y/core/AssocVector.h>

#include "GraphicPipeline.h"
#include "MaterialData.h"

namespace yave {

class RenderPass;

class Material : NonCopyable, public DeviceLinked {

	public:
		Material() = default;
		Material(DevicePtr dptr, const MaterialData& data);

		Material(Material&& other);
		Material& operator=(Material&& other);

		~Material();


		const GraphicPipeline& compile(const RenderPass& render_pass, const Viewport &viewport);

		const MaterialData& get_data() const;

		const vk::DescriptorSet& get_vk_descriptor_set() const;
		const vk::DescriptorSetLayout& get_vk_descriptor_set_layout() const;

	private:
		void swap(Material& other);

		MaterialData _data;
		Sampler _sampler;

		vk::DescriptorPool _pool;
		vk::DescriptorSetLayout _layout;
		vk::DescriptorSet _set;

		core::AssocVector<vk::RenderPass, GraphicPipeline> _compiled;
};

}


#endif // YAVE_MATERIAL_MATERIAL_H
