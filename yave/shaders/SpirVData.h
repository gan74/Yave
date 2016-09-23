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
#ifndef YAVE_MATERIAL_SPIRVDATA_H
#define YAVE_MATERIAL_SPIRVDATA_H

#include <yave/yave.h>
#include <y/io/Ref.h>

#include "ShaderResource.h"

namespace yave {

class SpirVData {

	public:
		SpirVData() = default;

		static SpirVData from_file(io::ReaderRef reader);

		usize size() const;
		bool is_empty() const;

		const u32* data() const;

		const core::Vector<ShaderResource>& shader_resources() const;
		vk::ShaderStageFlags shader_stage() const;

	private:
		SpirVData(const core::Vector<u32>& data);

		core::Vector<u32> _data;
		vk::ShaderStageFlags _stage;
		core::Vector<ShaderResource> _resources;
};

}

#endif // YAVE_MATERIAL_SPIRVDATA_H
