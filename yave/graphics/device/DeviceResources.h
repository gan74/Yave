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
#ifndef YAVE_DEVICE_DEVICERESOURCES_H
#define YAVE_DEVICE_DEVICERESOURCES_H

#include <yave/yave.h>

#include <yave/assets/AssetPtr.h>

#include <yave/graphics/images/Image.h>
#include <yave/graphics/images/ImageView.h>

namespace yave {

class SpirVData;
class ComputeProgram;
class MaterialTemplate;
class Material;
class StaticMesh;
class IBLProbe;

class DeviceResources final : NonMovable {
    public:
        enum SpirV {
            EquirecConvolutionComp,
            CubemapConvolutionComp,
            BRDFIntegratorComp,
            DeferredLocalsComp,
            LinearizeDepthComp,
            SSAOComp,
            SSAOUpsampleComp,
            SSAOUpsampleMergeComp,
            CopyComp,
            HistogramClearComp,
            HistogramComp,
            ToneMapParamsComp,
            DepthBoundComp,

            DeferredPointFrag,
            DeferredSpotFrag,
            DeferredAmbientFrag,
            AtmosphereFrag,
            AtmosphereIntegrationFrag,
            ToneMapFrag,
            TexturedFrag,
            TexturedAlphaFrag,
            TexturedAlphaEmissiveFrag,
            PassthroughFrag,
            DownsampleFrag,
            BloomFrag,
            BloomCombineFrag,
            HBlurFrag,
            VBlurFrag,

            DeferredPointVert,
            DeferredSpotVert,
            BasicVert,
            SkinnedVert,
            ScreenVert,

            MaxSpirV
        };

        enum ComputePrograms {
            EquirecConvolutionProgram,
            CubemapConvolutionProgram,
            BRDFIntegratorProgram,
            DeferredLocalsProgram,
            LinearizeDepthProgram,
            SSAOProgram,
            SSAOUpsampleProgram,
            SSAOUpsampleMergeProgram,
            CopyProgram,
            HistogramClearProgram,
            HistogramProgram,
            ToneMapParamsProgram,
            DepthBoundProgram,

            MaxComputePrograms
        };

        enum MaterialTemplates {
            TexturedMaterialTemplate,
            TexturedAlphaMaterialTemplate,
            TexturedAlphaEmissiveMaterialTemplate,
            TexturedSkinnedMaterialTemplate,

            DeferredPointLightMaterialTemplate,
            DeferredSpotLightMaterialTemplate,
            DeferredAmbientMaterialTemplate,
            AtmosphereMaterialTemplate,
            AtmosphereIntegrationMaterialTemplate,
            ToneMappingMaterialTemplate,

            ScreenPassthroughMaterialTemplate,
            ScreenBlendPassthroughMaterialTemplate,

            DownsampleMaterialTemplate,

            BloomMaterialTemplate,
            BloomCombineMaterialTemplate,

            HBlurMaterialTemplate,
            VBlurMaterialTemplate,

            MaxMaterialTemplates
        };

        enum Materials {
            EmptyMaterial,

            MaxMaterials
        };

        enum Textures {
            BlackTexture,
            WhiteTexture,
            GreyTexture,
            RedTexture,

            FlatNormalTexture,

            SkyIBLTexture,

            MaxTextures
        };

        enum Meshes {
            CubeMesh,
            SphereMesh,
            SimpleSphereMesh,
            ConeMesh,
            SweepMesh,

            MaxMeshes
        };

        DeviceResources();

        // can't default for inclusion reasons
        ~DeviceResources();


        TextureView brdf_lut() const;
        TextureView white_noise() const;

        const AssetPtr<IBLProbe>& ibl_probe() const;
        const AssetPtr<IBLProbe>& empty_probe() const;

        const SpirVData& operator[](SpirV i) const;
        const ComputeProgram& operator[](ComputePrograms i) const;
        const MaterialTemplate* operator[](MaterialTemplates i) const;

        const AssetPtr<Texture>& operator[](Textures i) const;
        const AssetPtr<Material>& operator[](Materials i) const;
        const AssetPtr<StaticMesh>& operator[](Meshes i) const;

    private:
        std::unique_ptr<SpirVData[]> _spirv;
        std::unique_ptr<ComputeProgram[]> _computes;
        std::unique_ptr<MaterialTemplate[]> _material_templates;

        std::unique_ptr<AssetPtr<Texture>[]> _textures;
        std::unique_ptr<AssetPtr<Material>[]> _materials;
        std::unique_ptr<AssetPtr<StaticMesh>[]> _meshes;

        AssetPtr<IBLProbe> _probe;
        AssetPtr<IBLProbe> _empty_probe;

        Texture _brdf_lut;
        Texture _white_noise;
};
}

#endif // YAVE_DEVICE_DEVICERESOURCES_H

