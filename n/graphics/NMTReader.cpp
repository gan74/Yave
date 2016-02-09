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

#include <n/io/File.h>
#include "MaterialLoader.h"
#include "ImageLoader.h"


namespace n {
namespace graphics {
class NmtReader : public MaterialLoader::MaterialReader<NmtReader, core::String>
{

	public:
		NmtReader() : MaterialLoader::MaterialReader<NmtReader, core::String>() {
		}

		MaterialData *operator()(core::String fileName) override {
			if(!fileName.endsWith(".nmt")) {
				return 0;
			}
			io::File file(fileName);
			return load(file);
		}

	private:
		MaterialData *load(io::File &file) {
			if(!file.open(io::IODevice::Read)) {
				logMsg(file.getName() + " not found", ErrorLog);
				return 0;
			}
			N_LOG_PERF;
			uint fs = file.size();
			char *data = new char[fs + 1];
			file.readBytes(data);
			data[fs] = 0;
			file.close();
			N_SCOPE(delete[] data);


			uint version = 0;
			memcpy(&version, data, sizeof(uint));

			switch(version) {
				case 1:
					return loadV1(data);

				default:
					return 0;
			}
			return 0;
		}

		MaterialData *loadV1(char *data) {
			struct Header
			{
				uint version;
				uint shaderCodeOffset;
				uint shaderLen;
				uint renderDataOffset;
				uint texHeaderOffset[4];
			} header;
			memcpy(&header, data, sizeof(Header));
			if(header.version != 1) {
				logMsg("Invalid material version", ErrorLog);
			}

			MaterialData *mat = new MaterialData();


			if(header.renderDataOffset != 0) {
				memcpy(&mat->render, data + header.renderDataOffset, sizeof(uint32));
			}

			if(header.shaderLen) {
				core::String shader(data + header.shaderCodeOffset, header.shaderLen);
				mat->prog = ShaderProgram(new Shader<FragmentShader>(shader));
			}

			for(uint i = 0; i != 4; i++) {
				uint offset = header.texHeaderOffset[i];
				if(offset) {
					struct TexHeader
					{
						uint nameLen;
					} h;
					memcpy(&h, data + offset, sizeof(TexHeader));

					core::String texName(data + offset, h.nameLen);
					mat->surface.textures[i] = Texture(ImageLoader::load<core::String>(texName));
				}
			}


			return mat;
		}

};

}
}
