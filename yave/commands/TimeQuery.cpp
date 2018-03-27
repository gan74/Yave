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

#include "TimeQuery.h"

#include <yave/device/Device.h>

namespace yave {

static vk::QueryPool create_pool(DevicePtr dptr) {
	return dptr->vk_device().createQueryPool(vk::QueryPoolCreateInfo()
			.setQueryCount(2)
			.setQueryType(vk::QueryType::eTimestamp)
		);
}

TimeQuery::TimeQuery(DevicePtr dptr)  : DeviceLinked(dptr), _pool(create_pool(dptr)) {
}

TimeQuery::~TimeQuery() {
	destroy(_pool);
}

void TimeQuery::start(CmdBufferRecorderBase& recorder) {
	recorder.vk_cmd_buffer().writeTimestamp(vk::PipelineStageFlagBits::eAllCommands, _pool, 0);
}

void TimeQuery::stop(CmdBufferRecorderBase& recorder) {
	recorder.vk_cmd_buffer().writeTimestamp(vk::PipelineStageFlagBits::eAllCommands, _pool, 1);
}

core::Duration TimeQuery::get() {
	std::array<u64, 2> results;
	device()->vk_device().getQueryPoolResults<u64>(_pool, 0, 2, results, 0, vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait);
	return core::Duration::nanoseconds(results[1] - results[0]);
}

}
