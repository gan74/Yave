#ifndef YAVE_RENDERERS_COLORCORRECTIONRENDERER_H
#define YAVE_RENDERERS_COLORCORRECTIONRENDERER_H

#include <yave/framebuffers/Framebuffer.h>
#include <yave/shaders/ComputeProgram.h>
#include <yave/bindings/uniforms.h>

#include <yave/pipeline/pipeline.h>
#include <yave/device/DeviceLinked.h>

namespace yave {


class ColorCorrectionRenderer : public EndOfPipeline {

	public:
		template<typename T>
		using Ptr = core::Rc<T>;

		ColorCorrectionRenderer(const Ptr<Renderer>& renderer);

	protected:
		void compute_dependencies(const FrameToken& token, RenderingNode& self) override;
		void process(const FrameToken& token, CmdBufferRecorder<>& recorder) override;

	private:
		const DescriptorSet& create_output_set(const StorageView& out);

		Ptr<Renderer> _renderer;

		ComputeShader _correction_shader;
		ComputeProgram _correction_program;

		std::unordered_map<VkImageView, DescriptorSet> _output_sets;
};

}


#endif // YAVE_RENDERERS_COLORCORRECTIONRENDERER_H
