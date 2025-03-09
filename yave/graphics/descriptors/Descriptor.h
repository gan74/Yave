/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
        static constexpr usize max_byte_size = 256;

        constexpr InlineDescriptor() = default;

        template<typename T>
        explicit constexpr InlineDescriptor(const T& data) : _data(&data), _size(sizeof(T)) {
            static_assert(sizeof(T) % 4 == 0, "InlineDescriptor's size must be a multiple of 4");
            static_assert(sizeof(T) <= InlineDescriptor::max_byte_size, "InlineDescriptor's size should be less or equal to max_byte_size");
            static_assert(std::is_standard_layout_v<T>, "T is not standard layout");
            static_assert(!std::is_pointer_v<T>, "T shouldn't be a pointer, use a span instead");
            static_assert(!std::is_array_v<T>, "T shouldn't be an array, use a span instead");
            y_debug_assert(_size <= max_byte_size);
        }

        template<typename T>
        explicit constexpr InlineDescriptor(core::Span<T> arr) : _data(arr.data()), _size(arr.size() * sizeof(T)) {
            static_assert(sizeof(T) % 4 == 0, "InlineDescriptor's size must be a multiple of 4");
            static_assert(sizeof(T) <= InlineDescriptor::max_byte_size, "InlineDescriptor's size should be less or equal to max_byte_size");
            static_assert(std::is_standard_layout_v<T>, "T is not standard layout");
            y_debug_assert(_size <= max_byte_size);
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

        core::Span<u32> as_words() const {
            return {words(), size_in_words()};
        }

    private:
        const void* _data = nullptr;
        usize _size = 0;
};

class Descriptor {

    [[maybe_unused]]
    static bool is_sampler_compatible(ImageFormat format, SamplerType sampler) {
        switch(sampler) {
            case SamplerType::LinearRepeatAniso:
            case SamplerType::LinearClamp:
            case SamplerType::LinearRepeat:
            case SamplerType::Shadow:
                return format.supports_filtering();

            default:
                return true;
        }
    }

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
            y_debug_assert(is_sampler_compatible(view.format(), sampler));
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

        void fill_write(u32 index, VkWriteDescriptorSet& write, VkDescriptorBufferInfo& inline_buffer_info, VkWriteDescriptorSetAccelerationStructureKHR& accel_struct) const {
            write = vk_struct();
            write.dstBinding = index;
            write.descriptorCount = 1;
            write.descriptorType = vk_descriptor_type();

            if(is_buffer()) {
                write.pBufferInfo = &descriptor_info().buffer;
            } else if(is_image()) {
                write.pImageInfo = &descriptor_info().image;
            } else if(is_inline_block()) {
                UniformBuffer<MemoryType::CpuVisible> buffer(_info.inline_block.size);
                std::memcpy(buffer.map_bytes(MappingAccess::WriteOnly).data(), _info.inline_block.data, _info.inline_block.size);
                inline_buffer_info = buffer.vk_descriptor_info();
                write.descriptorCount = 1;
                write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                write.pBufferInfo = &inline_buffer_info;
            } else if(is_acceleration_structure()) {
                accel_struct = vk_struct();
                accel_struct.accelerationStructureCount = 1;
                accel_struct.pAccelerationStructures = &_info.accel;
                write.pNext = &accel_struct;
            } else {
                y_fatal("Unknown descriptor type");
            }
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

