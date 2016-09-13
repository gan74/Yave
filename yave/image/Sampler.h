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
#ifndef YAVE_IMAGE_SAMPLER_H
#define YAVE_IMAGE_SAMPLER_H

#include <yave/yave.h>

#include "Image.h"

namespace yave {

class Sampler : public DeviceLinked {

	public:
		Sampler() = default;
		Sampler(DevicePtr dptr);
		
		~Sampler();

		vk::Sampler get_vk_sampler() const;

	private:
		vk::Sampler _sampler;
};

}

#endif // YAVE_IMAGE_SAMPLER_H
