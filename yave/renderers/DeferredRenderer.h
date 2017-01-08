/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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
#ifndef YAVE_RENDERERS_DEFERREDRENDERER_H
#define YAVE_RENDERERS_DEFERREDRENDERER_H

#include <yave/Framebuffer.h>
#include <yave/shaders/ComputeProgram.h>
#include <yave/scene/SceneView.h>

namespace yave {

class DeferredRenderer : NonCopyable, public DeviceLinked {

	using OutputView = StorageView;

	struct Camera {
		math::Matrix4<> inv_matrix;
		math::Vec3 position;
	};

	using Frustum = std::array<math::Vec4, 6>;

	public:
		DeferredRenderer(SceneView& scene, const math::Vec2ui& size);

		void update();
		void draw(CmdBufferRecorder& recorder, const OutputView& output);

	private:

		const DescriptorSet& create_output_set(const OutputView& out);

		SceneView& _scene;
		math::Vec2ui _size;

		DepthTextureAttachment _depth;
		ColorTextureAttachment _diffuse;
		ColorTextureAttachment _normal;

		Framebuffer _gbuffer;

		ComputeShader _lighting_shader;
		ComputeProgram _lighting_program;

		ComputeShader _culling_shader;
		ComputeProgram _culling_program;

		usize _light_count = 0;
		Buffer<BufferUsage::StorageBit, MemoryFlags::CpuVisible> _lights;
		Buffer<BufferUsage::StorageBit, MemoryFlags::DeviceLocal> _culled_lights;

		TypedBuffer<Camera, BufferUsage::UniformBit> _camera_buffer;
		TypedBuffer<Frustum, BufferUsage::UniformBit> _frustum_buffer;

		DescriptorSet _lighting_set;
		DescriptorSet _culling_set;
		std::unordered_map<VkImageView, DescriptorSet> _output_sets;
};

}


#endif // YAVE_RENDERERS_DEFERREDRENDERER_H
