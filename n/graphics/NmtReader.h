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

class NmtMaterialReader : public MaterialLoader::AssetReader<NmtMaterialReader, core::String>
{
	public:
		NmtMaterialReader();
		virtual MaterialData *operator()(core::String fileName) override;
};


class NmtReader
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

		struct Version2Header
		{
			uint version;
			uint numData;
		};

		struct Version1TexHeader
		{
			uint nameLen;
		};

		struct DataHeader
		{
			uint offset;
			uint id;
		};


		constexpr static uint FragShader = 0;
		constexpr static uint RenderData = 1;
		constexpr static uint TextureName = 2;
		constexpr static uint Reserved = 1024;


		template<typename T>
		void addReader(uint id, T t) {
			readers[id] = t;
		}

		NmtReader();
		MaterialData *load(const char *data, void *args = 0);

	private:
		core::Map<uint, core::Functor<void(const char *, uint, MaterialData *, void *)>> readers;


		MaterialData *loadV1(const char *data, void *);
		MaterialData *loadV2(const char *data, void *args);

};

}
}

#endif // N_GRAPHICS_NMTREADER_H
