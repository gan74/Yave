#include "ColorCorrectionRenderer.h"

#include <yave/pipeline/DependencyGraph.h>

#include <y/io/File.h>

namespace yave {

static ComputeShader create_correction_shader(DevicePtr dptr) {
	return ComputeShader(dptr, SpirVData::from_file(io::File::open("correction.comp.spv").expected("Unable to open SPIR-V file.")));
}


ColorCorrectionRenderer::ColorCorrectionRenderer(const Ptr<Renderer>& renderer) :
		_renderer(renderer),
		_correction_shader(create_correction_shader(_renderer->device())),
		_correction_program(_correction_shader) {
}

void ColorCorrectionRenderer::compute_dependencies(const FrameToken& token, DependencyGraphNode& self) {
	self.add_dependency(token, _renderer.as_ptr());
}

void ColorCorrectionRenderer::process(const FrameToken& token, CmdBufferRecorder<>& recorder) {
	auto size = token.image_view.size();
	recorder.dispatch(_correction_program, math::Vec3ui(size / _correction_shader.local_size().sub(3), 1), {create_output_set(token.image_view)});
}

const DescriptorSet& ColorCorrectionRenderer::create_output_set(const StorageView& out) {
	TextureView view = _renderer->view();

	if(out.size() != view.size()) {
		fatal("Invalid output image size.");
	}

	auto it = _output_sets.find(out.vk_image_view());
	if(it == _output_sets.end()) {
		it = _output_sets.insert(std::make_pair(out.vk_image_view(), DescriptorSet(_renderer->device(), {
				Binding(view),
				Binding(out)
			}))).first;
	}
	return it->second;
}

}
