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
#ifndef YAVE_GRAPHICS_VK_VK_H
#define YAVE_GRAPHICS_VK_VK_H

#include <y/defines.h>
#include <yave/yave.h>

// Sadly we need this in release to prevent unity builds including the file without the define
#define YAVE_VK_PLATFORM_INCLUDES

#ifdef YAVE_VK_PLATFORM_INCLUDES

#if defined(Y_OS_WIN)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(Y_OS_LINUX)
#define VK_USE_PLATFORM_XCB_KHR
#else
#error Unsupported platform
#endif

#endif

/**********************************
This can go into vulkan_core.h, so VkResult can be tagged with VK_NO_DISCARD
in order to get warning on missed returned results

#if __cplusplus >= 201703L
#define VK_NO_DISCARD [[nodiscard]]
#else
#define VK_NO_DISCARD
#endif

**********************************/

#define VK_NO_PROTOTYPES
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

#define VMA_ASSERT(expr) y_debug_assert(expr)

#ifdef YAVE_UNITY_BUILD
#define VOLK_IMPLEMENTATION
#endif


#include <Volk/volk.h>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>



namespace yave {

void initialize_volk();

namespace detail {

template<typename T>
struct VkStructType {};

#define VK_STRUCT_TYPE(Type, SType)                                             \
template<>                                                                      \
struct VkStructType<Type> { static constexpr VkStructureType type = SType; };


// Extensions
#ifdef Y_OS_WIN
VK_STRUCT_TYPE(VkWin32SurfaceCreateInfoKHR,                         VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR)
#endif

#ifdef Y_OS_LINUX
VK_STRUCT_TYPE(VkXcbSurfaceCreateInfoKHR,                           VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR)
#endif

VK_STRUCT_TYPE(VkPresentInfoKHR,                                    VK_STRUCTURE_TYPE_PRESENT_INFO_KHR)

VK_STRUCT_TYPE(VkSwapchainCreateInfoKHR,                            VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR)

VK_STRUCT_TYPE(VkDebugUtilsMessengerCreateInfoEXT,                  VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT)
VK_STRUCT_TYPE(VkDebugUtilsLabelEXT,                                VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT)
VK_STRUCT_TYPE(VkDebugUtilsObjectNameInfoEXT,                       VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT)
VK_STRUCT_TYPE(VkValidationFeaturesEXT,                             VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT)

VK_STRUCT_TYPE(VkCheckpointDataNV,                                  VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV)


// Raytracing
VK_STRUCT_TYPE(VkPhysicalDeviceAccelerationStructureFeaturesKHR,    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR)
VK_STRUCT_TYPE(VkPhysicalDeviceAccelerationStructurePropertiesKHR,  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR)
VK_STRUCT_TYPE(VkPhysicalDeviceRayTracingPipelineFeaturesKHR,       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR)
VK_STRUCT_TYPE(VkPhysicalDeviceRayQueryFeaturesKHR,                 VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR)
VK_STRUCT_TYPE(VkPhysicalDeviceRayTracingPipelinePropertiesKHR,     VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR)
VK_STRUCT_TYPE(VkAccelerationStructureBuildGeometryInfoKHR,         VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR)
VK_STRUCT_TYPE(VkAccelerationStructureCreateInfoKHR,                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR)
VK_STRUCT_TYPE(VkAccelerationStructureBuildSizesInfoKHR,            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR)
VK_STRUCT_TYPE(VkAccelerationStructureGeometryKHR,                  VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR)
VK_STRUCT_TYPE(VkAccelerationStructureGeometryTrianglesDataKHR,     VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR)
VK_STRUCT_TYPE(VkAccelerationStructureGeometryInstancesDataKHR,     VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR)
VK_STRUCT_TYPE(VkAccelerationStructureDeviceAddressInfoKHR,         VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR)
VK_STRUCT_TYPE(VkWriteDescriptorSetAccelerationStructureKHR,        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR)
VK_STRUCT_TYPE(VkRayTracingShaderGroupCreateInfoKHR,                VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR)
VK_STRUCT_TYPE(VkRayTracingPipelineCreateInfoKHR,                   VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR)


// Core
VK_STRUCT_TYPE(VkApplicationInfo,                                   VK_STRUCTURE_TYPE_APPLICATION_INFO)
VK_STRUCT_TYPE(VkInstanceCreateInfo,                                VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO)
VK_STRUCT_TYPE(VkDeviceQueueCreateInfo,                             VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO)
VK_STRUCT_TYPE(VkDeviceCreateInfo,                                  VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO)
VK_STRUCT_TYPE(VkSubmitInfo,                                        VK_STRUCTURE_TYPE_SUBMIT_INFO)
VK_STRUCT_TYPE(VkMemoryAllocateInfo,                                VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO)
VK_STRUCT_TYPE(VkMappedMemoryRange,                                 VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE)
VK_STRUCT_TYPE(VkBindSparseInfo,                                    VK_STRUCTURE_TYPE_BIND_SPARSE_INFO)
VK_STRUCT_TYPE(VkFenceCreateInfo,                                   VK_STRUCTURE_TYPE_FENCE_CREATE_INFO)
VK_STRUCT_TYPE(VkSemaphoreCreateInfo,                               VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO)
VK_STRUCT_TYPE(VkEventCreateInfo,                                   VK_STRUCTURE_TYPE_EVENT_CREATE_INFO)
VK_STRUCT_TYPE(VkQueryPoolCreateInfo,                               VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO)
VK_STRUCT_TYPE(VkBufferCreateInfo,                                  VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO)
VK_STRUCT_TYPE(VkBufferViewCreateInfo,                              VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO)
VK_STRUCT_TYPE(VkImageCreateInfo,                                   VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
VK_STRUCT_TYPE(VkImageViewCreateInfo,                               VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO)
VK_STRUCT_TYPE(VkShaderModuleCreateInfo,                            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO)
VK_STRUCT_TYPE(VkPipelineCacheCreateInfo,                           VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO)
VK_STRUCT_TYPE(VkPipelineShaderStageCreateInfo,                     VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO)
VK_STRUCT_TYPE(VkPipelineVertexInputStateCreateInfo,                VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO)
VK_STRUCT_TYPE(VkPipelineInputAssemblyStateCreateInfo,              VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO)
VK_STRUCT_TYPE(VkPipelineTessellationStateCreateInfo,               VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO)
VK_STRUCT_TYPE(VkPipelineViewportStateCreateInfo,                   VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO)
VK_STRUCT_TYPE(VkPipelineRasterizationStateCreateInfo,              VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO)
VK_STRUCT_TYPE(VkPipelineMultisampleStateCreateInfo,                VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO)
VK_STRUCT_TYPE(VkPipelineDepthStencilStateCreateInfo,               VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO)
VK_STRUCT_TYPE(VkPipelineColorBlendStateCreateInfo,                 VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO)
VK_STRUCT_TYPE(VkPipelineDynamicStateCreateInfo,                    VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO)
VK_STRUCT_TYPE(VkGraphicsPipelineCreateInfo,                        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO)
VK_STRUCT_TYPE(VkComputePipelineCreateInfo,                         VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO)
VK_STRUCT_TYPE(VkPipelineLayoutCreateInfo,                          VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO)
VK_STRUCT_TYPE(VkSamplerCreateInfo,                                 VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO)
VK_STRUCT_TYPE(VkDescriptorSetLayoutCreateInfo,                     VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO)
VK_STRUCT_TYPE(VkDescriptorPoolCreateInfo,                          VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO)
VK_STRUCT_TYPE(VkDescriptorPoolInlineUniformBlockCreateInfo,        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_INLINE_UNIFORM_BLOCK_CREATE_INFO)
VK_STRUCT_TYPE(VkDescriptorSetAllocateInfo,                         VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO)
VK_STRUCT_TYPE(VkWriteDescriptorSet,                                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET)
VK_STRUCT_TYPE(VkCopyDescriptorSet,                                 VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET)
VK_STRUCT_TYPE(VkFramebufferCreateInfo,                             VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO)
VK_STRUCT_TYPE(VkRenderPassCreateInfo,                              VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO)
VK_STRUCT_TYPE(VkCommandPoolCreateInfo,                             VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO)
VK_STRUCT_TYPE(VkCommandBufferAllocateInfo,                         VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO)
VK_STRUCT_TYPE(VkCommandBufferInheritanceInfo,                      VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO)
VK_STRUCT_TYPE(VkCommandBufferBeginInfo,                            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO)
VK_STRUCT_TYPE(VkRenderPassBeginInfo,                               VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO)
VK_STRUCT_TYPE(VkBufferMemoryBarrier,                               VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER)
VK_STRUCT_TYPE(VkImageMemoryBarrier,                                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER)
VK_STRUCT_TYPE(VkMemoryBarrier,                                     VK_STRUCTURE_TYPE_MEMORY_BARRIER)
VK_STRUCT_TYPE(VkPhysicalDeviceSubgroupProperties,                  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES)
VK_STRUCT_TYPE(VkBindBufferMemoryInfo,                              VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO)
VK_STRUCT_TYPE(VkBindImageMemoryInfo,                               VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO)
VK_STRUCT_TYPE(VkPhysicalDevice16BitStorageFeatures,                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES)
VK_STRUCT_TYPE(VkMemoryDedicatedRequirements,                       VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS)
VK_STRUCT_TYPE(VkMemoryDedicatedAllocateInfo,                       VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO)
VK_STRUCT_TYPE(VkMemoryAllocateFlagsInfo,                           VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO)
VK_STRUCT_TYPE(VkDeviceGroupRenderPassBeginInfo,                    VK_STRUCTURE_TYPE_DEVICE_GROUP_RENDER_PASS_BEGIN_INFO)
VK_STRUCT_TYPE(VkDeviceGroupCommandBufferBeginInfo,                 VK_STRUCTURE_TYPE_DEVICE_GROUP_COMMAND_BUFFER_BEGIN_INFO)
VK_STRUCT_TYPE(VkDeviceGroupSubmitInfo,                             VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO)
VK_STRUCT_TYPE(VkDeviceGroupBindSparseInfo,                         VK_STRUCTURE_TYPE_DEVICE_GROUP_BIND_SPARSE_INFO)
VK_STRUCT_TYPE(VkBindBufferMemoryDeviceGroupInfo,                   VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_DEVICE_GROUP_INFO)
VK_STRUCT_TYPE(VkBindImageMemoryDeviceGroupInfo,                    VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_DEVICE_GROUP_INFO)
VK_STRUCT_TYPE(VkPhysicalDeviceGroupProperties,                     VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES)
VK_STRUCT_TYPE(VkDeviceGroupDeviceCreateInfo,                       VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO)
VK_STRUCT_TYPE(VkBufferMemoryRequirementsInfo2,                     VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2)
VK_STRUCT_TYPE(VkImageMemoryRequirementsInfo2,                      VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2)
VK_STRUCT_TYPE(VkImageSparseMemoryRequirementsInfo2,                VK_STRUCTURE_TYPE_IMAGE_SPARSE_MEMORY_REQUIREMENTS_INFO_2)
VK_STRUCT_TYPE(VkMemoryRequirements2,                               VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2)
VK_STRUCT_TYPE(VkSparseImageMemoryRequirements2,                    VK_STRUCTURE_TYPE_SPARSE_IMAGE_MEMORY_REQUIREMENTS_2)
VK_STRUCT_TYPE(VkPhysicalDeviceFeatures2,                           VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2)
VK_STRUCT_TYPE(VkPhysicalDeviceProperties2,                         VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2)
VK_STRUCT_TYPE(VkFormatProperties2,                                 VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2)
VK_STRUCT_TYPE(VkImageFormatProperties2,                            VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2)
VK_STRUCT_TYPE(VkPhysicalDeviceImageFormatInfo2,                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2)
VK_STRUCT_TYPE(VkQueueFamilyProperties2,                            VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2)
VK_STRUCT_TYPE(VkPhysicalDeviceMemoryProperties2,                   VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2)
VK_STRUCT_TYPE(VkSparseImageFormatProperties2,                      VK_STRUCTURE_TYPE_SPARSE_IMAGE_FORMAT_PROPERTIES_2)
VK_STRUCT_TYPE(VkPhysicalDeviceSparseImageFormatInfo2,              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SPARSE_IMAGE_FORMAT_INFO_2)
VK_STRUCT_TYPE(VkPhysicalDevicePointClippingProperties,             VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_POINT_CLIPPING_PROPERTIES)
VK_STRUCT_TYPE(VkRenderPassInputAttachmentAspectCreateInfo,         VK_STRUCTURE_TYPE_RENDER_PASS_INPUT_ATTACHMENT_ASPECT_CREATE_INFO)
VK_STRUCT_TYPE(VkImageViewUsageCreateInfo,                          VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO)
VK_STRUCT_TYPE(VkPipelineTessellationDomainOriginStateCreateInfo,   VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO)
VK_STRUCT_TYPE(VkRenderPassMultiviewCreateInfo,                     VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO)
VK_STRUCT_TYPE(VkPhysicalDeviceMultiviewFeatures,                   VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES)
VK_STRUCT_TYPE(VkPhysicalDeviceMultiviewProperties,                 VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES)
VK_STRUCT_TYPE(VkPhysicalDeviceVariablePointersFeatures,            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES)
VK_STRUCT_TYPE(VkProtectedSubmitInfo,                               VK_STRUCTURE_TYPE_PROTECTED_SUBMIT_INFO)
VK_STRUCT_TYPE(VkPhysicalDeviceProtectedMemoryFeatures,             VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES)
VK_STRUCT_TYPE(VkPhysicalDeviceProtectedMemoryProperties,           VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_PROPERTIES)
VK_STRUCT_TYPE(VkDeviceQueueInfo2,                                  VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2)
VK_STRUCT_TYPE(VkSamplerYcbcrConversionCreateInfo,                  VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO)
VK_STRUCT_TYPE(VkSamplerYcbcrConversionInfo,                        VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO)
VK_STRUCT_TYPE(VkBindImagePlaneMemoryInfo,                          VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO)
VK_STRUCT_TYPE(VkImagePlaneMemoryRequirementsInfo,                  VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO)
VK_STRUCT_TYPE(VkPhysicalDeviceSamplerYcbcrConversionFeatures,      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES)
VK_STRUCT_TYPE(VkSamplerYcbcrConversionImageFormatProperties,       VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES)
VK_STRUCT_TYPE(VkDescriptorUpdateTemplateCreateInfo,                VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO)
VK_STRUCT_TYPE(VkPhysicalDeviceExternalImageFormatInfo,             VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO)
VK_STRUCT_TYPE(VkExternalImageFormatProperties,                     VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES)
VK_STRUCT_TYPE(VkPhysicalDeviceExternalBufferInfo,                  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO)
VK_STRUCT_TYPE(VkExternalBufferProperties,                          VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES)
VK_STRUCT_TYPE(VkPhysicalDeviceIDProperties,                        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES)
VK_STRUCT_TYPE(VkExternalMemoryBufferCreateInfo,                    VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO)
VK_STRUCT_TYPE(VkExternalMemoryImageCreateInfo,                     VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO)
VK_STRUCT_TYPE(VkExportMemoryAllocateInfo,                          VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO)
VK_STRUCT_TYPE(VkPhysicalDeviceExternalFenceInfo,                   VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_FENCE_INFO)
VK_STRUCT_TYPE(VkExternalFenceProperties,                           VK_STRUCTURE_TYPE_EXTERNAL_FENCE_PROPERTIES)
VK_STRUCT_TYPE(VkExportFenceCreateInfo,                             VK_STRUCTURE_TYPE_EXPORT_FENCE_CREATE_INFO)
VK_STRUCT_TYPE(VkExportSemaphoreCreateInfo,                         VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO)
VK_STRUCT_TYPE(VkPhysicalDeviceExternalSemaphoreInfo,               VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO)
VK_STRUCT_TYPE(VkExternalSemaphoreProperties,                       VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES)
VK_STRUCT_TYPE(VkPhysicalDeviceMaintenance3Properties,              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES)
VK_STRUCT_TYPE(VkDescriptorSetLayoutSupport,                        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_SUPPORT)
VK_STRUCT_TYPE(VkPhysicalDeviceShaderDrawParametersFeatures,        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES)
VK_STRUCT_TYPE(VkPhysicalDeviceVulkan11Features,                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES)
VK_STRUCT_TYPE(VkPhysicalDeviceVulkan11Properties,                  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES)
VK_STRUCT_TYPE(VkPhysicalDeviceVulkan12Features,                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES)
VK_STRUCT_TYPE(VkPhysicalDeviceVulkan12Properties,                  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES)
VK_STRUCT_TYPE(VkPhysicalDeviceVulkan13Features,                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES)
VK_STRUCT_TYPE(VkPhysicalDeviceVulkan13Properties,                  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES)
VK_STRUCT_TYPE(VkPhysicalDeviceVulkan14Features,                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES)
VK_STRUCT_TYPE(VkPhysicalDeviceVulkan14Properties,                  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES)
VK_STRUCT_TYPE(VkImageFormatListCreateInfo,                         VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO)
VK_STRUCT_TYPE(VkAttachmentDescription2,                            VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2)
VK_STRUCT_TYPE(VkAttachmentReference2,                              VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2)
VK_STRUCT_TYPE(VkSubpassDescription2,                               VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2)
VK_STRUCT_TYPE(VkSubpassDependency2,                                VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2)
VK_STRUCT_TYPE(VkRenderPassCreateInfo2,                             VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2)
VK_STRUCT_TYPE(VkSubpassBeginInfo,                                  VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO)
VK_STRUCT_TYPE(VkSubpassEndInfo,                                    VK_STRUCTURE_TYPE_SUBPASS_END_INFO)
VK_STRUCT_TYPE(VkPhysicalDevice8BitStorageFeatures,                 VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES)
VK_STRUCT_TYPE(VkPhysicalDeviceDriverProperties,                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES)
VK_STRUCT_TYPE(VkPhysicalDeviceShaderAtomicInt64Features,           VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES)
VK_STRUCT_TYPE(VkPhysicalDeviceShaderFloat16Int8Features,           VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES)
VK_STRUCT_TYPE(VkPhysicalDeviceFloatControlsProperties,             VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES)
VK_STRUCT_TYPE(VkDescriptorSetLayoutBindingFlagsCreateInfo,         VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO)
VK_STRUCT_TYPE(VkPhysicalDeviceDescriptorIndexingFeatures,          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES)
VK_STRUCT_TYPE(VkPhysicalDeviceDescriptorIndexingProperties,        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES)
VK_STRUCT_TYPE(VkDescriptorSetVariableDescriptorCountAllocateInfo,  VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO)
VK_STRUCT_TYPE(VkDescriptorSetVariableDescriptorCountLayoutSupport, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_LAYOUT_SUPPORT)
VK_STRUCT_TYPE(VkPhysicalDeviceDepthStencilResolveProperties,       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES)
VK_STRUCT_TYPE(VkSubpassDescriptionDepthStencilResolve,             VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE)
VK_STRUCT_TYPE(VkPhysicalDeviceScalarBlockLayoutFeatures,           VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES)
VK_STRUCT_TYPE(VkImageStencilUsageCreateInfo,                       VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO)
VK_STRUCT_TYPE(VkPhysicalDeviceSamplerFilterMinmaxProperties,       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_FILTER_MINMAX_PROPERTIES)
VK_STRUCT_TYPE(VkSamplerReductionModeCreateInfo,                    VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO)
VK_STRUCT_TYPE(VkPhysicalDeviceVulkanMemoryModelFeatures,           VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES)
VK_STRUCT_TYPE(VkPhysicalDeviceImagelessFramebufferFeatures,        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES)
VK_STRUCT_TYPE(VkFramebufferAttachmentsCreateInfo,                  VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO)
VK_STRUCT_TYPE(VkFramebufferAttachmentImageInfo,                    VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO)
VK_STRUCT_TYPE(VkRenderPassAttachmentBeginInfo,                     VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO)
VK_STRUCT_TYPE(VkPhysicalDeviceUniformBufferStandardLayoutFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES)
VK_STRUCT_TYPE(VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES)
VK_STRUCT_TYPE(VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES)
VK_STRUCT_TYPE(VkAttachmentReferenceStencilLayout,                  VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_STENCIL_LAYOUT)
VK_STRUCT_TYPE(VkAttachmentDescriptionStencilLayout,                VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_STENCIL_LAYOUT)
VK_STRUCT_TYPE(VkPhysicalDeviceHostQueryResetFeatures,              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES)
VK_STRUCT_TYPE(VkPhysicalDeviceTimelineSemaphoreFeatures,           VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES)
VK_STRUCT_TYPE(VkPhysicalDeviceTimelineSemaphoreProperties,         VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES)
VK_STRUCT_TYPE(VkSemaphoreTypeCreateInfo,                           VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO)
VK_STRUCT_TYPE(VkTimelineSemaphoreSubmitInfo,                       VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO)
VK_STRUCT_TYPE(VkSemaphoreWaitInfo,                                 VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO)
VK_STRUCT_TYPE(VkSemaphoreSignalInfo,                               VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO)
VK_STRUCT_TYPE(VkPhysicalDeviceBufferDeviceAddressFeatures,         VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES)
VK_STRUCT_TYPE(VkBufferDeviceAddressInfo,                           VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO)
VK_STRUCT_TYPE(VkBufferOpaqueCaptureAddressCreateInfo,              VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO)
VK_STRUCT_TYPE(VkMemoryOpaqueCaptureAddressAllocateInfo,            VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO)
VK_STRUCT_TYPE(VkDeviceMemoryOpaqueCaptureAddressInfo,              VK_STRUCTURE_TYPE_DEVICE_MEMORY_OPAQUE_CAPTURE_ADDRESS_INFO)

#undef VK_STRUCT_TYPE


struct VkStructInitializer {
    template<typename T>
    inline constexpr operator T() const {
        T t = {};
        t.sType = VkStructType<T>::type;
        return t;
    }
};
}


inline constexpr auto vk_struct() {
    return detail::VkStructInitializer();
}


template<typename T, typename R>
inline constexpr T* vk_find_pnext(const R& r) {
    void* next = r.pNext;
    while(next) {
        T* t = static_cast<T*>(next);
        if(t->sType == detail::VkStructType<T>::type) {
            return t;
        }
        next = t->pNext;
    }
    return nullptr;
}

inline constexpr bool is_error(VkResult result) {
    return result < VK_SUCCESS;
}
inline constexpr bool is_incomplete(VkResult result) {
    return result == VK_INCOMPLETE;
}

const char* vk_result_str(VkResult result);
void on_vk_device_lost();

inline VkResult vk_check(VkResult result) {
    if(is_error(result)) [[unlikely]] {
        if(result == VK_ERROR_DEVICE_LOST) {
            on_vk_device_lost();
        }
        y_fatal(vk_result_str(result));
    }

    return result;
}

[[nodiscard]] inline bool vk_swapchain_out_of_date(VkResult result) {
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return true;
    }
    vk_check(result);
    return false;
}

inline VkResult vk_check_or_incomplete(VkResult result) {
    y_always_assert(is_incomplete(result) || !is_error(result), vk_result_str(result));
    return result;
}


template<typename T>
class VkHandle {
    public:
        static_assert(std::is_trivially_copyable_v<T>);

