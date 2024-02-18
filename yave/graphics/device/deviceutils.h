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
#ifndef YAVE_DEVICE_DEVICEUTILS_H
#define YAVE_DEVICE_DEVICEUTILS_H

#include <yave/graphics/graphics.h>

#include <y/core/Span.h>
#include <y/core/Vector.h>

#include <atomic>

namespace yave {

float device_score(const PhysicalDevice& device);

bool try_enable_extension(core::Vector<const char*>& exts, const char* name, const PhysicalDevice& device);

VkSamplerAddressMode vk_address_mode(SamplerType type);
VkFilter vk_filter(SamplerType type);
VkSamplerMipmapMode vk_mip_filter(SamplerType type);

VkHandle<VkSampler> create_sampler(SamplerType type);

core::Vector<VkQueueFamilyProperties> enumerate_family_properties(VkPhysicalDevice device);
u32 queue_family_index(core::Span<VkQueueFamilyProperties> families, VkQueueFlags flags);
VkQueue create_queue(VkDevice device, u32 family_index, u32 index);

void print_physical_properties(const VkPhysicalDeviceProperties& properties);
void print_enabled_extensions(core::Span<const char*> extensions);
void print_properties(const DeviceProperties& properties);

PhysicalDevice find_best_device(const Instance& instance);

VkPhysicalDeviceFeatures required_device_features();
VkPhysicalDeviceVulkan11Features required_device_features_1_1();
VkPhysicalDeviceVulkan12Features required_device_features_1_2();
VkPhysicalDeviceVulkan13Features required_device_features_1_3();

bool has_required_features(const PhysicalDevice& physical);
bool has_required_properties(const PhysicalDevice &physical);

}


#endif // YAVE_DEVICE_DEVICEUTILS_H

