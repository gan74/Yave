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

#include "NmtReader.h"

namespace n {
namespace graphics {

NmtMaterialReader::NmtMaterialReader() : MaterialLoader::AssetReader<NmtMaterialReader, core::String>() {
}

MaterialData *NmtMaterialReader::operator()(core::String fileName) {
	if(!fileName.endsWith(".nmt")) {
		return 0;
	}
	io::File file(fileName);

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

	NmtReader reader;
	return reader.load(data);
}





NmtReader::NmtReader() {

	addReader(FragShader, [](const char *data, uint offset, MaterialData *mat, void *) {
		logMsg("shader");
		uint length = 0;
		memcpy(&length, data + offset, sizeof(uint));
		if(length) {
			core::String shader(data + offset + sizeof(uint), length);
			mat->prog = ShaderProgram(new Shader<FragmentShader>(shader));
		}
	});

	addReader(RenderData, [](const char *data, uint offset, MaterialData *mat, void *) {
		logMsg("data");
		memcpy(&mat->render, data + offset, sizeof(mat->render));
	});

	addReader(TextureName, [](const char *data, uint offset, MaterialData *mat, void *) {
		logMsg("texture");
		uint length = 0;
		memcpy(&length, data + offset, sizeof(uint));
		if(length) {
			uint id = 0;
			memcpy(&id, data + offset + sizeof(uint), sizeof(uint));
			core::String texName(data + offset + 2 * sizeof(uint), length);
			mat->surface.textures[id] = Texture(ImageLoader::load<core::String>(texName), true);
		}
	});
}


MaterialData *NmtReader::load(const char *data, void *args) {
	uint version = 0;
	memcpy(&version, data, sizeof(uint));

	switch(version) {
		case 1:
			return loadV1(data, args);

		default:
			return loadV2(data, args);
	}
	return 0;
}

MaterialData *NmtReader::loadV2(const char *data, void *args) {
	Version2Header header;
	memcpy(&header, data, sizeof(header));

	if(header.version != 2) {
		logMsg("Invalid material version", ErrorLog);
	}

	DataHeader *datas = new DataHeader[header.numData];
	memcpy(datas, data + sizeof(header), sizeof(DataHeader) * header.numData);

	MaterialData *mat = new MaterialData();

	for(uint i = 0; i != header.numData; i++) {
		readers[datas[i].id](data, datas[i].offset, mat, args);
	}

	return mat;
}

MaterialData *NmtReader::loadV1(const char *data, void *) {
	Version1Header header;
	memcpy(&header, data, sizeof(Version1Header));
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
			Version1TexHeader h;
			memcpy(&h, data + offset, sizeof(Version1TexHeader));

			core::String texName(data + offset, h.nameLen);
			mat->surface.textures[i] = Texture(ImageLoader::load<core::String>(texName), true);
		}
	}


	return mat;
}

}
}
