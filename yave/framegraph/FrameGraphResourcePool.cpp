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

#include "FrameGraphResourcePool.h"

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

template<typename U>
static void check_usage(U u) {
    if(u == U::None) {
        y_fatal("Invalid resource usage.");
    }
}

FrameGraphResourcePool::FrameGraphResourcePool() {
}

FrameGraphResourcePool::~FrameGraphResourcePool() {
}

TransientImage FrameGraphResourcePool::create_image(ImageFormat format, const math::Vec2ui& size, ImageUsage usage) {
    y_profile();

    check_usage(usage);

    TransientImage image;
    if(create_image_from_pool(image, format, size, usage)) {
        return image;
    }

    y_profile_zone("create image");
    return TransientImage(format, usage, size);
}

TransientVolume FrameGraphResourcePool::FrameGraphResourcePool::create_volume(ImageFormat format, const math::Vec3ui& size, ImageUsage usage) {
    y_profile();

    check_usage(usage);

    TransientVolume volume;
    if(create_volume_from_pool(volume, format, size, usage)) {
        return volume;
    }

    y_profile_zone("create volume");
    return TransientVolume(format, usage, size);
}

TransientBuffer FrameGraphResourcePool::create_buffer(u64 byte_size, BufferUsage usage, MemoryType memory) {
    y_profile();

    check_usage(usage);

    if(memory == MemoryType::DontCare) {
        memory = prefered_memory_type(usage);
    }

    TransientBuffer buffer;
    if(create_buffer_from_pool(buffer, byte_size, usage, memory)) {
        return buffer;
    }

    y_profile_zone("create buffer");
    return TransientBuffer(byte_size, usage, memory);
}

bool FrameGraphResourcePool::create_image_from_pool(TransientImage& res, ImageFormat format, const math::Vec2ui& size, ImageUsage usage) {
    return _images.locked([&](auto&& images) {
        for(auto it = images.begin(); it != images.end(); ++it) {
            auto& img = it->first;

            if(img.format() == format && img.size() == size && img.usage() == usage) {

                res = std::move(img);
                images.erase(it);

                y_debug_assert(!res.is_null());

                return true;
            }
        }
        return false;
    });
}

bool FrameGraphResourcePool::create_volume_from_pool(TransientVolume& res, ImageFormat format, const math::Vec3ui& size, ImageUsage usage) {
    return _volumes.locked([&](auto&& volumes) {
        for(auto it = volumes.begin(); it != volumes.end(); ++it) {
            auto& vol = it->first;

            if(vol.format() == format && vol.size() == size && vol.usage() == usage) {

                res = std::move(vol);
                volumes.erase(it);

                y_debug_assert(!res.is_null());

                return true;
            }
        }
        return false;
    });
}

bool FrameGraphResourcePool::create_buffer_from_pool(TransientBuffer& res, usize byte_size, BufferUsage usage, MemoryType memory) {
    return _buffers.locked([&](auto&& buffers) {
        for(auto it = buffers.begin(); it != buffers.end(); ++it) {
            auto& buffer = it->first;

            if(buffer.byte_size() == byte_size &&
               buffer.usage() == usage &&
               (buffer.memory_type() == memory || memory == MemoryType::DontCare)) {

                res = std::move(buffer);
                buffers.erase(it);

                y_debug_assert(!res.is_null());

                return true;
            }
        }
        return false;
    });
}

void FrameGraphResourcePool::release(TransientImage image, FrameGraphPersistentResourceId persistent_id) {
    y_debug_assert(!image.is_null());
    if(persistent_id.is_valid()) {
        _persistent_images.locked([&](auto&& images) {
            const usize index = persistent_id.id();
            images.set_min_size(index + 1);
            if(!images[index].is_null()) {
                release(std::move(images[index]));
            }
            y_debug_assert(images[index].is_null());
            images[index] = std::move(image);
        });
    } else {
        _images.locked([&](auto&& images) { images.emplace_back(std::move(image), _frame_id); });
    }
}

void FrameGraphResourcePool::release(TransientVolume volume, FrameGraphPersistentResourceId persistent_id) {
    y_always_assert(!persistent_id.is_valid(), "Persistent volumes not supported");
    y_debug_assert(!volume.is_null());
    _volumes.locked([&](auto&& volumes) { volumes.emplace_back(std::move(volume), _frame_id); });
}

void FrameGraphResourcePool::release(TransientBuffer buffer, FrameGraphPersistentResourceId persistent_id) {
    y_debug_assert(!buffer.is_null());
    if(persistent_id.is_valid()) {
        _persistent_buffers.locked([&](auto&& buffers) {
            const usize index = persistent_id.id();
            buffers.set_min_size(index + 1);
            if(!buffers[index].is_null()) {
                release(std::move(buffers[index]));
            }
            y_debug_assert(buffers[index].is_null());
            buffers[index] = std::move(buffer);
        });
    } else {
        _buffers.locked([&](auto&& buffers) { buffers.emplace_back(std::move(buffer), _frame_id); });
    }
}

bool FrameGraphResourcePool::has_persistent_image(FrameGraphPersistentResourceId persistent_id) const {
    return _persistent_images.locked([&](auto&& images) {
        return persistent_id.id() < images.size() && !images[persistent_id.id()].is_null();
    });
}

bool FrameGraphResourcePool::has_persistent_buffer(FrameGraphPersistentResourceId persistent_id) const {
    return _persistent_buffers.locked([&](auto&& buffers) {
        return persistent_id.id() < buffers.size() && !buffers[persistent_id.id()].is_null();
    });
}

TransientImage FrameGraphResourcePool::persistent_image(FrameGraphPersistentResourceId persistent_id) {
    return _persistent_images.locked([&](auto&& images) {
        if(persistent_id.id() < images.size()) {
            TransientImage img(std::move(images[persistent_id.id()]));
            return img;
        }
        y_fatal("Persistent resource not found");
    });
}

TransientBuffer FrameGraphResourcePool::persistent_buffer(FrameGraphPersistentResourceId persistent_id) {
    return _persistent_buffers.locked([&](auto&& buffers) {
        if(persistent_id.id() < buffers.size()) {
            TransientBuffer buff(std::move(buffers[persistent_id.id()]));
            return buff;
        }
        y_fatal("Persistent resource not found");
    });
}

void FrameGraphResourcePool::garbage_collect() {
    y_profile();

    const u64 max_col_count = 6;
    const u64 frame_id = _frame_id++;

    _images.locked([&](auto&& images) {
        for(usize i = 0; i < images.size(); ++i) {
            if(images[i].second + max_col_count < frame_id) {
                images.erase(images.begin() + i);
                --i;
            }
        }
    });

    _volumes.locked([&](auto&& volumes) {
        for(usize i = 0; i < volumes.size(); ++i) {
            if(volumes[i].second + max_col_count < frame_id) {
                volumes.erase(volumes.begin() + i);
                --i;
            }
        }
    });

    _buffers.locked([&](auto&& buffers) {
        for(usize i = 0; i < buffers.size(); ++i) {
            if(buffers[i].second + max_col_count < frame_id) {
                buffers.erase(buffers.begin() + i);
                --i;
            }
        }
    });
}

u64 FrameGraphResourcePool::frame_id() const {
    return _frame_id;
}

}

