
#include "imgui_dock.inl"


// Settings load/save

namespace ImGui {

#define DOCK_CTX (*g_dock)

static DockContext::Dock* createDockWithIndex(int idx)
{
	if(idx < 0) return nullptr;
	DOCK_CTX.m_docks.reserve(idx + 1);
	while(DOCK_CTX.m_docks.size() <= idx)
	{
		DockContext::Dock* new_dock = (DockContext::Dock*)MemAlloc(sizeof(DockContext::Dock));
		IM_PLACEMENT_NEW(new_dock) DockContext::Dock();
		DOCK_CTX.m_docks.push_back(new_dock);
	}
	return DOCK_CTX.m_docks[idx];
}

void InitDock()
{
	if(GImGui)
	{
		ImGuiSettingsHandler settings;
		settings.TypeName = "Dock";
		settings.TypeHash = ImHash(settings.TypeName, 0);

		settings.WriteAllFn = [](ImGuiContext*, ImGuiSettingsHandler*, ImGuiTextBuffer* buf) {
			for (int i = 0; i < DOCK_CTX.m_docks.size(); ++i)
			{
				DockContext::Dock& dock = *DOCK_CTX.m_docks[i];

				buf->appendf("[Dock][%d]\n", i);
				buf->appendf("label=%.256s\n", dock.label);
				buf->appendf("size=%d,%d\n", (int)dock.size.x, (int)dock.size.y);
				buf->appendf("float=%d,%d\n", (int)dock.floatmode_size.x, (int)dock.floatmode_size.y);
				buf->appendf("pos=%d,%d\n", (int)dock.pos.x, (int)dock.pos.y);
				buf->appendf("location=%.256s\n", dock.location);
				buf->appendf("status=%d\n", (int)dock.status);
				buf->appendf("active=%d\n", (int)dock.active);
				buf->appendf("opened=%d\n", (int)dock.opened);
				buf->appendf("prev=%d\n", (int)DOCK_CTX.getDockIndex(dock.prev_tab));
				buf->appendf("next=%d\n", (int)DOCK_CTX.getDockIndex(dock.next_tab));
				buf->appendf("children=%d,%d\n", (int)DOCK_CTX.getDockIndex(dock.children[0]), (int)DOCK_CTX.getDockIndex(dock.children[1]));
				buf->appendf("parent=%d\n", (int)DOCK_CTX.getDockIndex(dock.parent));
				buf->appendf("\n");
			}
		};

		settings.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line) {
			DockContext::Dock* dock = (DockContext::Dock*)entry;
			int x, y;
			char buffer[256];

			if		(sscanf(line, "label=%[^\n]s\n", buffer))			{ dock->label = ImStrdup(buffer); dock->id = ImHash(dock->label, 0); }
			else if (sscanf(line, "size=%d,%d\n", &x, &y) == 2)			dock->size = ImVec2(x, y);
			else if (sscanf(line, "float=%d,%d\n", &x, &y) == 2)		dock->floatmode_size = ImVec2(x, y);
			else if (sscanf(line, "pos=%d,%d\n", &x, &y) == 2)			dock->pos = ImVec2(x, y);
			else if (sscanf(line, "location=%[^\n]s\n", buffer))		strcpy(dock->location, buffer);
			else if (sscanf(line, "status=%d\n", &x))					dock->status = (DockContext::Status_)x;
			else if (sscanf(line, "active=%d\n", &x))					dock->active = x;
			else if (sscanf(line, "opened=%d\n", &x))					dock->opened = x;
			else if (sscanf(line, "prev=%d\n", &x))						dock->prev_tab = createDockWithIndex(x);
			else if (sscanf(line, "next=%d\n", &x))						dock->next_tab = createDockWithIndex(x);
			else if (sscanf(line, "children=%d,%d\n", &x, &y) == 2)		{ dock->children[0] = createDockWithIndex(x); dock->children[1] = createDockWithIndex(y); }
			else if (sscanf(line, "parent=%d\n", &x))					dock->parent = createDockWithIndex(x);
		};

		settings.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler*, const char* id) {
			return (void*)createDockWithIndex(atoi(id));
		};

		GImGui->SettingsHandlers.push_back(settings);
	}
}

#undef DOCK_CTX

}


#ifdef  __GNUC__
#pragma GCC diagnostic pop
#endif

