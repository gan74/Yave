/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

#include "DeviceResources.h"

#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/graphics/commands/CmdQueue.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/descriptors/DescriptorSetProxy.h>
#include <yave/graphics/barriers/Barrier.h>
#include <yave/graphics/images/ImageView.h>
#include <yave/graphics/shader_structs.h>

#include <y/core/Chrono.h>

namespace yave {

static constexpr u32 transmittance_width = 256;
static constexpr u32 transmittance_height = 64;
static constexpr u32 scattering_width = 256;
static constexpr u32 scattering_height = 64;
static constexpr u32 scattering_depth = 32;
static constexpr u32 irradiance_width = 64;
static constexpr u32 irradiance_height = 16;
static constexpr u32 num_scattering_orders = 4;


Texture create_brdf_lut(const ComputeProgram& brdf_integrator, usize size) {
    y_profile();

    core::DebugTimer _("create_brdf_lut()");

    StorageTexture image(ImageFormat(VK_FORMAT_R16G16_UNORM), {size, size});

    CmdBufferRecorder recorder = create_disposable_cmd_buffer();
    {
        const auto region = recorder.region("create_brdf_lut");
        const auto descriptors = make_descriptor_set(StorageView(image));
        recorder.dispatch_threads(brdf_integrator, image.size(), DescriptorSetProxy(descriptors));
    }
    recorder.submit().wait();

    return image;
}

void create_atmosphere_luts(const DeviceResources& resources, StorageTexture& transmittance_out, StorageVolume& scattering_out) {
    y_profile();
    core::DebugTimer _("create_atmosphere_luts()");

    const ImageFormat format(VK_FORMAT_R32G32B32A32_SFLOAT);
    const math::Vec2ui transmittance_size(transmittance_width, transmittance_height);
    const math::Vec3ui scattering_size(scattering_width, scattering_height, scattering_depth);
    const math::Vec2ui irradiance_size(irradiance_width, irradiance_height);

    const shader::AtmosphereParams atmosphere_params;

    StorageTexture transmittance(format, transmittance_size);
    StorageTexture transmittance_scratch(format, transmittance_size);
    StorageTexture delta_irradiance(format, irradiance_size);

    StorageVolume delta_rayleigh(format, scattering_size);
    StorageVolume delta_mie(format, scattering_size);
    StorageVolume delta_density(format, scattering_size);
    StorageVolume density_scratch(format, scattering_size);
    StorageVolume delta_multiple(format, scattering_size);
    StorageVolume scattering_0(format, scattering_size);
    StorageVolume scattering_1(format, scattering_size);

    StorageVolume* scattering_read = &scattering_0;
    StorageVolume* scattering_write = &scattering_1;

    auto dispatch_single = [&](CmdBufferRecorder& recorder, StorageTexture& transmittance_write, StorageTexture& transmittance_read) {
        const auto descriptors = make_descriptor_set(
            StorageView(transmittance_write),
            StorageView(delta_irradiance),
            Descriptor(TextureView(transmittance_read), SamplerType::LinearClamp),
            VolumeStorageView(delta_rayleigh),
            VolumeStorageView(delta_mie),
            VolumeStorageView(*scattering_write),
            InlineDescriptor(atmosphere_params)
        );
        recorder.dispatch_threads(resources[DeviceResources::AtmosphereSingleProgram], scattering_size, DescriptorSetProxy(descriptors));
    };

    auto dispatch_multiple = [&](CmdBufferRecorder& recorder, u32 scattering_order, StorageVolume& density_write, StorageVolume& density_read) {
        const struct Params {
            u32 scattering_order;
            u32 max_scattering_order;
        } params {
            scattering_order,
            num_scattering_orders,
        };
        const auto descriptors = make_descriptor_set(
            Descriptor(TextureView(transmittance), SamplerType::LinearClamp),
            Descriptor(VolumeView(delta_rayleigh), SamplerType::LinearClamp),
            Descriptor(VolumeView(delta_mie), SamplerType::LinearClamp),
            Descriptor(VolumeView(delta_multiple), SamplerType::LinearClamp),
            Descriptor(TextureView(delta_irradiance), SamplerType::LinearClamp),
            VolumeStorageView(density_write),
            Descriptor(VolumeView(density_read), SamplerType::LinearClamp),
            StorageView(delta_irradiance),
            VolumeStorageView(delta_multiple),
            VolumeStorageView(*scattering_write),
            Descriptor(VolumeView(*scattering_read), SamplerType::LinearClamp),
            InlineDescriptor(atmosphere_params),
            InlineDescriptor(params)
        );
        recorder.dispatch_threads(resources[DeviceResources::AtmosphereMultipleProgram], scattering_size, DescriptorSetProxy(descriptors));
    };

    CmdBufferRecorder recorder = create_disposable_cmd_buffer();
    {
        const auto region = recorder.region("create_atmosphere_luts");

        // Write transmittance, then re-run with it readable for irradiance + single scattering.
        dispatch_single(recorder, transmittance, transmittance_scratch);
        recorder.full_barrier();
        dispatch_single(recorder, transmittance_scratch, transmittance);
        recorder.full_barrier();
        std::swap(scattering_read, scattering_write);

        for(u32 scattering_order = 2; scattering_order <= num_scattering_orders; ++scattering_order) {
            // Write density, then re-run with it readable for multiple scattering.
            dispatch_multiple(recorder, scattering_order, delta_density, density_scratch);
            recorder.full_barrier();
            dispatch_multiple(recorder, scattering_order, density_scratch, delta_density);
            recorder.full_barrier();
            std::swap(scattering_read, scattering_write);
        }
    }

    recorder.submit().wait();

    transmittance_out = std::move(transmittance);
    scattering_out = std::move(*scattering_read);
}

}
