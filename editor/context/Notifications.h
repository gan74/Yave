/*******************************
Copyright (c) 2016-2021 Gr�goire Angerand

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
#ifndef EDITOR_CONTEXT_NOTIFICATIONS_H
#define EDITOR_CONTEXT_NOTIFICATIONS_H

#include <editor/ui/Widget.h>

#include <y/utils/iter.h>

#include <mutex>
#include <memory>

namespace editor {

class Notifications : public ContextLinked {

    class Tqdm {
        struct Data {
            usize it = 0;
            usize size = 0;
            core::String msg;

            bool is_over() const {
                return it >= size;
            }

            float fraction() const {
                return float(it) / float(size);
            }
        };

        public:
            void update(u32 id, usize value);
            u32 create(usize size, core::String msg);

            core::Vector<Data> progress_items() const;

        private:
            u32 _first_id = 0;
            core::Vector<Data> _progress;

            mutable std::mutex _lock;
    };

    public:
        Notifications(ContextPtr ctx);

        auto progress_items() const {
            return _tqdm->progress_items();
        }

        template<typename C>
        decltype(auto) tqdm(C&& c, core::String msg) {
            const u32 id = _tqdm->create(c.size(), std::move(msg));
            auto update = [id, i = usize(0), tqdm = _tqdm](auto&&) mutable { tqdm->update(id, ++i); return true; };
            return core::Range(FilterIterator(c.begin(), c.end(), update), EndIterator());
        }

    private:
        std::shared_ptr<Tqdm> _tqdm;
};

}

#endif // EDITOR_CONTEXT_NOTIFICATIONS_H

