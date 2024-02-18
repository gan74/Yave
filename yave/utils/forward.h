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
#ifndef YAVE_UTILS_FORWARD_H
#define YAVE_UTILS_FORWARD_H

// Auto generated: forward definitions for most non template classes
// TODO: Nested classes will not be declared correctly

namespace yave {
class AABB;
class AABBUpdateSystem;
class Animation;
class AnimationChannel;
class AssetDependencies;
class AssetLoader;
class AssetLoaderSystem;
class AssetLoadingContext;
class AssetLoadingThreadPool;
class AssetStore;
class AtmosphereComponent;
class BufferBarrier;
class BufferBase;
class BufferMappingBase;
class Camera;
class CmdBufferData;
class CmdBufferPool;
class CmdBufferRecorder;
class CmdBufferRecorderBase;
class CmdBufferRegion;
class CmdQueue;
class CmdTimingRecorder;
class ComputeCmdBufferRecorder;
class ComputeProgram;
class DebugParams;
class DebugUtils;
class DedicatedDeviceMemoryAllocator;
class Descriptor;
class DescriptorArray;
class DescriptorSet;
class DescriptorSetAllocator;
class DescriptorSetBase;
class DescriptorSetData;
class DescriptorSetLayout;
class DescriptorSetPool;
class DeviceMemory;
class DeviceMemoryAllocator;
class DeviceMemoryHeap;
class DeviceMemoryHeapBase;
class DeviceMemoryView;
class DeviceResources;
class DirectDraw;
class DirectDrawPrimitive;
class DirectionalLightComponent;
class EventHandler;
class FileSystemModel;
class FolderAssetStore;
class FolderFileSystemModel;
class FrameGraph;
class FrameGraphComputePassBuilder;
class FrameGraphDescriptorBinding;
class FrameGraphFrameResources;
class FrameGraphPass;
class FrameGraphPassBuilder;
class FrameGraphPassBuilderBase;
class FrameGraphPersistentResourceId;
class FrameGraphRegion;
class FrameGraphResourceId;
class FrameGraphResourcePool;
class Framebuffer;
class Frustum;
class GenericAssetPtr;
class GraphicPipeline;
class IBLProbe;
class ImageBarrier;
class ImageBase;
class ImageData;
class ImageFormat;
class InlineDescriptor;
class Instance;
class KeyCombination;
class Layout;
class LifetimeManager;
class LoaderBase;
class LoadingJob;
class LocalFileSystemModel;
class LocalLightBase;
class Material;
class MaterialAllocator;
class MaterialCompiler;
class MaterialData;
class MaterialDrawData;
class MaterialTemplate;
class MaterialTemplateData;
class MeshAllocator;
class MeshData;
class MeshDrawBuffers;
class MeshDrawData;
class MeshVertexStreams;
class Octree;
class OctreeData;
class OctreeNode;
class OctreeSystem;
class PhysicalDevice;
class PointLightComponent;
class RenderPass;
class RenderPassRecorder;
class Renderer;
class RendererSystem;
class ResourceFence;
class Sampler;
class SceneView;
class ScriptSystem;
class ScriptWorldComponent;
class SearchableFileSystemModel;
class ShaderModuleBase;
class ShaderProgram;
class Skeleton;
class SkeletonInstance;
class SkyLightComponent;
class SpecializationData;
class SpirVData;
class SpotLightComponent;
class StaticMesh;
class StaticMeshComponent;
class StaticMeshRenderer;
class SubBufferBase;
class Swapchain;
class SwapchainImage;
class TextureLibrary;
class Timeline;
class TimelineFence;
class TimestampQuery;
class TimestampQueryPool;
class TimestampQueryPoolData;
class TransferCmdBufferRecorder;
class TransformManager;
class TransformableComponent;
class TransientBuffer;
class Window;
struct AABBTypeInfo;
struct Allocator;
struct AssetData;
struct AssetDesc;
struct AssetId;
struct AsyncSubmitData;
struct AtmospherePass;
struct AtmosphereSettings;
struct Attachment;
struct AttachmentData;
struct Attribute;
struct BloomPass;
struct BloomSettings;
struct BlurPass;
struct BlurSettings;
struct Bone;
struct BoneKey;
struct BoneTransform;
struct BufferCopyInfo;
struct BufferCreateInfo;
struct BufferData;
struct CameraBufferPass;
struct DefaultRenderer;
struct DescriptorArraySet;
struct DeviceProperties;
struct DirectVertex;
struct DownsamplePass;
struct EmptyResource;
struct EnclosingSphere;
struct Entry;
struct EntryInfo;
struct Event;
struct ExposurePass;
struct FrameGraphBufferId;
struct FrameGraphImageId;
struct FrameGraphMutableBufferId;
struct FrameGraphMutableImageId;
struct FrameGraphMutableResourceId;
struct FrameGraphMutableVolumeId;
struct FrameGraphVolumeId;
struct FrameSyncObjects;
struct FrameToken;
struct FreeBlock;
struct FullVertex;
struct GBufferPass;
struct Hasher;
struct IdBufferPass;
struct ImageClearInfo;
struct ImageCopyInfo;
struct ImageCreateInfo;
struct ImageSampler;
struct InlineBlock;
struct InlineStorage;
struct KeyEqual;
struct KeyHash;
struct LayoutPools;
struct Level;
struct LightingPass;
struct LightingSettings;
struct LoadableComponentTypeInfo;
struct MeshDrawCommand;
struct Mip;
struct Monitor;
struct OneShotScript;
struct PackedVertex;
struct Plane;
struct Pool;
struct QueryResult;
struct Region;
struct RendererSettings;
struct ResourceCreateInfo;
struct ResourceUsageInfo;
struct RunOnce;
struct RunOnceResult;
struct SSAOPass;
struct SSAOSettings;
struct SceneRenderSubPass;
struct Script;
struct ShadowMapPass;
struct ShadowMapSettings;
struct SkeletonData;
struct SkinWeights;
struct SkinnedVertex;
struct StaticMeshRenderSubPass;
struct SubMesh;
struct TAAPass;
struct TAASettings;
struct ToneMappingPass;
struct ToneMappingSettings;
struct Viewport;
struct VolumeSampler;
}


namespace yave::detail {
class AssetPtrDataBase;
struct VkStructInitializer;
}


namespace yave::ecs {
class ComponentBoxBase;
class ComponentContainerBase;
class ComponentInspector;
class EntityId;
class EntityPool;
class EntityPrefab;
class EntityWorld;
class SparseIdSet;
class SparseIdSetBase;
class System;
class WorldComponentContainerBase;
struct ComponentRuntimeInfo;
struct ComponentsReturnPolicy;
struct Entity;
struct IdComponents;
struct IdComponentsReturnPolicy;
struct QueryUtils;
struct SetMatch;
}


namespace yave::uniform {
struct AtmosphereParams;
struct Camera;
struct DirectionalLight;
struct ExposureParams;
struct MaterialData;
struct PointLight;
struct ShadowMapParams;
struct SpotLight;
struct TransformableData;
}


#endif // YAVE_UTILS_FORWARD_H