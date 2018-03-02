/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include "DebugMarker.h"

#include <yave/device/Instance.h>
#include <yave/vk/vk.h>

namespace yave {

const char* DebugMarker::name() {
	return VK_EXT_DEBUG_MARKER_EXTENSION_NAME;
}

DebugMarker::DebugMarker(vk::Device device) : _device(device) {
	_set_object_name = reinterpret_cast<PFN_vkDebugMarkerSetObjectNameEXT>(device.getProcAddr("vkDebugMarkerSetObjectNameEXT"));
	_begin_region = reinterpret_cast<PFN_vkCmdDebugMarkerBeginEXT>(device.getProcAddr("vkCmdDebugMarkerBeginEXT"));
	_end_region = reinterpret_cast<PFN_vkCmdDebugMarkerEndEXT>(device.getProcAddr("vkCmdDebugMarkerEndEXT"));
}


void DebugMarker::begin_region(vk::CommandBuffer buffer, const char* name, const math::Vec4& color) const {
	VkDebugMarkerMarkerInfoEXT marker = vk::DebugMarkerMarkerInfoEXT()
			.setPMarkerName(name)
			.setColor({{color.x(), color.y(), color.z(), color.w()}})
		;

	_begin_region(buffer, &marker);
}

void DebugMarker::end_region(vk::CommandBuffer buffer) const {
	_end_region(buffer);
}

}
