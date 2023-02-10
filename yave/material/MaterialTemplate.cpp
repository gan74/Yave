/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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
#include "MaterialTemplate.h"
#include "MaterialCompiler.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/framebuffer/RenderPass.h>
#include <yave/graphics/device/extensions/DebugUtils.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <mutex>

namespace yave {

MaterialTemplate::MaterialTemplate(MaterialTemplateData&& data) : _data(std::move(data)) {
}

const GraphicPipeline& MaterialTemplate::compile(const RenderPass& render_pass) const {
    Y_TODO(make material compilation thread safe?)
    if(!render_pass.vk_render_pass()) {
        y_fatal("Unable to compile material: null renderpass");
    }

    const auto& key = render_pass.layout();
    const auto it = _compiled.find(key);
    if(it == _compiled.end()) {
        if(_compiled.size() == max_compiled_pipelines) {
            log_msg("Discarding graphic pipeline", Log::Warning);
            std::move(_compiled.begin() + 1, _compiled.end(), _compiled.begin());
            _compiled.pop();
        }

        _compiled.insert(key, MaterialCompiler::compile(this, render_pass));

#ifdef Y_DEBUG
        if(const auto* debug = debug_utils(); debug && !_name.is_empty()) {
            debug->set_resource_name(_compiled.last().second.vk_pipeline(), _name.data());
        }
#endif

        return _compiled.last().second;
    }
    return it->second;
}


const MaterialTemplateData& MaterialTemplate::data() const {
    return _data;
}

void MaterialTemplate::set_name(const char* name) {
    unused(name);
#ifdef Y_DEBUG
    _name = name;
#endif
}

}

