/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include <y/core/String.h>
#include <y/core/HashMap.h>

#include <y/utils/hash.h>


#ifdef Y_DEBUG
#include <mutex>
#endif

namespace yave {

class SpirVData;
class ComputeProgram;
class MaterialTemplate;
class Material;
class StaticMesh;
class IBLProbe;

class DeviceResources final : NonCopyable {
    public:
        enum SpirV {
            EquirecConvolutionComp,
            CubemapConvolutionComp,
            BRDFIntegratorComp,
            DeferredAmbientComp,
            DeferredLocalsComp,
            DepthLinearizeDownSampleComp,
            SSAOComp,
            SSAOUpsampleComp,
            CopyComp,
            HistogramClearComp,
            HistogramComp,
            ToneMapParamsComp,
            DepthBoundComp,

            ToneMapFrag,
            TexturedFrag,
            TexturedAlphaFrag,
            PassthroughFrag,
            BloomFrag,
            HBlurFrag,
            VBlurFrag,

            BasicVert,
            SkinnedVert,
            ScreenVert,

            MaxSpirV
        };

        enum ComputePrograms {
            EquirecConvolutionProgram,
            CubemapConvolutionProgram,
            BRDFIntegratorProgram,
            DeferredAmbientProgram,
            DeferredLocalsProgram,
            DepthLinearizeDownSampleProgram,
            SSAOProgram,
            SSAOUpsampleProgram,
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
            TexturedSkinnedMaterialTemplate,

            ToneMappingMaterialTemplate,

            ScreenPassthroughMaterialTemplate,
            ScreenBlendPassthroughMaterialTemplate,

            BloomMaterialTemplate,

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
            SweepMesh,

            MaxMeshes
        };

        DeviceResources();
        DeviceResources(DevicePtr dptr);

        void init(DevicePtr dptr);

        DeviceResources(DeviceResources&& other);
        DeviceResources& operator=(DeviceResources&& other);

        // can't default for inclusion reasons
        ~DeviceResources();

        bool is_init() const;

        DevicePtr device() const;

        TextureView brdf_lut() const;
        const AssetPtr<IBLProbe>& ibl_probe() const;
        const AssetPtr<IBLProbe>& empty_probe() const;

        const SpirVData& operator[](SpirV i) const;
        const ComputeProgram& operator[](ComputePrograms i) const;
        const MaterialTemplate* operator[](MaterialTemplates i) const;

        const AssetPtr<Texture>& operator[](Textures i) const;
        const AssetPtr<Material>& operator[](Materials i) const;
        const AssetPtr<StaticMesh>& operator[](Meshes i) const;

        void reload();

    private:
        void swap(DeviceResources& other);

        void load_resources();

        DevicePtr _device = nullptr;

        std::unique_ptr<SpirVData[]> _spirv;
        std::unique_ptr<ComputeProgram[]> _computes;
        std::unique_ptr<MaterialTemplate[]> _material_templates;

        std::unique_ptr<AssetPtr<Texture>[]> _textures;
        std::unique_ptr<AssetPtr<Material>[]> _materials;
        std::unique_ptr<AssetPtr<StaticMesh>[]> _meshes;

        AssetPtr<IBLProbe> _probe;
        AssetPtr<IBLProbe> _empty_probe;
        Texture _brdf_lut;


#ifdef Y_DEBUG
        std::unique_ptr<std::recursive_mutex> _lock;
        mutable core::ExternalHashMap<core::String, std::unique_ptr<ComputeProgram>> _programs;

    public:
        const ComputeProgram& program_from_file(std::string_view file) const;
#endif

};
}

#endif // YAVE_DEVICE_DEVICERESOURCES_H

