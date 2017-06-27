/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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

#include "Sampler.h"

#include <yave/device/Device.h>

namespace yave {

static vk::Sampler create_sampler(DevicePtr dptr) {
	return dptr->vk_device().createSampler(vk::SamplerCreateInfo()
			.setMagFilter(vk::Filter::eLinear)
			.setMinFilter(vk::Filter::eLinear)
			.setAddressModeU(vk::SamplerAddressMode::eRepeat)
			.setAddressModeV(vk::SamplerAddressMode::eRepeat)
			.setAddressModeW(vk::SamplerAddressMode::eRepeat)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear)
			.setMinLod(0.0f)
			.setMaxLod(1000.0f)
			.setCompareEnable(false)
		);
}

Sampler::Sampler(DevicePtr dptr) : DeviceLinked(dptr), _sampler(create_sampler(dptr)) {
}


Sampler::Sampler(Sampler&& other) {
	swap(other);
}

Sampler& Sampler::operator=(Sampler&& other) {
	swap(other);
	return *this;
}

Sampler::~Sampler() {
	destroy(_sampler);
}

void Sampler::swap(Sampler& other) {
	DeviceLinked::swap(other);
	std::swap(_sampler, other._sampler);
}

vk::Sampler Sampler::vk_sampler() const {
	return _sampler;
}


}
