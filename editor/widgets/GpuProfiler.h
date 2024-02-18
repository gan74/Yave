/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#ifndef EDITOR_WIDGETS_GPUPROFILER_H
#define EDITOR_WIDGETS_GPUPROFILER_H

#include <editor/Widget.h>

#include <y/core/HashMap.h>

namespace editor {

class GpuProfiler final : public Widget {
    editor_widget(GpuProfiler, "View")

    struct ZoneHistory {
        usize sample_count = 0;

        double gpu_ms_sum = 0.0;
        double gpu_ms_max = 0.0;

        double cpu_ms_sum = 0.0;
        double cpu_ms_max = 0.0;

        void update(double gpu_ms, double cpu_ms) {
            ++sample_count;

            gpu_ms_sum += gpu_ms;
            gpu_ms_max = std::max(gpu_ms, gpu_ms_max);

            cpu_ms_sum += cpu_ms;
            cpu_ms_max = std::max(cpu_ms, cpu_ms_max);
        }
    };

    public:
        GpuProfiler();

    protected:
        void on_gui() override;

    private:
        core::FlatHashMap<i64, ZoneHistory> _history;
        bool _tree = true;
};

}

#endif // EDITOR_WIDGETS_GPUPROFILER_H
