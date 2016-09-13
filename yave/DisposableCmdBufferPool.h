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
#ifndef YAVE_DISPOSABLECMDBUFFERPOOL_H
#define YAVE_DISPOSABLECMDBUFFERPOOL_H

#include "yave.h"
#include "DeviceLinked.h"
#include "CmdBufferRecorder.h"
#include "DisposableCmdBuffer.h"

namespace yave {

class DisposableCmdBufferPool : NonCopyable, public DeviceLinked {

	public:
		using Recorder = CmdBufferRecorder<DisposableCmdBuffer>;

		DisposableCmdBufferPool(DevicePtr dptr);
		~DisposableCmdBufferPool();

		Recorder allocate();

	private:
		vk::CommandPool _pool;

};

}

#endif // YAVE_DISPOSABLECMDBUFFERPOOL_H
