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
#ifndef YAVE_DEBUGPARAMS_H
#define YAVE_DEBUGPARAMS_H

#include "yave.h"
#include <y/core/Vector.h>

namespace yave {

class DebugParams {

	public:
		static DebugParams debug() {
			return DebugParams({"VK_LAYER_LUNARG_standard_validation"}, true);
		}

		static DebugParams none() {
			return DebugParams({}, false);
		}

		bool is_debug_callback_enabled() const {
			return _callbacks_enabled;
		}

		const core::Vector<const char*>& instance_layers() const {
			return _instance_layers;
		}

		const core::Vector<const char*>& device_layers() const {
			return _device_layers;
		}

	private:
		//friend class LowLevelGraphics;

		DebugParams(std::initializer_list<const char*> instance, std::initializer_list<const char*> device, bool callbacks) :
				_instance_layers(instance),
				_device_layers(device),
				_callbacks_enabled(callbacks) {
		}

		DebugParams(std::initializer_list<const char*> layers, bool callbacks) : DebugParams(layers, layers, callbacks) {
		}

		core::Vector<const char*> _instance_layers;
		core::Vector<const char*> _device_layers;
		bool _callbacks_enabled;
};

}

#endif // YAVE_DEBUGPARAMS_H
