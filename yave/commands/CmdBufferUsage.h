/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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
#ifndef YAVE_COMMANDS_CMDBUFFERUSAGE_H
#define YAVE_COMMANDS_CMDBUFFERUSAGE_H

#include <yave/vk/vk.h>

#include <y/concurrent/Arc.h>
#include <yave/device/DeviceLinked.h>

namespace yave {

enum class CmdBufferUsage {
	Primary = uenum(vk::CommandBufferUsageFlagBits()), // eSimultaneousUse ?
	Disposable = uenum(vk::CommandBufferUsageFlagBits::eOneTimeSubmit),
	Secondary = uenum((std::max(Primary, Disposable) << 1)) | uenum(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
};

class CmdBufferBase;
class CmdBufferPoolBase;

template<CmdBufferUsage Usage = CmdBufferUsage::Primary>
class CmdBufferPool;

template<CmdBufferUsage Usage = CmdBufferUsage::Primary>
class RecordedCmdBuffer;

template<CmdBufferUsage Usage = CmdBufferUsage::Primary>
class CmdBuffer;

template<CmdBufferUsage Usage = CmdBufferUsage::Primary>
class CmdBufferRecorder;

}

#endif // YAVE_COMMANDS_CMDBUFFERUSAGE_H
