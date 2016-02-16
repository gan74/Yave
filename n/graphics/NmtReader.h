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
#ifndef N_GRAPHICS_NMTREADER_H
#define N_GRAPHICS_NMTREADER_H

#include <n/io/File.h>
#include "MaterialLoader.h"
#include "ImageLoader.h"

namespace n {
namespace graphics {

class NmtReader : public MaterialLoader::MaterialReader<NmtReader, core::String>
{

	public:
		struct Version1Header
		{
			uint version;
			uint shaderCodeOffset;
			uint shaderLen;
			uint renderDataOffset;
			uint texHeaderOffset[4];
		};

		struct Version1TexHeader
		{
			uint nameLen;
		};

		NmtReader();
		MaterialData *operator()(core::String fileName) override;

	private:
		MaterialData *load(io::File &file);
		MaterialData *loadV1(char *data);

};

}
}

#endif // N_GRAPHICS_NMTREADER_H
