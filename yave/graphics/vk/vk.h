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
#ifndef YAVE_GRAPHICS_VK_VK_H
#define YAVE_GRAPHICS_VK_VK_H

#include <y/defines.h>
#include <yave/yave.h>

// Sadly we need this in release to prevent unity builds including the file without the define
#define YAVE_VK_PLATFORM_INCLUDES

#ifdef YAVE_VK_PLATFORM_INCLUDES
#ifdef Y_OS_WIN
#define VK_USE_PLATFORM_WIN32_KHR
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

#include <vulkan/vulkan.h>

namespace yave {
namespace detail {

#define VK_STRUCT_INIT(Type, SType)                         \
inline constexpr void vk_init_struct_stype(Type& t) {       \
    t.sType = SType;                                        \
}

// Extensions
VK_STRUCT_INIT(VkPresentInfoKHR,                                    VK_STRUCTURE_TYPE_PRESENT_INFO_KHR)

VK_STRUCT_INIT(VkSwapchainCreateInfoKHR,                            VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR)
VK_STRUCT_INIT(VkWin32SurfaceCreateInfoKHR,                         VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR)

VK_STRUCT_INIT(VkWriteDescriptorSetInlineUniformBlockEXT,           VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK_EXT)
VK_STRUCT_INIT(VkPhysicalDeviceInlineUniformBlockPropertiesEXT,     VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_PROPERTIES_EXT)

VK_STRUCT_INIT(VkDebugUtilsMessengerCreateInfoEXT,                  VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT)
VK_STRUCT_INIT(VkDebugUtilsLabelEXT,                                VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT)
VK_STRUCT_INIT(VkDebugUtilsObjectNameInfoEXT,                       VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT)
VK_STRUCT_INIT(VkValidationFeaturesEXT,                             VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT)

VK_STRUCT_INIT(VkRayTracingShaderGroupCreateInfoNV,                 VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV)
VK_STRUCT_INIT(VkRayTracingPipelineCreateInfoNV,                    VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV)
VK_STRUCT_INIT(VkGeometryTrianglesNV,                               VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV)
VK_STRUCT_INIT(VkGeometryAABBNV,                                    VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV)
VK_STRUCT_INIT(VkGeometryNV,                                        VK_STRUCTURE_TYPE_GEOMETRY_NV)
VK_STRUCT_INIT(VkAccelerationStructureInfoNV,                       VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV)
VK_STRUCT_INIT(VkAccelerationStructureCreateInfoNV,                 VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV)
VK_STRUCT_INIT(VkBindAccelerationStructureMemoryInfoNV,             VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV)
VK_STRUCT_INIT(VkWriteDescriptorSetAccelerationStructureNV,         VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV)
VK_STRUCT_INIT(VkAccelerationStructureMemoryRequirementsInfoNV,     VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV)
VK_STRUCT_INIT(VkPhysicalDeviceRayTracingPropertiesNV,              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV)


// Core
VK_STRUCT_INIT(VkApplicationInfo,                                   VK_STRUCTURE_TYPE_APPLICATION_INFO)
VK_STRUCT_INIT(VkInstanceCreateInfo,                                VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO)
VK_STRUCT_INIT(VkDeviceQueueCreateInfo,                             VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO)
VK_STRUCT_INIT(VkDeviceCreateInfo,                                  VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO)
VK_STRUCT_INIT(VkSubmitInfo,                                        VK_STRUCTURE_TYPE_SUBMIT_INFO)
VK_STRUCT_INIT(VkMemoryAllocateInfo,                                VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO)
VK_STRUCT_INIT(VkMappedMemoryRange,                                 VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE)
VK_STRUCT_INIT(VkBindSparseInfo,                                    VK_STRUCTURE_TYPE_BIND_SPARSE_INFO)
VK_STRUCT_INIT(VkFenceCreateInfo,                                   VK_STRUCTURE_TYPE_FENCE_CREATE_INFO)
VK_STRUCT_INIT(VkSemaphoreCreateInfo,                               VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO)
VK_STRUCT_INIT(VkEventCreateInfo,                                   VK_STRUCTURE_TYPE_EVENT_CREATE_INFO)
VK_STRUCT_INIT(VkQueryPoolCreateInfo,                               VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO)
VK_STRUCT_INIT(VkBufferCreateInfo,                                  VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO)
VK_STRUCT_INIT(VkBufferViewCreateInfo,                              VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO)
VK_STRUCT_INIT(VkImageCreateInfo,                                   VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
VK_STRUCT_INIT(VkImageViewCreateInfo,                               VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO)
VK_STRUCT_INIT(VkShaderModuleCreateInfo,                            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO)
VK_STRUCT_INIT(VkPipelineCacheCreateInfo,                           VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO)
VK_STRUCT_INIT(VkPipelineShaderStageCreateInfo,                     VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO)
VK_STRUCT_INIT(VkPipelineVertexInputStateCreateInfo,                VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO)
VK_STRUCT_INIT(VkPipelineInputAssemblyStateCreateInfo,              VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO)
VK_STRUCT_INIT(VkPipelineTessellationStateCreateInfo,               VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO)
VK_STRUCT_INIT(VkPipelineViewportStateCreateInfo,                   VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO)
VK_STRUCT_INIT(VkPipelineRasterizationStateCreateInfo,              VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO)
VK_STRUCT_INIT(VkPipelineMultisampleStateCreateInfo,                VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO)
VK_STRUCT_INIT(VkPipelineDepthStencilStateCreateInfo,               VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO)
VK_STRUCT_INIT(VkPipelineColorBlendStateCreateInfo,                 VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO)
VK_STRUCT_INIT(VkPipelineDynamicStateCreateInfo,                    VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO)
VK_STRUCT_INIT(VkGraphicsPipelineCreateInfo,                        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO)
VK_STRUCT_INIT(VkComputePipelineCreateInfo,                         VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO)
VK_STRUCT_INIT(VkPipelineLayoutCreateInfo,                          VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO)
VK_STRUCT_INIT(VkSamplerCreateInfo,                                 VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO)
VK_STRUCT_INIT(VkDescriptorSetLayoutCreateInfo,                     VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO)
VK_STRUCT_INIT(VkDescriptorPoolCreateInfo,                          VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO)
VK_STRUCT_INIT(VkDescriptorSetAllocateInfo,                         VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO)
VK_STRUCT_INIT(VkWriteDescriptorSet,                                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET)
VK_STRUCT_INIT(VkCopyDescriptorSet,                                 VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET)
VK_STRUCT_INIT(VkFramebufferCreateInfo,                             VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO)
VK_STRUCT_INIT(VkRenderPassCreateInfo,                              VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO)
VK_STRUCT_INIT(VkCommandPoolCreateInfo,                             VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO)
VK_STRUCT_INIT(VkCommandBufferAllocateInfo,                         VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO)
VK_STRUCT_INIT(VkCommandBufferInheritanceInfo,                      VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO)
VK_STRUCT_INIT(VkCommandBufferBeginInfo,                            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO)
VK_STRUCT_INIT(VkRenderPassBeginInfo,                               VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO)
VK_STRUCT_INIT(VkBufferMemoryBarrier,                               VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER)
VK_STRUCT_INIT(VkImageMemoryBarrier,                                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER)
VK_STRUCT_INIT(VkMemoryBarrier,                                     VK_STRUCTURE_TYPE_MEMORY_BARRIER)
VK_STRUCT_INIT(VkPhysicalDeviceSubgroupProperties,                  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES)
VK_STRUCT_INIT(VkBindBufferMemoryInfo,                              VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO)
VK_STRUCT_INIT(VkBindImageMemoryInfo,                               VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO)
VK_STRUCT_INIT(VkPhysicalDevice16BitStorageFeatures,                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES)
VK_STRUCT_INIT(VkMemoryDedicatedRequirements,                       VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS)
VK_STRUCT_INIT(VkMemoryDedicatedAllocateInfo,                       VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO)
VK_STRUCT_INIT(VkMemoryAllocateFlagsInfo,                           VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO)
VK_STRUCT_INIT(VkDeviceGroupRenderPassBeginInfo,                    VK_STRUCTURE_TYPE_DEVICE_GROUP_RENDER_PASS_BEGIN_INFO)
VK_STRUCT_INIT(VkDeviceGroupCommandBufferBeginInfo,                 VK_STRUCTURE_TYPE_DEVICE_GROUP_COMMAND_BUFFER_BEGIN_INFO)
VK_STRUCT_INIT(VkDeviceGroupSubmitInfo,                             VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO)
VK_STRUCT_INIT(VkDeviceGroupBindSparseInfo,                         VK_STRUCTURE_TYPE_DEVICE_GROUP_BIND_SPARSE_INFO)
VK_STRUCT_INIT(VkBindBufferMemoryDeviceGroupInfo,                   VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_DEVICE_GROUP_INFO)
VK_STRUCT_INIT(VkBindImageMemoryDeviceGroupInfo,                    VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_DEVICE_GROUP_INFO)
VK_STRUCT_INIT(VkPhysicalDeviceGroupProperties,                     VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES)
VK_STRUCT_INIT(VkDeviceGroupDeviceCreateInfo,                       VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO)
VK_STRUCT_INIT(VkBufferMemoryRequirementsInfo2,                     VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2)
VK_STRUCT_INIT(VkImageMemoryRequirementsInfo2,                      VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2)
VK_STRUCT_INIT(VkImageSparseMemoryRequirementsInfo2,                VK_STRUCTURE_TYPE_IMAGE_SPARSE_MEMORY_REQUIREMENTS_INFO_2)
VK_STRUCT_INIT(VkMemoryRequirements2,                               VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2)
VK_STRUCT_INIT(VkSparseImageMemoryRequirements2,                    VK_STRUCTURE_TYPE_SPARSE_IMAGE_MEMORY_REQUIREMENTS_2)
VK_STRUCT_INIT(VkPhysicalDeviceFeatures2,                           VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2)
VK_STRUCT_INIT(VkPhysicalDeviceProperties2,                         VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2)
VK_STRUCT_INIT(VkFormatProperties2,                                 VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2)
VK_STRUCT_INIT(VkImageFormatProperties2,                            VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2)
VK_STRUCT_INIT(VkPhysicalDeviceImageFormatInfo2,                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2)
VK_STRUCT_INIT(VkQueueFamilyProperties2,                            VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2)
VK_STRUCT_INIT(VkPhysicalDeviceMemoryProperties2,                   VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2)
VK_STRUCT_INIT(VkSparseImageFormatProperties2,                      VK_STRUCTURE_TYPE_SPARSE_IMAGE_FORMAT_PROPERTIES_2)
VK_STRUCT_INIT(VkPhysicalDeviceSparseImageFormatInfo2,              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SPARSE_IMAGE_FORMAT_INFO_2)
VK_STRUCT_INIT(VkPhysicalDevicePointClippingProperties,             VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_POINT_CLIPPING_PROPERTIES)
VK_STRUCT_INIT(VkRenderPassInputAttachmentAspectCreateInfo,         VK_STRUCTURE_TYPE_RENDER_PASS_INPUT_ATTACHMENT_ASPECT_CREATE_INFO)
VK_STRUCT_INIT(VkImageViewUsageCreateInfo,                          VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO)
VK_STRUCT_INIT(VkPipelineTessellationDomainOriginStateCreateInfo,   VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO)
VK_STRUCT_INIT(VkRenderPassMultiviewCreateInfo,                     VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO)
VK_STRUCT_INIT(VkPhysicalDeviceMultiviewFeatures,                   VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES)
VK_STRUCT_INIT(VkPhysicalDeviceMultiviewProperties,                 VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES)
VK_STRUCT_INIT(VkPhysicalDeviceVariablePointersFeatures,            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES)
VK_STRUCT_INIT(VkProtectedSubmitInfo,                               VK_STRUCTURE_TYPE_PROTECTED_SUBMIT_INFO)
VK_STRUCT_INIT(VkPhysicalDeviceProtectedMemoryFeatures,             VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES)
VK_STRUCT_INIT(VkPhysicalDeviceProtectedMemoryProperties,           VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_PROPERTIES)
VK_STRUCT_INIT(VkDeviceQueueInfo2,                                  VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2)
VK_STRUCT_INIT(VkSamplerYcbcrConversionCreateInfo,                  VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO)
VK_STRUCT_INIT(VkSamplerYcbcrConversionInfo,                        VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO)
VK_STRUCT_INIT(VkBindImagePlaneMemoryInfo,                          VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO)
VK_STRUCT_INIT(VkImagePlaneMemoryRequirementsInfo,                  VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO)
VK_STRUCT_INIT(VkPhysicalDeviceSamplerYcbcrConversionFeatures,      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES)
VK_STRUCT_INIT(VkSamplerYcbcrConversionImageFormatProperties,       VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES)
VK_STRUCT_INIT(VkDescriptorUpdateTemplateCreateInfo,                VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO)
VK_STRUCT_INIT(VkPhysicalDeviceExternalImageFormatInfo,             VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO)
VK_STRUCT_INIT(VkExternalImageFormatProperties,                     VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES)
VK_STRUCT_INIT(VkPhysicalDeviceExternalBufferInfo,                  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO)
VK_STRUCT_INIT(VkExternalBufferProperties,                          VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES)
VK_STRUCT_INIT(VkPhysicalDeviceIDProperties,                        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES)
VK_STRUCT_INIT(VkExternalMemoryBufferCreateInfo,                    VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO)
VK_STRUCT_INIT(VkExternalMemoryImageCreateInfo,                     VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO)
VK_STRUCT_INIT(VkExportMemoryAllocateInfo,                          VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO)
VK_STRUCT_INIT(VkPhysicalDeviceExternalFenceInfo,                   VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_FENCE_INFO)
VK_STRUCT_INIT(VkExternalFenceProperties,                           VK_STRUCTURE_TYPE_EXTERNAL_FENCE_PROPERTIES)
VK_STRUCT_INIT(VkExportFenceCreateInfo,                             VK_STRUCTURE_TYPE_EXPORT_FENCE_CREATE_INFO)
VK_STRUCT_INIT(VkExportSemaphoreCreateInfo,                         VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO)
VK_STRUCT_INIT(VkPhysicalDeviceExternalSemaphoreInfo,               VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO)
VK_STRUCT_INIT(VkExternalSemaphoreProperties,                       VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES)
VK_STRUCT_INIT(VkPhysicalDeviceMaintenance3Properties,              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES)
VK_STRUCT_INIT(VkDescriptorSetLayoutSupport,                        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_SUPPORT)
VK_STRUCT_INIT(VkPhysicalDeviceShaderDrawParametersFeatures,        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES)
VK_STRUCT_INIT(VkPhysicalDeviceVulkan11Features,                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES)
VK_STRUCT_INIT(VkPhysicalDeviceVulkan11Properties,                  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES)
VK_STRUCT_INIT(VkPhysicalDeviceVulkan12Features,                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES)
VK_STRUCT_INIT(VkPhysicalDeviceVulkan12Properties,                  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES)
VK_STRUCT_INIT(VkImageFormatListCreateInfo,                         VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO)
VK_STRUCT_INIT(VkAttachmentDescription2,                            VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2)
VK_STRUCT_INIT(VkAttachmentReference2,                              VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2)
VK_STRUCT_INIT(VkSubpassDescription2,                               VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2)
VK_STRUCT_INIT(VkSubpassDependency2,                                VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2)
VK_STRUCT_INIT(VkRenderPassCreateInfo2,                             VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2)
VK_STRUCT_INIT(VkSubpassBeginInfo,                                  VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO)
VK_STRUCT_INIT(VkSubpassEndInfo,                                    VK_STRUCTURE_TYPE_SUBPASS_END_INFO)
VK_STRUCT_INIT(VkPhysicalDevice8BitStorageFeatures,                 VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES)
VK_STRUCT_INIT(VkPhysicalDeviceDriverProperties,                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES)
VK_STRUCT_INIT(VkPhysicalDeviceShaderAtomicInt64Features,           VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES)
VK_STRUCT_INIT(VkPhysicalDeviceShaderFloat16Int8Features,           VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES)
VK_STRUCT_INIT(VkPhysicalDeviceFloatControlsProperties,             VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES)
VK_STRUCT_INIT(VkDescriptorSetLayoutBindingFlagsCreateInfo,         VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO)
VK_STRUCT_INIT(VkPhysicalDeviceDescriptorIndexingFeatures,          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES)
VK_STRUCT_INIT(VkPhysicalDeviceDescriptorIndexingProperties,        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES)
VK_STRUCT_INIT(VkDescriptorSetVariableDescriptorCountAllocateInfo,  VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO)
VK_STRUCT_INIT(VkDescriptorSetVariableDescriptorCountLayoutSupport, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_LAYOUT_SUPPORT)
VK_STRUCT_INIT(VkPhysicalDeviceDepthStencilResolveProperties,       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES)
VK_STRUCT_INIT(VkSubpassDescriptionDepthStencilResolve,             VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE)
VK_STRUCT_INIT(VkPhysicalDeviceScalarBlockLayoutFeatures,           VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES)
VK_STRUCT_INIT(VkImageStencilUsageCreateInfo,                       VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO)
VK_STRUCT_INIT(VkPhysicalDeviceSamplerFilterMinmaxProperties,       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_FILTER_MINMAX_PROPERTIES)
VK_STRUCT_INIT(VkSamplerReductionModeCreateInfo,                    VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO)
VK_STRUCT_INIT(VkPhysicalDeviceVulkanMemoryModelFeatures,           VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES)
VK_STRUCT_INIT(VkPhysicalDeviceImagelessFramebufferFeatures,        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES)
VK_STRUCT_INIT(VkFramebufferAttachmentsCreateInfo,                  VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO)
VK_STRUCT_INIT(VkFramebufferAttachmentImageInfo,                    VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO)
VK_STRUCT_INIT(VkRenderPassAttachmentBeginInfo,                     VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO)
VK_STRUCT_INIT(VkPhysicalDeviceUniformBufferStandardLayoutFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES)
VK_STRUCT_INIT(VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES)
VK_STRUCT_INIT(VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES)
VK_STRUCT_INIT(VkAttachmentReferenceStencilLayout,                  VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_STENCIL_LAYOUT)
VK_STRUCT_INIT(VkAttachmentDescriptionStencilLayout,                VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_STENCIL_LAYOUT)
VK_STRUCT_INIT(VkPhysicalDeviceHostQueryResetFeatures,              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES)
VK_STRUCT_INIT(VkPhysicalDeviceTimelineSemaphoreFeatures,           VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES)
VK_STRUCT_INIT(VkPhysicalDeviceTimelineSemaphoreProperties,         VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES)
VK_STRUCT_INIT(VkSemaphoreTypeCreateInfo,                           VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO)
VK_STRUCT_INIT(VkTimelineSemaphoreSubmitInfo,                       VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO)
VK_STRUCT_INIT(VkSemaphoreWaitInfo,                                 VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO)
VK_STRUCT_INIT(VkSemaphoreSignalInfo,                               VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO)
VK_STRUCT_INIT(VkPhysicalDeviceBufferDeviceAddressFeatures,         VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES)
VK_STRUCT_INIT(VkBufferDeviceAddressInfo,                           VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO)
VK_STRUCT_INIT(VkBufferOpaqueCaptureAddressCreateInfo,              VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO)
VK_STRUCT_INIT(VkMemoryOpaqueCaptureAddressAllocateInfo,            VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO)
VK_STRUCT_INIT(VkDeviceMemoryOpaqueCaptureAddressInfo,              VK_STRUCTURE_TYPE_DEVICE_MEMORY_OPAQUE_CAPTURE_ADDRESS_INFO)

#undef VK_STRUCT_INIT

template<typename T>
inline constexpr T vk_init() {
    T t = {};
    vk_init_struct_stype(t);
    return t;
}

struct VkStructInitializer {
    template<typename T>
    constexpr operator T() const {
        return vk_init<T>();
    }
};

struct VkNull {
    template<typename T>
    constexpr operator T() const {
        return T{};
    }
};
}

inline constexpr auto vk_struct() {
    return detail::VkStructInitializer();
}

inline constexpr auto vk_null() {
    return detail::VkNull();
}

inline constexpr bool is_error(VkResult result) {
    return result != VK_SUCCESS;
}
inline constexpr bool is_incomplete(VkResult result) {
    return result == VK_INCOMPLETE;
}

const char* vk_result_str(VkResult result);

inline void vk_check(VkResult result) {
    y_always_assert(!is_error(result), vk_result_str(result));
}

inline void vk_check_or_incomplete(VkResult result) {
    y_always_assert(is_incomplete(result) || !is_error(result), vk_result_str(result));
}



template<typename T>
class VkHandle {
    public:
        VkHandle() = default;

        VkHandle(T t) : _t(t) {
        }

        VkHandle(VkHandle&& other) {
            std::swap(_t, other._t);
        }

        VkHandle& operator=(VkHandle&& other) {
            std::swap(_t, other._t);
            return *this;
        }

        VkHandle& operator=(T other) {
            _t = other;
            return *this;
        }

        void swap(VkHandle& other) {
            std::swap(_t, other._t);
        }

        operator T() const {
            return _t;
        }

        T& get() {
            return _t;
        }

        const T& get() const {
            return _t;
        }

    private:
        T _t = {};
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

