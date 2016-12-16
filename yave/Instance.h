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
#ifndef YAVE_INSTANCE_H
#define YAVE_INSTANCE_H

#include "yave.h"
#include "DebugParams.h"

namespace yave {

class Instance : NonCopyable {
	public:
		Instance(DebugParams debug);
		~Instance();

		const DebugParams& debug_params() const;

		vk::Instance vk_instance() const;

	private:
		void setup_debug();

		core::Vector<const char*> _instance_extentions;

		DebugParams _debug_params;
		vk::Instance _instance;

		VkDebugReportCallbackEXT _debug_callback;
};

}

#endif // YAVE_INSTANCE_H
