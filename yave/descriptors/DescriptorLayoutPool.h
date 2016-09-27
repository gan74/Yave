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
#ifndef YAVE_DESCRIPTOR_DESCRIPTORLAYOUTPOOL_H
#define YAVE_DESCRIPTOR_DESCRIPTORLAYOUTPOOL_H

#include <yave/yave.h>
#include <unordered_map>
#include <y/core/AssocVector.h>

#include "DescriptorLayout.h"

namespace yave {

class DescriptorLayoutPool : NonCopyable, public DeviceLinked {

	/*struct Hash {
		template<typename T>
		static auto hash(const T& t) {
			return std::hash<T>(t);
		}

		auto operator()(const core::Vector<ShaderStageResource>& v) {
			return hash(core::range(v));
		}
	};*/

	public:
		DescriptorLayoutPool() = default;
		DescriptorLayoutPool(DevicePtr dptr);

		DescriptorLayoutPool(DescriptorLayoutPool&& other);
		DescriptorLayoutPool& operator=(DescriptorLayoutPool&& other);

		const DescriptorLayout& operator[](const core::Vector<ShaderStageResource>& res);

	private:
		void swap(DescriptorLayoutPool& other);

		core::AssocVector<core::Vector<ShaderStageResource>, core::Ptr<DescriptorLayout>> _layouts;

};

}

#endif // YAVE_DESCRIPTOR_DESCRIPTORLAYOUTPOOL_H
