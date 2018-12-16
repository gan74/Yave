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

#include <y/utils.h>

#include "renderdochelper.h"

#if __has_include("renderdoc_app.h")
#include "renderdoc_app.h"
#define RENDERDOC_SUPPORTED
#elif __has_include(<renderdoc_app.h>)
#include <renderdoc_app.h>
#define RENDERDOC_SUPPORTED
#else
#warning RenderDoc helper not supported: header missing
#endif

namespace editor {
namespace renderdoc {

#ifdef RENDERDOC_SUPPORTED

RENDERDOC_API_1_1_2 *renderdoc_api = nullptr;

void init_renderdoc() {
#ifdef Y_OS_WIN
	if(renderdoc_api) {
		return;
	}
	if(HMODULE module = GetModuleHandleA("renderdoc.dll")) {
		pRENDERDOC_GetAPI RENDERDOC_GetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(GetProcAddress(module, "RENDERDOC_GetAPI"));
		int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, reinterpret_cast<void**>(&renderdoc_api));
		if(ret != 1) {
			log_msg("Unable to load RenderDoc API", Log::Warning);
		}
	}
#else
#warning RenderDoc helper not supported: unsupported OS
#endif
}

bool is_supported() {
	init_renderdoc();
	return renderdoc_api;
}

void start_capture() {
	init_renderdoc();
	if(renderdoc_api) {
		renderdoc_api->StartFrameCapture(nullptr, nullptr);
	}
}

void end_capture() {
	if(renderdoc_api) {
		renderdoc_api->EndFrameCapture(nullptr, nullptr);
	}
}

#else

bool is_supported() {
	return false;
}

void start_capture() {
}

void end_capture() {
}

#endif

}
}
