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
#ifndef YAVE_UTILS_FORWARD_H
#define YAVE_UTILS_FORWARD_H

// Auto generated: forward definitions for most non template classes
// TODO: Nested classes will not be declared correctly

namespace yave {
class AABB;
class AccelerationStructure;
class Animation;
class AnimationChannel;
class AssetDependencies;
class AssetLoader;
class AssetLoaderSystem;
class AssetLoadingContext;
class AssetLoadingThreadPool;
class AssetStore;
class AtmosphereComponent;
class BLAS;
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
class CmdTimestampPool;
class ComputeCapableCmdBufferRecorder;
class ComputeCmdBufferRecorder;
class ComputeProgram;
class DebugUtils;
class DedicatedDeviceMemoryAllocator;
class Descriptor;
class DescriptorArray;
class DescriptorLayoutAllocator;
class DeviceMemory;
class DeviceMemoryAllocator;
class DeviceMemoryHeap;
class DeviceMemoryHeapBase;
class DeviceMemoryView;
class DeviceResources;
class DiagnosticCheckpoints;
class DirectDraw;
class DirectDrawPrimitive;
class DirectionalLightComponent;
class EcsScene;
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
class PhysicalDevice;
class PointLightComponent;
class RaytracingProgram;
class RenderPass;
class RenderPassRecorder;
class ResourceFence;
class Sampler;
class Scene;
class SceneView;
class ShaderModuleBase;
class ShaderProgram;
class ShaderProgramBase;
class Skeleton;
class SkeletonInstance;
class SkyLightComponent;
class SpirVData;
class SpotLightComponent;
class StaticMesh;
class StaticMeshComponent;
class SubBufferBase;
class Swapchain;
class SwapchainImage;
class TLAS;
class TextureLibrary;
class Timeline;
class TimelineFence;
class TransferCmdBufferRecorder;
class TransformManager;
class TransformableComponent;
class TransientBuffer;
class TransientMipViewContainer;
class Window;
struct AOPass;
struct AOSettings;
struct Allocator;
struct AssetData;
struct AssetDesc;
struct AssetId;
struct AsyncSubmitData;
struct AtmosphereObject;
struct AtmospherePass;
struct AtmosphereSettings;
struct Attachment;
struct Attribute;
struct BloomPass;
struct BloomSettings;
struct BlurPass;
struct BlurSettings;
struct Bone;
struct BoneTransform;
struct BufferCopyInfo;
struct BufferCreateInfo;
struct BufferData;
struct CameraBufferPass;
struct DefaultRenderer;
struct DenoisePass;
struct DenoiseSettings;
struct DescriptorSetCommon;
struct DeviceProperties;
struct DirectVertex;
struct DownsamplePass;
struct EmptyResource;
struct EnclosingSphere;
struct Entry;
struct EntryInfo;
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
struct InlineStorage;
struct InstanceParams;
struct JitterSettings;
struct KeyEqual;
struct KeyHash;
struct Level;
struct LightingPass;
struct LightingSettings;
struct LoadableComponentTypeInfo;
struct MeshDrawCommand;
struct Mip;
struct Monitor;
struct ObjectIndices;
struct PackedVertex;
struct RaytracingPass;
struct Region;
struct RendererSettings;
struct ResourceCreateInfo;
struct ResourceUsageInfo;
struct SceneRenderSubPass;
struct SceneVisibility;
struct SceneVisibilitySubPass;
struct ShadowMapPass;
struct ShadowMapSettings;
struct SkinWeights;
struct SkinnedVertex;
struct TAAPass;
struct TAASettings;
struct TemporalPrePass;
struct TimedZone;
struct ToneMappingPass;
struct ToneMappingSettings;
struct TransformData;
struct TransformableSceneObjectData;
struct Viewport;
struct Zone;
}


namespace editor {
struct DebugValues;
}


namespace yave::detail {
class AssetPtrDataBase;
struct VkStructInitializer;
}


namespace yave::ecs {
class ArgumentResolver;
class ComponentBoxBase;
class ComponentContainerBase;
class ComponentInspector;
class ComponentMatrix;
class EntityGroupProvider;
class EntityId;
class EntityPool;
class EntityPrefab;
class EntityWorld;
class SparseIdSet;
class SparseIdSetBase;
class System;
class SystemManager;
class SystemScheduler;
class TickId;
struct ComponentReturnPolicy;
struct ComponentRuntimeInfo;
struct Entity;
struct FirstTime;
struct IdComponentReturnPolicy;
struct Schedule;
struct SparseElement;
struct Task;
}


#endif // YAVE_UTILS_FORWARD_H