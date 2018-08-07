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
#ifndef YAVE_DEVICE_EXTENTIONS_DEBUGMARKER_H
#define YAVE_DEVICE_EXTENTIONS_DEBUGMARKER_H

#include <yave/graphics/vk/vk.h>

namespace yave {

class DebugMarker : NonCopyable {
	public:
		static const char* name();

		DebugMarker(vk::Device device);

		void begin_region(vk::CommandBuffer buffer, const char* name, const math::Vec4& color = math::Vec4()) const;
		void end_region(vk::CommandBuffer buffer) const;

	private:
		vk::Device _device;

		PFN_vkDebugMarkerSetObjectNameEXT _set_object_name;
		PFN_vkCmdDebugMarkerBeginEXT _begin_region;
		PFN_vkCmdDebugMarkerEndEXT _end_region;

};

}

#endif // YAVE_DEVICE_EXTENTIONS_DEBUGMARKER_H
