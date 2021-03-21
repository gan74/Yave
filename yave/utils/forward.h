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
#ifndef YAVE_UTILS_FORWARD_H
#define YAVE_UTILS_FORWARD_H

// Auto generated: forward definitions for most non template classes
// TODO: Nested classes will not be declared correctly

namespace yave {
class ComputeProgram;
class SQLiteAssetStore;
class ImageFormat;
class MaterialTemplateData;
class Queue;
class PushConstant;
struct Bone;
struct ResourceUsageInfo;
class Instance;
class AccelerationStructure;
class AssetStore;
class GenericAssetPtr;
class DeviceMemoryHeap;
class SimpleMaterialData;
struct ToneMappingSettings;
class LoaderBase;
struct Monitor;
class SpotLightComponent;
class Sampler;
struct Semaphores;
struct BlurSettings;
class LifetimeManager;
class Framebuffer;
class InlineDescriptor;
class ImageBarrier;
class BufferBarrier;
struct CmdBuffer;
struct EnclosingSphere;
struct FrameGraphMutableImageId;
class DeviceMemory;
struct LightingPass;
class DescriptorSetData;
struct SkinWeights;
class SpecializationData;
class DirectionalLightComponent;
class FrameGraphResourcePool;
class LoadingJob;
struct FrameGraphImageId;
class MaterialTemplate;
class AssetIdFactory;
class SceneView;
struct FrameToken;
struct AtmospherePass;
struct SkeletonData;
class Frustum;
class Mapping;
struct BufferCreateInfo;
class ResourceFence;
class SkeletonInstance;
class SearchableFileSystemModel;
struct BoneKey;
class AssetDependencies;
struct BoneTransform;
struct KeepAlive;
class DeviceMemoryHeapBase;
struct Contants;
class ShaderProgram;
struct Viewport;
struct FrameGraphMutableResourceId;
struct BloomSettings;
class AtmosphereComponent;
class RayTracing;
struct SubMesh;
class Swapchain;
class Layout;
struct SSAOPass;
class DeviceResources;
class DeviceMemoryView;
struct AssetData;
struct BlurPass;
struct InlineStorage;
struct LayoutPools;
class AssetLoader;
class CmdBufferData;
struct DownsamplePass;
class FrameGraphPassBuilder;
struct Box;
class CmdBufferPool;
struct Region;
class ShaderModuleBase;
class SwapchainImage;
struct ImageCreateInfo;
struct BloomPass;
struct Attachment;
struct SkinnedVertex;
class StaticMeshComponent;
struct LightingSettings;
class ImageData;
class DeviceMemoryAllocator;
class PhysicalDevice;
struct ShadowMapSettings;
class Descriptor;
class MeshData;
class ImageBase;
class StaticMesh;
class FileSystemModel;
class SkyLightComponent;
class TimeQuery;
class GraphicObject;
class FrameGraphRegion;
class AssetLoadingThreadPool;
class SpirVData;
class FolderFileSystemModel;
struct SceneRenderSubPass;
struct Vertex;
class RenderPassRecorder;
struct SubPassData;
class PointLightComponent;
struct ShadowMapPass;
struct EmptyResource;
class MaterialCompiler;
class FrameGraphDescriptorBinding;
class DedicatedDeviceMemoryAllocator;
class FrameGraph;
class Skeleton;
struct AttachmentData;
class Animation;
class DescriptorSet;
struct DefaultRenderer;
class ThreadLocalDevice;
struct RendererSettings;
class DebugUtils;
class Renderable;
class EventHandler;
class SkinnedMesh;
class RenderPass;
class AABB;
struct Attribute;
class DescriptorSetPool;
struct GBufferPass;
class FolderAssetStore;
class FrameGraphFrameResources;
struct SceneData;
struct SubPass;
struct ResourceCreateInfo;
class DebugParams;
class AssetLoaderSystem;
class IBLProbe;
class SQLiteFileSystemModel;
struct ImageCopyInfo;
class FrameGraphResourceId;
struct FrameGraphBufferId;
struct DeviceProperties;
class CmdBufferRegion;
struct FreeBlock;
class FrameGraphPass;
struct ScopedDevice;
class Material;
struct SSAOSettings;
class Device;
struct FrameGraphMutableBufferId;
class Camera;
class LocalFileSystemModel;
class TransformableComponent;
class CmdBufferRecorder;
class DescriptorSetLayout;
class TransientBuffer;
class Window;
struct ToneMappingPass;
struct InlineBlock;
class BufferBase;
class DescriptorSetBase;
class GraphicPipeline;
class AnimationChannel;
class AssetLoadingContext;
class SubBufferBase;
struct AssetId;
class DescriptorSetAllocator;
}


namespace yave::detail {
struct VkStructInitializer;
class AssetPtrDataBase;
struct VkNull;
}


namespace yave::ecs {
struct IdComponentsReturnPolicy;
class EntityIdPool;
class Archetype;
class ComponentBoxBase;
struct ComponentRuntimeInfo;
class System;
struct IdReturnPolicy;
struct ComponentsReturnPolicy;
class ComponentContainerBase;
class EntityId;
class SparseIdSet;
class IdComponents;
class EntityWorld;
class EntityPrefab;
}


namespace yave::uniform {
struct Camera;
struct SH;
struct ShadowMapParams;
struct Surfel;
struct RayleighSky;
struct PointLight;
struct ToneMappingParams;
struct SpotLight;
struct DirectionalLight;
}


#endif // YAVE_UTILS_FORWARD_H