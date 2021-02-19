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
#ifndef YAVE_FRAMEGRAPH_FRAMEGRAPHRECOURCEID_H
#define YAVE_FRAMEGRAPH_FRAMEGRAPHRECOURCEID_H

#include <yave/yave.h>

#include <y/utils/hash.h>

namespace yave {

class FrameGraphResourceId {
    public:
        constexpr FrameGraphResourceId() = default;

        u32 id() const {
            return u32(_id);
        }

        bool operator==(const FrameGraphResourceId& other) const {
            return _id == other._id;
        }

        bool is_valid() const {
            return _id != invalid_id;
        }

        void check_valid() const {
            if(!is_valid()) {
                y_fatal("Invalid resource.");
            }
        }

    protected:
        friend class FrameGraph;

        static constexpr u32 invalid_id = u32(-1);

        u32 _id = invalid_id;


        template<typename T>
        constexpr T convert() const {
            static_assert(std::is_base_of_v<FrameGraphResourceId, T>);
            T t;
            t._id = _id;
            return t;
        }
};

struct FrameGraphMutableResourceId : FrameGraphResourceId {
};

// ---------------------------- images ----------------------------
struct FrameGraphImageId : FrameGraphResourceId {
};

struct FrameGraphMutableImageId : FrameGraphMutableResourceId {
    constexpr operator FrameGraphImageId() const {
        return convert<FrameGraphImageId>();
    }
};




// ---------------------------- buffers ----------------------------
struct FrameGraphBufferId : FrameGraphResourceId {
};

struct FrameGraphMutableBufferId : FrameGraphMutableResourceId {
    constexpr operator FrameGraphBufferId() const {
        return convert<FrameGraphBufferId>();
    }
};



// ---------------------------- typed buffers ----------------------------
template<typename T>
struct FrameGraphTypedBufferId : FrameGraphResourceId {
    operator FrameGraphBufferId() const {
        return convert<FrameGraphBufferId>();
    }
};

template<typename T>
struct FrameGraphMutableTypedBufferId : FrameGraphMutableResourceId {
    constexpr operator FrameGraphMutableBufferId() const {
        return convert<FrameGraphMutableBufferId>();
    }

    constexpr operator FrameGraphBufferId() const {
        return convert<FrameGraphBufferId>();
    }

    constexpr operator FrameGraphTypedBufferId<T>() const {
        return convert<FrameGraphTypedBufferId<T>>();
    }

    static constexpr FrameGraphMutableTypedBufferId<T> from_untyped(FrameGraphMutableBufferId res) {
        FrameGraphMutableTypedBufferId t;
        static_cast<FrameGraphResourceId&>(t) = res;
        return t;
    }
};

}



namespace std {
template<>
struct hash<yave::FrameGraphResourceId> : hash<y::u32>{
    auto operator()(yave::FrameGraphResourceId r) const {
        return hash<y::u32>::operator()(r.id());
    }
};
}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHRECOURCEID_H

