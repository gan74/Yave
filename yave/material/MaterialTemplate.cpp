/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
#include "Material.h"
#include "MaterialCompiler.h"

#include <yave/device/Device.h>

#include <mutex>

namespace yave {

MaterialTemplate::MaterialTemplate(DevicePtr dptr, MaterialTemplateData&& data) :
		DeviceLinked(dptr),
		_data(std::move(data)) {
}

const GraphicPipeline& MaterialTemplate::compile(const RenderPass& render_pass) const {
#warning MaterialTemplate::compile not thread safe
	if(!render_pass.vk_render_pass()) {
		y_fatal("Unable to compile material: null renderpass.");
	}

	const auto& key = render_pass.layout();
	auto it = _compiled.find(key);
	if(it == _compiled.end()) {
		if(_compiled.size() == max_compiled_pipelines) {
			log_msg("Discarding graphic pipeline", Log::Warning);
			std::move(_compiled.begin() + 1, _compiled.end(), _compiled.begin());
			_compiled.pop();
		}

		MaterialCompiler compiler(device());
		_compiled.insert(key, compiler.compile(this, render_pass));
		return _compiled.last().second;
	}
	return it->second;
}


const MaterialTemplateData& MaterialTemplate::data() const {
	return _data;
}

}
