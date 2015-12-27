/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

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

#ifndef N_GRAPHICS_DRAWCOMMANDBUFFER_H
#define N_GRAPHICS_DRAWCOMMANDBUFFER_H

#include "RenderBatch.h"

namespace n {
namespace graphics {

class DrawCommandBuffer
{
	public:
		DrawCommandBuffer();
		~DrawCommandBuffer();

		bool push(const RenderBatch &r);
		void present(RenderFlag flags);

		bool isFull() const;
		bool isEmpty() const;
		uint getCapacity() const;
		uint getSize() const;

	private:
		UniformBuffer<math::Matrix4<>> matrixBuffer;
		UniformBuffer<MaterialBufferData> materialBuffer;
		IndirectBuffer cmdBuffer;
		MaterialRenderData renderData;
		ShaderProgram shader;
		uint index;
};

}
}

#endif // N_GRAPHICS_DRAWCOMMANDBUFFER_H
