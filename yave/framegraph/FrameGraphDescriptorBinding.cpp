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

#include "FrameGraphDescriptorBinding.h"
#include "FrameGraphFrameResources.h"

namespace yave {

FrameGraphDescriptorBinding::FrameGraphDescriptorBinding(const Descriptor& desc) : _type(BindingType::External), _external(desc) {
}

FrameGraphDescriptorBinding::FrameGraphDescriptorBinding(BindingType type, FrameGraphBufferId res) :
        _type(type), _buffer(res) {
    y_debug_assert(type == BindingType::InputBuffer || type == BindingType::StorageBuffer);
}

FrameGraphDescriptorBinding::FrameGraphDescriptorBinding(BindingType type, FrameGraphVolumeId res, SamplerType sampler) :
        _type(type), _volume({res, sampler}) {
    y_debug_assert(type == BindingType::InputVolume || type == BindingType::StorageVolume);
}

FrameGraphDescriptorBinding::FrameGraphDescriptorBinding(BindingType type, FrameGraphImageId res, SamplerType sampler) :
        _type(type), _image({res, sampler}) {
    y_debug_assert(type == BindingType::InputImage || type == BindingType::StorageImage);
}


FrameGraphDescriptorBinding FrameGraphDescriptorBinding::create_storage_binding(FrameGraphBufferId res) {
    return FrameGraphDescriptorBinding(BindingType::StorageBuffer, res);
}

FrameGraphDescriptorBinding FrameGraphDescriptorBinding::create_storage_binding(FrameGraphVolumeId res) {
    return FrameGraphDescriptorBinding(BindingType::StorageVolume, res);
}

FrameGraphDescriptorBinding FrameGraphDescriptorBinding::create_storage_binding(FrameGraphImageId res) {
    return FrameGraphDescriptorBinding(BindingType::StorageImage, res);
}

FrameGraphDescriptorBinding FrameGraphDescriptorBinding::create_uniform_binding(FrameGraphBufferId res) {
    return FrameGraphDescriptorBinding(BindingType::InputBuffer, res);
}

FrameGraphDescriptorBinding FrameGraphDescriptorBinding::create_uniform_binding(FrameGraphVolumeId res, SamplerType sampler) {
    return FrameGraphDescriptorBinding(BindingType::InputVolume, res, sampler);
}

FrameGraphDescriptorBinding FrameGraphDescriptorBinding::create_uniform_binding(FrameGraphImageId res, SamplerType sampler) {
    return FrameGraphDescriptorBinding(BindingType::InputImage, res, sampler);
}

Descriptor FrameGraphDescriptorBinding::create_descriptor(const FrameGraphFrameResources& resources) const {
    switch(_type) {
        case BindingType::External:
            return _external;

        case BindingType::InputImage:
            return Descriptor(resources.image<ImageUsage::TextureBit>(_image.image), _image.sampler);
        case BindingType::StorageImage:
            return resources.image<ImageUsage::StorageBit>(_image.image);

        case BindingType::InputVolume:
            return Descriptor(resources.volume<ImageUsage::TextureBit>(_volume.volume), _volume.sampler);
        case BindingType::StorageVolume:
            return resources.volume<ImageUsage::StorageBit>(_volume.volume);

        case BindingType::InputBuffer:
            return resources.buffer<BufferUsage::UniformBit>(_buffer);
        case BindingType::StorageBuffer:
            return resources.buffer<BufferUsage::StorageBit>(_buffer);

        default:
        break;
    }
    /*return*/ y_fatal("Invalid descriptor.");
}

}

