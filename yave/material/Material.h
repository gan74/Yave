/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#ifndef YAVE_MATERIAL_MATERIAL_H
#define YAVE_MATERIAL_MATERIAL_H

#include <yave/yave.h>
#include <yave/Viewport.h>

#include <yave/bindings/DescriptorSet.h>

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

		const MaterialData& data() const;
		const DescriptorSet& descriptor_set() const;

	private:
		void swap(Material& other);

		MaterialData _data;

		DescriptorSet _set;

		core::AssocVector<vk::RenderPass, GraphicPipeline> _compiled;
};

}


#endif // YAVE_MATERIAL_MATERIAL_H
