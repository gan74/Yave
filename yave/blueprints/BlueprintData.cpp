/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

#include "BlueprintData.h"

namespace yave {

BlueprintData BlueprintData::from_blueprint(Blueprint blueprint) {
    BlueprintData data;

    const core::Span nodes = blueprint.all_nodes();
    for(usize dst_index = 0; dst_index != nodes.size(); ++dst_index) {
        const BlueprintNode* dst = nodes[dst_index].get();
        const usize input_count = dst->input_pins().size();
        for(usize pin = 0; pin != input_count; ++pin) {
            if(const void* in = dst->input(pin)) {
                const auto [src, src_pin] = blueprint.find_output(in);
                y_debug_assert(src);

                usize src_index = 0;
                for(; src_index != nodes.size(); ++src_index) {
                    if(nodes[src_index].get() == src) {
                        break;
                    }
                }
                y_debug_assert(src_index != nodes.size());

                data._links << BlueprintLink{
                    u32(src_index),
                    u32(src_pin),
                    u32(dst_index),
                    u32(pin),
                };
            }
        }
    }

    data._nodes = std::move(blueprint._nodes);
    return data;
}

}
