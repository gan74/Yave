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
class LoaderBase;
class OctreeNode;
class StaticMesh;
struct AssetId;
class AccelerationStructure;
class DeviceMemoryHeapBase;
class AssetLoadingThreadPool;
class FolderAssetStore;
struct Viewport;
struct SkinWeights;
class DescriptorSetData;
class SubBufferBase;
class AssetDependencies;
class LoadingJob;
class ImageBarrier;
struct BloomSettings;
struct SceneData;
class InlineDescriptor;
struct FrameGraphBufferId;
class ImageFormat;
class DescriptorSetLayout;
class SpirVData;
struct LayoutPools;
struct Vertex;
struct DefaultRenderer;
class CmdBufferPool;
class DescriptorSetBase;
class ImageBase;
class Device;
class MeshData;
struct DeviceProperties;
class OctreeData;
struct LoadableComponentRegister;
class SimpleMaterialData;
class FolderFileSystemModel;
class TimeQuery;
class ShaderProgram;
class Camera;
struct FrameGraphMutableResourceId;
class DeviceResources;
struct ToneMappingPass;
class MaterialTemplate;
class MaterialTemplateData;
class DebugParams;
class Sampler;
struct ResourceCreateInfo;
class Swapchain;
struct Attachment;
struct BlurSettings;
struct Attribute;
struct DownsamplePass;
class SkyLightComponent;
class DeviceMemoryAllocator;
class DeviceMemory;
struct SkeletonData;
class PointLightComponent;
struct FreeBlock;
struct BoneKey;
class SkeletonInstance;
class DescriptorSetAllocator;
struct BlurPass;
class FrameGraphResourcePool;
class DebugUtils;
class SceneView;
class DeviceMemoryHeap;
struct ImageCreateInfo;
struct Box;
class CmdBufferRegion;
class AtmosphereComponent;
class GenericAssetPtr;
class Queue;
class SkinnedMesh;
class DirectionalLightComponent;
class TransientBuffer;
class ResourceFence;
struct Semaphores;
class SpecializationData;
struct KeepAlive;
struct BoneTransform;
class Octree;
class AssetLoaderSystem;
struct ShadowMapPass;
class IBLProbe;
class FrameGraphPass;
class ComputeProgram;
class RayTracing;
class SQLiteFileSystemModel;
class LocalFileSystemModel;
struct SkinnedVertex;
class LifetimeManager;
class OctreeSystem;
struct SceneRenderSubPass;
struct InlineStorage;
struct LightingPass;
class RenderPassRecorder;
struct EmptyResource;
class FrameGraphResourceId;
struct SSAOSettings;
struct RendererSettings;
class SQLiteAssetStore;
class FrameGraphRegion;
class DescriptorSetPool;
class AssetLoader;
class ShaderModuleBase;
class OctreeEntityId;
struct FrameToken;
struct SubPass;
struct SubMesh;
struct AtmospherePass;
class FrameGraphPassBuilder;
class Window;
class MaterialCompiler;
class Descriptor;
class AssetLoadingContext;
class SwapchainImage;
class Instance;
class SpotLightComponent;
struct Registerer;
struct ShadowMapSettings;
struct Monitor;
class Skeleton;
class CmdBufferData;
class BufferBase;
class Layout;
class EventHandler;
struct FrameGraphMutableImageId;
struct CmdBuffer;
class AssetStore;
struct SSAOPass;
struct ResourceUsageInfo;
class ThreadLocalDevice;
class TransformableComponent;
class Renderable;
class AnimationChannel;
class FrameGraphFrameResources;
struct GBufferPass;
class ImageData;
struct Bone;
struct BufferCreateInfo;
struct Region;
struct AttachmentData;
class DeviceMemoryView;
class DedicatedDeviceMemoryAllocator;
struct Contants;
struct FrameGraphImageId;
struct BloomPass;
struct InlineBlock;
class AssetIdFactory;
class Framebuffer;
class RenderPass;
class Material;
struct AssetData;
class AABB;
class FrameGraph;
class PushConstant;
class Mapping;
struct EnclosingSphere;
class FrameGraphDescriptorBinding;
class DescriptorSet;
class FileSystemModel;
class Animation;
struct LightingSettings;
struct SubPassData;
class BufferBarrier;
class SearchableFileSystemModel;
class PhysicalDevice;
struct ImageCopyInfo;
class GraphicPipeline;
struct LoadableComponentTypeInfo;
class CmdBufferRecorder;
class Frustum;
struct ToneMappingSettings;
struct FrameGraphMutableBufferId;
}


namespace yave::detail {
class AssetPtrDataBase;
struct VkNull;
struct VkStructInitializer;
}


namespace yave::ecs {
struct IdComponentsReturnPolicy;
class EntityId;
struct ComponentRuntimeInfo;
class ComponentBoxBase;
class SparseIdSet;
class ComponentContainerBase;
class IdComponents;
class EntityWorld;
struct ComponentsReturnPolicy;
class System;
class EntityPrefab;
struct IdReturnPolicy;
class Archetype;
class EntityIdPool;
}


namespace yave::uniform {
struct SH;
struct ShadowMapParams;
struct ToneMappingParams;
struct PointLight;
struct DirectionalLight;
struct Camera;
struct SpotLight;
struct RayleighSky;
struct Surfel;
}


#endif // YAVE_UTILS_FORWARD_H