        VkHandle() = default;

        VkHandle(VkHandle&& other) {
            swap(other);
        }

        ~VkHandle() {
            y_debug_assert(!_t || _consumed);
        }

        VkHandle& operator=(VkHandle&& other) {
            swap(other);
            return *this;
        }

        void swap(VkHandle& other) {
            std::swap(_t, other._t);
#ifdef Y_DEBUG
            std::swap(_consumed, other._consumed);
#endif
        }

        operator T() const {
            y_debug_assert(!_consumed);
            return _t;
        }

        T* get_ptr_for_init() {
            y_debug_assert(!_t || _consumed);
#ifdef Y_DEBUG
            _t = {};
            _consumed = false;
#endif
            return &_t;
        }

        const T& get() const {
            y_debug_assert(!_consumed);
            return _t;
        }

        T consume() {
#ifdef Y_DEBUG
            y_debug_assert(!std::exchange(_consumed, true));
            return _t;
#else
            return std::exchange(_t, T{});
#endif
        }

        bool is_null() const {
            return !_t;
        }

        bool operator==(const VkHandle& other) const {
            return _t == other._t;
        }

        bool operator!=(const VkHandle& other) const {
            return !operator==(other);
        }

    private:
        T _t = {};

#ifdef Y_DEBUG
        bool _consumed = false;
#endif
};

}




