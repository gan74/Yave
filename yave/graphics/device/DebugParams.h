/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#ifndef YAVE_DEVICE_DEBUGPARAMS_H
#define YAVE_DEVICE_DEBUGPARAMS_H

#include <yave/yave.h>
#include <y/core/Span.h>

namespace yave {

class DebugParams {

    public:
        static DebugParams debug() {
            return DebugParams(true);
        }

        static DebugParams none() {
            return DebugParams(false);
        }

        bool debug_features_enabled() const {
            return _debug_enabled;
        }

        core::Span<const char*> instance_layers() const {
            if(_debug_enabled) {
                return layers;
            }
            return {};
        }

        core::Span<const char*> device_layers() const {
            if(_debug_enabled) {
                return layers;
            }
            return {};
        }

        void set_enabled(bool enabled) {
            _debug_enabled = enabled;
        }

    private:
        static constexpr std::array<const char*, 2> layers = {"VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor"};

        DebugParams(bool enabled) :
            _debug_enabled(enabled) {
        }


        bool _debug_enabled;
};

}

#endif // YAVE_DEVICE_DEBUGPARAMS_H

