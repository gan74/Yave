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
#ifndef YAVE_RENDERPASS_H
#define YAVE_RENDERPASS_H

#include "yave.h"

#include <yave/image/ImageFormat.h>
#include <yave/DeviceLinked.h>

namespace yave {

class RenderPass : NonCopyable, public DeviceLinked {
	public:
		RenderPass() = default;
		RenderPass(DevicePtr dptr, ImageFormat depth_format, ImageFormat color_format);

		RenderPass(RenderPass&& other);
		RenderPass& operator=(RenderPass&& other);
		
		~RenderPass();

		vk::RenderPass get_vk_render_pass() const;

	private:
		void swap(RenderPass& other);

		vk::RenderPass _render_pass;
};

}

#endif // YAVE_RENDERPASS_H
