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
#ifndef EDITOR_WIDGETS_PERFORMANCEMETRICS_H
#define EDITOR_WIDGETS_PERFORMANCEMETRICS_H

#include <editor/Widget.h>

#include <y/core/FixedArray.h>
#include <y/core/Chrono.h>


namespace editor {

class PerformanceMetrics : public Widget {

    editor_widget(PerformanceMetrics, "View", "Statistics")

    class PlotData {
        public:
            PlotData(core::Duration total_duration = core::Duration::seconds(60.0), usize size = 256);

            void push(float value);
            float last();

            core::Span<float> values() const;
            usize value_count() const;
            usize current_index() const;
            usize next_index() const;
            float max() const;
            float average() const;

        private:
            void advance();

            core::Chrono _timer;

            core::FixedArray<float> _data;
            usize _current = 0;
            bool _full = false;

            float _max = 0.0f;
            double _total = 0.0;

            double _duration = 1.0;
    };

    public:
        PerformanceMetrics();

    protected:
        void on_gui() override;

        bool before_gui() override;

    private:
        void draw_timings();
        void draw_memory();

        core::Chrono _timer;

        PlotData _frames;
        PlotData _average;
        PlotData _memory;
};

}

#endif // EDITOR_WIDGETS_PERFORMANCEMETRICS_H

