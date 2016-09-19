/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#include "Sampler.h"

#include <yave/Device.h>

namespace yave {

vk::Sampler create_sampler(DevicePtr dptr) {
	return dptr->get_vk_device().createSampler(vk::SamplerCreateInfo()
			.setMagFilter(vk::Filter::eLinear)
			.setMinFilter(vk::Filter::eLinear)
			.setAddressModeU(vk::SamplerAddressMode::eRepeat)
			.setAddressModeV(vk::SamplerAddressMode::eRepeat)
			.setAddressModeW(vk::SamplerAddressMode::eRepeat)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear)
			.setMinLod(0.0f)
			.setMaxLod(0.0f)
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

vk::Sampler Sampler::get_vk_sampler() const {
	return _sampler;
}


}
