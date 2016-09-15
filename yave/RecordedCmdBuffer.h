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
#ifndef YAVE_RECORDEDCMDBUFFER_H
#define YAVE_RECORDEDCMDBUFFER_H

#include "CmdBufferState.h"

namespace yave {

class CmdBufferRecorder;

class RecordedCmdBuffer {

	public:
		void submit(vk::Queue queue) {
			_state->submit(queue);
		}

	private:
		friend class CmdBufferRecorder;

		RecordedCmdBuffer(const core::Rc<CmdBufferState>& cmd_buffer) : _state(cmd_buffer) {
		}

		core::Rc<CmdBufferState> _state;
};

}

#endif // YAVE_RECORDEDCMDBUFFER_H
