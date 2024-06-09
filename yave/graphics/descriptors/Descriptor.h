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
#ifndef YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTOR_H
#define YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTOR_H

#include <yave/graphics/graphics.h>
#include <yave/graphics/images/SamplerType.h>
#include <yave/graphics/images/ImageView.h>
#include <yave/graphics/buffers/Buffer.h>
#include <yave/graphics/raytracing/AccelerationStructure.h>

#include <y/core/Span.h>

namespace yave {

class InlineDescriptor {
    public:
        constexpr InlineDescriptor() = default;

        template<typename T>
        explicit constexpr InlineDescriptor(const T& data) : _data(&data), _size(sizeof(T)) {
            static_assert(sizeof(T) % 4 == 0, "InlineDescriptor's size must be a multiple of 4");
            static_assert(std::is_standard_layout_v<T>, "T is not standard layout");
            static_assert(!std::is_pointer_v<T>, "T shouldn't be a pointer, use a span instead");
            static_assert(!std::is_array_v<T>, "T shouldn't be an array, use a span instead");
        }

        template<typename T>
        explicit constexpr InlineDescriptor(core::Span<T> arr) : _data(arr.data()), _size(arr.size() * sizeof(T)) {
            static_assert(sizeof(T) % 4 == 0, "InlineDescriptor's size must be a multiple of 4");
            static_assert(std::is_standard_layout_v<T>, "T is not standard layout");
        }

        const void* data() const {
            return _data;
        }

        usize size() const {
            return _size;
        }

        bool is_empty() const {
            return !_size;
        }

        usize size_in_words() const {
            return _size / sizeof(u32);
        }

        const u32* words() const {
            return static_cast<const u32*>(_data);
        }

    private:
        const void* _data = nullptr;
        usize _size = 0;
};

class Descriptor {

    public:
        struct InlineBlock {
            const void* data;
            usize size;
        };

        union DescriptorInfo {
            VkDescriptorImageInfo image;
            VkDescriptorBufferInfo buffer;
            VkAccelerationStructureKHR accel;
            InlineBlock inline_block;

            DescriptorInfo(VkDescriptorImageInfo i) : image(i) {
            }

            DescriptorInfo(VkDescriptorBufferInfo b) : buffer(b) {
            }

            DescriptorInfo(VkAccelerationStructureKHR a) : accel(a) {
            }

            DescriptorInfo(const void* data, usize size) : inline_block({data, size}) {
            }
        };

        template<ImageType Type>
        Descriptor(const ImageView<ImageUsage::TextureBit, Type>& view, SamplerType sampler = SamplerType::LinearRepeat) :
                 _type(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER),
                 _info(view.vk_descriptor_info(sampler)) {
            y_debug_assert(!view.is_null());
        }

        template<ImageType Type>
        Descriptor(const ImageView<ImageUsage::StorageBit, Type>& view, SamplerType sampler = SamplerType::LinearRepeat) :
                 _type(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE),
                 _info(view.vk_descriptor_info(sampler)) {
            y_debug_assert(!view.is_null());
        }

        template<ImageUsage Usage, ImageType Type>
        Descriptor(const Image<Usage, Type>& image, SamplerType sampler = SamplerType::LinearRepeat) : Descriptor(ImageView<Usage, Type>(image), sampler) {
        }


        Descriptor(const SubBuffer<BufferUsage::UniformBit>& buffer) :
                _type(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
                _info(buffer.vk_descriptor_info()) {
            y_debug_assert(!buffer.is_null());
        }

        Descriptor(const SubBuffer<BufferUsage::StorageBit>& buffer) :
                _type(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
                _info(buffer.vk_descriptor_info()) {
            y_debug_assert(!buffer.is_null());
        }

        template<BufferUsage Usage, MemoryType Memory>
        Descriptor(const Buffer<Usage, Memory>& buffer) :
                Descriptor(SubBuffer<(Usage & BufferUsage::StorageBit) != BufferUsage::None ? BufferUsage::StorageBit : BufferUsage::UniformBit>(buffer)) {
        }

        Descriptor(const TLAS& tlas) :
                _type(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR),
                _info(tlas.vk_accel_struct()) {
        }

        Descriptor(InlineDescriptor inline_block) : Descriptor(inline_block.data(), inline_block.size()) {
        }

        Descriptor(const void* data, usize size) :
                _type(VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK),
                _info(data, size) {
        }



        VkDescriptorSetLayoutBinding descriptor_set_layout_binding(u32 index) const {
            const u32 size = is_inline_block() ? u32(_info.inline_block.size) : 1;
            VkDescriptorSetLayoutBinding binding = {};
            {
                binding.binding = index;
                binding.descriptorCount = size;
                binding.descriptorType = _type;
                binding.stageFlags = VK_SHADER_STAGE_ALL;
            }
            return binding;
        }

        const DescriptorInfo& descriptor_info() const {
            return _info;
        }

        VkDescriptorType vk_descriptor_type() const {
            return _type;
        }


        bool is_buffer() const {
            return is_buffer(_type);
        }

        bool is_image() const {
            return is_image(_type);
        }

        bool is_inline_block() const {
            return is_inline_block(_type);
        }

        bool is_acceleration_structure() const {
            return is_acceleration_structure(_type);
        }

        static bool is_buffer(VkDescriptorType type) {
            switch(type) {
                case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                    return true;
                default:
                    break;
            }
            return false;
        }

        static bool is_image(VkDescriptorType type) {
            switch(type) {
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    return true;
                default:
                    break;
            }
            return false;
        }

        static bool is_inline_block(VkDescriptorType type) {
            return type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;
        }

        static bool is_acceleration_structure(VkDescriptorType type) {
            return type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        }

    private:
        VkDescriptorType _type;
        DescriptorInfo _info;

};

}

#endif // YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTOR_H