#define VK_STRUCT_NEQ(Type)                                                             \
inline constexpr bool operator!=(const Type& lhs, const Type& rhs) {                    \
    return !operator==(lhs, rhs);                                                       \
}

#define VK_STRUCT_OP_1(Type)                                                            \
inline constexpr bool operator==(const Type& lhs, const Type& rhs) {                    \
    const auto& [l0] = lhs;                                                             \
    const auto& [r0] = rhs;                                                             \
    return l0 == r0;                                                                    \
}                                                                                       \
VK_STRUCT_NEQ(Type)

#define VK_STRUCT_OP_4(Type)                                                            \
inline constexpr bool operator==(const Type& lhs, const Type& rhs) {                    \
    const auto& [l0, l1, l2, l3, l4] = lhs;                                             \
    const auto& [r0, r1, r2, r3, r4] = rhs;                                             \
    return (l0 == r0) && (l1 == r1) && (l2 == r2) && (l3 == r3);                        \
}                                                                                       \
VK_STRUCT_NEQ(Type)

#define VK_STRUCT_OP_5(Type)                                                            \
inline constexpr bool operator==(const Type& lhs, const Type& rhs) {                    \
    const auto& [l0, l1, l2, l3, l4] = lhs;                                             \
    const auto& [r0, r1, r2, r3, r4] = rhs;                                             \
    return (l0 == r0) && (l1 == r1) && (l2 == r2) && (l3 == r3) && (l4 == r4);          \
}                                                                                       \
VK_STRUCT_NEQ(Type)



VK_STRUCT_OP_5(VkDescriptorSetLayoutBinding)


#undef VK_STRUCT_OP_1
#undef VK_STRUCT_OP_4
#undef VK_STRUCT_OP_5
#undef VK_STRUCT_NEQ


#define YAVE_VK

#endif // YAVE_GRAPHICS_VK_VK_H
