/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#ifndef EDITOR_CONTEXT_EDITORRESOURCES_H
#define EDITOR_CONTEXT_EDITORRESOURCES_H

#include <editor/editor.h>

#include <yave/graphics/images/Image.h>

namespace editor {

class EditorResources final : NonMovable {
	public:
		enum SpirV {
			DepthAlphaComp,
			PickingComp,

			ImGuiFrag,
			ImGuiBillBoardFrag,
			PickingFrag,
			CopyTargetFrag,
			WireFrameFrag,

			ScreenVert,
			ImGuiVert,
			ImGuiBillBoardVert,
			PickingVert,
			WireFrameVert,

			ImGuiBillBoardGeom,

			MaxSpirV
		};

		enum ComputePrograms {
			DepthAlphaProgram,
			PickingProgram,

			MaxComputePrograms
		};

		enum MaterialTemplates {
			ImGuiMaterialTemplate,
			ImGuiBillBoardMaterialTemplate,

			PickingMaterialTemplate,
			ImGuiBillBoardPickingMaterialTemplate,

			CopyTargetMaterialTemplate,

			WireFrameMaterialTemplate,

			MaxMaterialTemplates
		};


		EditorResources(DevicePtr dptr);

		// can't default for inclusion reasons
		~EditorResources();

		DevicePtr device() const;

		const ComputeProgram& operator[](ComputePrograms i) const;
		const MaterialTemplate* operator[](MaterialTemplates i) const;

		void reload();

	private:
		void load_resources(DevicePtr dptr);

		std::unique_ptr<SpirVData[]> _spirv;
		std::unique_ptr<ComputeProgram[]> _computes;
		std::unique_ptr<MaterialTemplate[]> _material_templates;

};

}

#endif // EDITOR_CONTEXT_EDITORRESOURCES_H
