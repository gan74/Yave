/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#ifndef YAVE_DEVICE_DEVICERESOURCES_H
#define YAVE_DEVICE_DEVICERESOURCES_H

#include <yave/yave.h>

#include <yave/assets/AssetPtr.h>
#include <yave/graphics/shaders/SpirVData.h>
#include <yave/graphics/shaders/ShaderModule.h>
#include <yave/graphics/images/Image.h>

namespace yave {

class ComputeProgram;
class MaterialTemplate;


class DeviceResources final : NonCopyable {
	public:
		enum SpirV {
			EquirecConvolutionComp,
			CubemapConvolutionComp,
			BRDFIntegratorComp,
			DeferredLightingComp,
			SSAOComp,
			PickingComp,
			DepthAlphaComp,
			CopyComp,

			TonemapFrag,
			BasicFrag,
			SkinnedFrag,
			TexturedFrag,
			ImguiFrag,

			BasicVert,
			SkinnedVert,
			ScreenVert,
			ImguiVert,

			MaxSpirV
		};

		enum ComputePrograms {
			EquirecConvolutionProgram,
			CubemapConvolutionProgram,
			BRDFIntegratorProgram,
			DeferredLightingProgram,
			SSAOProgram,
			PickingProgram,
			DepthAlphaProgram,
			CopyProgram,

			MaxComputePrograms
		};

		enum MaterialTemplates {
			BasicMaterialTemplate,
			BasicSkinnedMaterialTemplate,

			TexturedMaterialTemplate,

			TonemappingMaterialTemplate,
			ImguiMaterialTemplate,

			MaxMaterialTemplates
		};

		enum Textures {
			BlackTexture,

			MaxTextures
		};

		DeviceResources(DevicePtr dptr);

		// can't default for inclusion reasons
		DeviceResources();
		~DeviceResources();
		DeviceResources(DeviceResources&& other);
		DeviceResources& operator=(DeviceResources&& other);


		const SpirVData& operator[](SpirV i) const;
		const ComputeProgram& operator[](ComputePrograms i) const;
		const MaterialTemplate* operator[](MaterialTemplates i) const;
		const Texture* operator[](Textures i) const;

	private:
		void swap(DeviceResources& other);

		std::unique_ptr<SpirVData[]> _spirv;
		std::unique_ptr<ComputeProgram[]> _computes;
		std::unique_ptr<MaterialTemplate[]> _materials;
		std::unique_ptr<Texture[]> _textures;

};
}

#endif // YAVE_DEVICE_DEVICERESOURCES_H
