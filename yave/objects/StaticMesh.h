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
#ifndef YAVE_OBJECTS_STATICMESH_H
#define YAVE_OBJECTS_STATICMESH_H

#include <yave/yave.h>

#include <yave/assets/AssetPtr.h>
#include <yave/material/Material.h>
#include <yave/mesh/StaticMeshInstance.h>
#include <yave/commands/RecordedCmdBuffer.h>

namespace yave {

class StaticMesh {

	public:
		StaticMesh(const AssetPtr<StaticMeshInstance>& instance, const AssetPtr<Material>& material);

	private:
		AssetPtr<StaticMeshInstance> _instance;
		AssetPtr<Material> _material;

};

}

#endif // YAVE_OBJECTS_STATICMESH_H
