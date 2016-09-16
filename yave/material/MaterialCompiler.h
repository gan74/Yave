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
#ifndef YAVE_MATERIALCOMPILER_H
#define YAVE_MATERIALCOMPILER_H

#include <yave/yave.h>
#include "Material.h"
#include "GraphicPipeline.h"
#include "DescriptorSetBuilder.h"

#include <yave/RenderPass.h>
#include <yave/Viewport.h>

namespace yave {

class ShaderModule;

class MaterialCompiler : NonCopyable, public DeviceLinked {
	public:
		MaterialCompiler(DevicePtr dptr);

		GraphicPipeline compile(const Material& material, const RenderPass& render_pass, Viewport view) const;

	private:
		ShaderModule create_shader_module(const SpirVData& data) const;

		DescriptorSetBuilder _ds_builder;

};


}

#endif // YAVE_MATERIALCOMPILER_H
