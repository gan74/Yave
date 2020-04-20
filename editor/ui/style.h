// This is mostly all stolen from ImGui "Post your screenshots" issues



if(_style == Style::Yave) {
	// Custom style, not complete yet

	ImGuiStyle& style = ImGui::GetStyle();
	style.ChildBorderSize = 0;
	style.PopupBorderSize = 0;
	style.FrameBorderSize = 0;
	style.TabBorderSize	= 0;

	style.PopupRounding = 0;
	style.WindowRounding = 0;
	style.ChildRounding = 0;
	style.FrameRounding = 3;
	style.TabRounding = 0;

	style.WindowMinSize = ImVec2(100, 100);

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg]               = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
	colors[ImGuiCol_PopupBg]                = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_Border]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg]                = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
	colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
	colors[ImGuiCol_FrameBgActive]          = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
	colors[ImGuiCol_TitleBg]                = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_TitleBgActive]          = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_MenuBarBg]              = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.10f);
	colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_CheckMark]              = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_SliderGrab]             = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
	colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_Button]                 = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
	colors[ImGuiCol_ButtonHovered]          = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_ButtonActive]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_Header]                 = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_HeaderHovered]          = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	colors[ImGuiCol_HeaderActive]           = ImVec4(0.46f, 0.46f, 0.46f, 1.00f);
	colors[ImGuiCol_Separator]              = ImVec4(0.00f, 0.00f, 0.00f, 0.10f);
	colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
	colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
	colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.84f);
	colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_Tab]                    = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
	colors[ImGuiCol_TabHovered]             = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_TabUnfocused]           = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_DockingPreview]         = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
	colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
	colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

if(_style == Style::Yave2) {

	auto& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	const auto rgb = [&](u8 r, u8 g, u8 b, u8 a = 255) { return math::Vec4(r, g, b, a) / 255.0f; };
	const auto with_alpha = [&](const math::Vec4& rgba, u8 a = 255) { return math::Vec4(rgba.to<3>(), a / 255.0f); };

	for(usize i = 0; i != ImGuiCol_COUNT; ++i) {
		colors[i] = rgb(255, 0, 0);
	}

	const auto transparent					= rgb(  0,   0,   0,   0);
	const auto highlight					= rgb( 67, 149, 251);
	const auto highlight2					= rgb(255,   3,  85);
	const auto highlight3					= rgb( 53, 194,  66);
	const auto button						= rgb( 35,  35,  43);
	const auto background					= rgb( 21,  22,  24);
	const auto border						= rgb(  1,   1,   6);
	const auto tab							= rgb( 11,  11,  11);

	unused(highlight2, highlight3, tab);

	colors[ImGuiCol_Text]					= rgb(255, 255, 255);
	colors[ImGuiCol_TextDisabled]			= rgb(116, 116, 116);

	colors[ImGuiCol_MenuBarBg]              = background;
	colors[ImGuiCol_ChildBg]                = background;
	colors[ImGuiCol_PopupBg]                = background;
	colors[ImGuiCol_FrameBg]				= rgb( 25,  25,  31);
	colors[ImGuiCol_WindowBg]               = rgb( 31,  32,  37);

	colors[ImGuiCol_FrameBgHovered]         = colors[ImGuiCol_FrameBg];
	colors[ImGuiCol_FrameBgActive]          = colors[ImGuiCol_FrameBg];

	colors[ImGuiCol_TitleBg]                = colors[ImGuiCol_WindowBg];
	colors[ImGuiCol_TitleBgActive]          = colors[ImGuiCol_WindowBg];
	colors[ImGuiCol_TitleBgCollapsed]       = colors[ImGuiCol_WindowBg];

	colors[ImGuiCol_Button]                 = button;
	colors[ImGuiCol_ButtonHovered]          = highlight;
	colors[ImGuiCol_ButtonActive]           = highlight;
	colors[ImGuiCol_Header]                 = button;
	colors[ImGuiCol_HeaderHovered]          = highlight;
	colors[ImGuiCol_HeaderActive]           = highlight;
	colors[ImGuiCol_ResizeGrip]             = button;
	colors[ImGuiCol_ResizeGripHovered]      = highlight;
	colors[ImGuiCol_ResizeGripActive]       = highlight;

	colors[ImGuiCol_ScrollbarBg]            = rgb( 16,  16,  19);
	colors[ImGuiCol_ScrollbarGrab]          = button;
	colors[ImGuiCol_ScrollbarGrabHovered]   = highlight;
	colors[ImGuiCol_ScrollbarGrabActive]    = highlight;
	colors[ImGuiCol_CheckMark]              = highlight;
	colors[ImGuiCol_SliderGrab]             = button;
	colors[ImGuiCol_SliderGrabActive]       = highlight;

	colors[ImGuiCol_Separator]				= colors[ImGuiCol_WindowBg];
	colors[ImGuiCol_SeparatorHovered]       = highlight;
	colors[ImGuiCol_SeparatorActive]        = highlight;

	colors[ImGuiCol_Border]                 = border;
	colors[ImGuiCol_BorderShadow]           = transparent;

	colors[ImGuiCol_Tab]					= colors[ImGuiCol_WindowBg];
	colors[ImGuiCol_TabActive]              = background;
	colors[ImGuiCol_TabHovered]             = background;
	colors[ImGuiCol_TabUnfocused]           = with_alpha(background, 64);
	colors[ImGuiCol_TabUnfocusedActive]     = background;



	style.PopupRounding = 0;

	style.WindowPadding = ImVec2(4, 4);
	style.FramePadding  = ImVec2(6, 4);
	style.ItemSpacing   = ImVec2(6, 2);

	style.ScrollbarSize = 18;

	style.IndentSpacing = 12;

	style.WindowBorderSize = 1;
	style.ChildBorderSize  = 1;
	style.PopupBorderSize  = 1;
	style.FrameBorderSize  = 1;

	style.WindowRounding    = 0;
	style.ChildRounding     = 2;
	style.FrameRounding     = 2;
	style.ScrollbarRounding = 2;
	style.GrabRounding      = 2;

	style.TabBorderSize     = 1;
	style.TabRounding       = 0;

	if(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	{
		usize unset = 0;
		for(usize i = 0; i != ImGuiCol_COUNT; ++i) {
			unset += math::Vec4(colors[i]) == rgb(255, 0, 0);
		}

		if(unset) {
			log_msg(fmt("% colors have not been specified.", unset), Log::Warning);
		}
	}
}


// https://github.com/ocornut/imgui/issues/707#issuecomment-468798935
if(_style == Style::Corporate || _style == Style::Corporate3D) {
	const float darkening = 1.0f;
	const auto dImVec4 = [=](float r, float g, float b, float a) { return ImVec4(r * darkening, g * darkening, b * darkening, a); };

	auto& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	/// 0 = FLAT APPEARENCE
	/// 1 = MORE "3D" LOOK
	const int is_3D = _style == Style::Corporate3D ? 1 : 0;

	colors[ImGuiCol_Text]                   = dImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]           = dImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	//colors[ImGuiCol_ChildBg]                = dImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_ChildBg]                = dImVec4(0.00f, 0.00f, 0.00f, 0.10f);
	colors[ImGuiCol_WindowBg]               = dImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_PopupBg]                = dImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_Border]                 = dImVec4(0.12f, 0.12f, 0.12f, 0.71f);
	colors[ImGuiCol_BorderShadow]           = dImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_FrameBg]                = dImVec4(0.42f, 0.42f, 0.42f, 0.54f);
	colors[ImGuiCol_FrameBgHovered]         = dImVec4(0.42f, 0.42f, 0.42f, 0.40f);
	colors[ImGuiCol_FrameBgActive]          = dImVec4(0.56f, 0.56f, 0.56f, 0.67f);
	colors[ImGuiCol_TitleBg]                = dImVec4(0.19f, 0.19f, 0.19f, 1.00f);
	colors[ImGuiCol_TitleBgActive]          = dImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]       = dImVec4(0.17f, 0.17f, 0.17f, 0.90f);
	colors[ImGuiCol_MenuBarBg]              = dImVec4(0.335f, 0.335f, 0.335f, 1.000f);
	colors[ImGuiCol_ScrollbarBg]            = dImVec4(0.24f, 0.24f, 0.24f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab]          = dImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered]   = dImVec4(0.52f, 0.52f, 0.52f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive]    = dImVec4(0.76f, 0.76f, 0.76f, 1.00f);
	colors[ImGuiCol_CheckMark]              = dImVec4(0.65f, 0.65f, 0.65f, 1.00f);
	colors[ImGuiCol_SliderGrab]             = dImVec4(0.52f, 0.52f, 0.52f, 1.00f);
	colors[ImGuiCol_SliderGrabActive]       = dImVec4(0.64f, 0.64f, 0.64f, 1.00f);
	colors[ImGuiCol_Button]                 = dImVec4(0.54f, 0.54f, 0.54f, 0.35f);
	colors[ImGuiCol_ButtonHovered]          = dImVec4(0.52f, 0.52f, 0.52f, 0.59f);
	colors[ImGuiCol_ButtonActive]           = dImVec4(0.76f, 0.76f, 0.76f, 1.00f);
	colors[ImGuiCol_Header]                 = dImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	colors[ImGuiCol_HeaderHovered]          = dImVec4(0.47f, 0.47f, 0.47f, 1.00f);
	colors[ImGuiCol_HeaderActive]           = dImVec4(0.76f, 0.76f, 0.76f, 0.77f);
	colors[ImGuiCol_Separator]              = dImVec4(0.000f, 0.000f, 0.000f, 0.137f);
	colors[ImGuiCol_SeparatorHovered]       = dImVec4(0.700f, 0.671f, 0.600f, 0.290f);
	colors[ImGuiCol_SeparatorActive]        = dImVec4(0.702f, 0.671f, 0.600f, 0.674f);
	colors[ImGuiCol_ResizeGrip]             = dImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered]      = dImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive]       = dImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_PlotLines]              = dImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered]       = dImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram]          = dImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered]   = dImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg]         = dImVec4(0.73f, 0.73f, 0.73f, 0.35f);
	colors[ImGuiCol_ModalWindowDimBg]       = dImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	colors[ImGuiCol_DragDropTarget]         = dImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight]           = dImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight]  = dImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]      = dImVec4(0.80f, 0.80f, 0.80f, 0.20f);

	style.PopupRounding = 3;

	style.WindowPadding = ImVec2(4, 4);
	style.FramePadding  = ImVec2(6, 4);
	style.ItemSpacing   = ImVec2(6, 2);

	style.ScrollbarSize = 18;

	style.WindowBorderSize = 1;
	style.ChildBorderSize  = 1;
	style.PopupBorderSize  = 1;
	style.FrameBorderSize  = is_3D;

	style.WindowRounding    = 3;
	style.ChildRounding     = 3;
	style.FrameRounding     = 3;
	style.ScrollbarRounding = 2;
	style.GrabRounding      = 3;

	style.TabBorderSize = is_3D;
	style.TabRounding   = 3;

	colors[ImGuiCol_DockingEmptyBg]     = dImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	colors[ImGuiCol_Tab]                = dImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_TabHovered]         = dImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_TabActive]          = dImVec4(0.33f, 0.33f, 0.33f, 1.00f);
	colors[ImGuiCol_TabUnfocused]       = dImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = dImVec4(0.33f, 0.33f, 0.33f, 1.00f);
	colors[ImGuiCol_DockingPreview]     = dImVec4(0.85f, 0.85f, 0.85f, 0.28f);

	if(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	style.IndentSpacing		= 12;


#if 0
	colors[ImGuiCol_Text]                   =  ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]           =  ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	//colors[ImGuiCol_ChildBg]                = dImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_ChildBg]                =  ImVec4(0.00f, 0.00f, 0.00f, 0.15f);
	colors[ImGuiCol_WindowBg]               = dImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_PopupBg]                = dImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_Border]                 = dImVec4(0.12f, 0.12f, 0.12f, 0.71f);
	colors[ImGuiCol_BorderShadow]           = dImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_FrameBg]                = dImVec4(0.42f, 0.42f, 0.42f, 0.54f);
	colors[ImGuiCol_FrameBgHovered]         = dImVec4(0.42f, 0.42f, 0.42f, 0.40f);
	colors[ImGuiCol_FrameBgActive]          = dImVec4(0.56f, 0.56f, 0.56f, 0.67f);
	colors[ImGuiCol_TitleBg]                = dImVec4(0.19f, 0.19f, 0.19f, 1.00f);
	colors[ImGuiCol_TitleBgActive]          = dImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]       = dImVec4(0.17f, 0.17f, 0.17f, 0.90f);
	//colors[ImGuiCol_MenuBarBg]              = dImVec4(0.335f, 0.335f, 0.335f, 1.000f);
	colors[ImGuiCol_MenuBarBg]              = dImVec4(0.00f, 0.00f, 0.00f, 0.25f);
	colors[ImGuiCol_ScrollbarBg]            = dImVec4(0.24f, 0.24f, 0.24f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab]          = dImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered]   = dImVec4(0.52f, 0.52f, 0.52f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive]    = dImVec4(0.76f, 0.76f, 0.76f, 1.00f);
	colors[ImGuiCol_CheckMark]              = dImVec4(0.65f, 0.65f, 0.65f, 1.00f);
	colors[ImGuiCol_SliderGrab]             = dImVec4(0.52f, 0.52f, 0.52f, 1.00f);
	colors[ImGuiCol_SliderGrabActive]       = dImVec4(0.64f, 0.64f, 0.64f, 1.00f);
	colors[ImGuiCol_Button]                 = dImVec4(0.54f, 0.54f, 0.54f, 0.35f);
	colors[ImGuiCol_ButtonHovered]          = dImVec4(0.52f, 0.52f, 0.52f, 0.59f);
	colors[ImGuiCol_ButtonActive]           = dImVec4(0.76f, 0.76f, 0.76f, 1.00f);
	colors[ImGuiCol_Header]                 = dImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	colors[ImGuiCol_HeaderHovered]          = dImVec4(0.47f, 0.47f, 0.47f, 1.00f);
	colors[ImGuiCol_HeaderActive]           = dImVec4(0.76f, 0.76f, 0.76f, 0.77f);
	colors[ImGuiCol_Separator]              = dImVec4(0.000f, 0.000f, 0.000f, 0.137f);
	colors[ImGuiCol_SeparatorHovered]       = dImVec4(0.700f, 0.671f, 0.600f, 0.290f);
	colors[ImGuiCol_SeparatorActive]        = dImVec4(0.702f, 0.671f, 0.600f, 0.674f);
	colors[ImGuiCol_ResizeGrip]             = dImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered]      = dImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive]       = dImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_PlotLines]              = dImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered]       = dImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram]          = dImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered]   = dImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg]         = dImVec4(0.73f, 0.73f, 0.73f, 0.35f);
	colors[ImGuiCol_ModalWindowDimBg]       = dImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	//colors[ImGuiCol_DragDropTarget]         = dImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_DragDropTarget]         = dImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_NavHighlight]           = dImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight]  = dImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]      = dImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_DockingEmptyBg]         = dImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	colors[ImGuiCol_Tab]                    = dImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_TabHovered]             = dImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_TabActive]              = dImVec4(0.33f, 0.33f, 0.33f, 1.00f);
	colors[ImGuiCol_TabUnfocused]           = dImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive]     = dImVec4(0.33f, 0.33f, 0.33f, 1.00f);
	colors[ImGuiCol_DockingPreview]         = dImVec4(0.85f, 0.85f, 0.85f, 0.28f);

	style.PopupRounding = 0;

	style.WindowPadding = ImVec2(4, 4);
	style.FramePadding  = ImVec2(6, 4);
	style.ItemSpacing   = ImVec2(6, 2);

	style.ScrollbarSize = 18;

	style.IndentSpacing = 12;

	style.WindowBorderSize = 1;
	style.ChildBorderSize  = 1;
	style.PopupBorderSize  = 1;
	style.FrameBorderSize  = is_3D;

	style.WindowRounding    = 0;
	style.ChildRounding     = 2;
	style.FrameRounding     = 0;
	style.ScrollbarRounding = 2;
	style.GrabRounding      = 2;

	style.TabBorderSize     = is_3D;
	style.TabRounding       = 0;

	if(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
#endif
}



if(_style == Style::Blender) {
	auto& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	for(usize i = ImGuiCol_Text + 2; i != ImGuiCol_COUNT; ++i) {
		colors[i] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
	}

	auto rgba = [](u8 r, u8 g, u8 b, u8 a = 255) { return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f); };

	colors[ImGuiCol_Text]					= rgba(255, 255, 255);
	colors[ImGuiCol_TextDisabled]			= rgba(128, 128, 128);

	colors[ImGuiCol_WindowBg]				= rgba( 75,  75,  75);
	colors[ImGuiCol_ChildBg]				= rgba( 50,  50,  50, 100);
	colors[ImGuiCol_MenuBarBg]				= rgba( 35,  35,  35);
	colors[ImGuiCol_ScrollbarBg]			= rgba( 35,  35,  35);
	colors[ImGuiCol_PopupBg]				= rgba( 35,  35,  35, 220);

	colors[ImGuiCol_Border]					= rgba(  0,   0,   0,  40);
	colors[ImGuiCol_BorderShadow]			= rgba(  0,   0,   0,   0);

	colors[ImGuiCol_FrameBg]				= rgba( 45,  45,  45);
	colors[ImGuiCol_FrameBgActive]			= rgba( 78,  78,  78);
	colors[ImGuiCol_FrameBgHovered]			= rgba( 78,  78,  78);

	colors[ImGuiCol_TitleBg]				= rgba( 35,  35,  35);
	colors[ImGuiCol_TitleBgActive]			= rgba( 35,  35,  35);
	colors[ImGuiCol_TitleBgCollapsed]		= rgba( 35,  35,  35);

	colors[ImGuiCol_Tab]					= rgba( 45,  45,  45);
	colors[ImGuiCol_TabUnfocused]			= rgba( 45,  45,  45);
	colors[ImGuiCol_TabHovered]				= rgba( 58,  58,  58);
	colors[ImGuiCol_TabActive]				= rgba( 65,  65,  65);
	colors[ImGuiCol_TabUnfocusedActive]		= rgba( 65,  65,  65);

	/*colors[ImGuiCol_Header]					= rgba(  0,   0,   0,   0);
	colors[ImGuiCol_HeaderHovered]			= rgba(  0,   0,   0,   0);
	colors[ImGuiCol_HeaderActive]			= rgba(  0,   0,   0,   0);*/

	colors[ImGuiCol_Button]					= rgba( 90,  90,  90);
	colors[ImGuiCol_ButtonHovered]			= rgba(105, 105, 105);
	colors[ImGuiCol_ButtonActive]			= rgba( 85, 120, 180);

	colors[ImGuiCol_Separator]				= colors[ImGuiCol_Button];
	colors[ImGuiCol_SeparatorHovered]		= colors[ImGuiCol_ButtonHovered];
	colors[ImGuiCol_SeparatorActive]		= colors[ImGuiCol_ButtonActive];

	colors[ImGuiCol_ResizeGrip]				= colors[ImGuiCol_Button];
	colors[ImGuiCol_ResizeGripHovered]		= colors[ImGuiCol_ButtonHovered];
	colors[ImGuiCol_ResizeGripActive]		= colors[ImGuiCol_ButtonActive];

	colors[ImGuiCol_ScrollbarGrab]			= colors[ImGuiCol_Button];
	colors[ImGuiCol_ScrollbarGrabHovered]	= colors[ImGuiCol_ButtonHovered];
	colors[ImGuiCol_ScrollbarGrabActive]	= colors[ImGuiCol_ButtonActive];

	colors[ImGuiCol_Header]					= colors[ImGuiCol_Button];
	colors[ImGuiCol_HeaderHovered]			= colors[ImGuiCol_ButtonHovered];
	colors[ImGuiCol_HeaderActive]			= colors[ImGuiCol_ButtonActive];

	colors[ImGuiCol_SliderGrab]				= colors[ImGuiCol_Button];
	colors[ImGuiCol_SliderGrabActive]		= colors[ImGuiCol_ButtonActive];

	colors[ImGuiCol_PlotLines]				= colors[ImGuiCol_ButtonActive];
	colors[ImGuiCol_PlotLinesHovered]		= colors[ImGuiCol_ButtonActive];
	colors[ImGuiCol_PlotHistogram]			= colors[ImGuiCol_ButtonActive];
	colors[ImGuiCol_PlotHistogramHovered]	= colors[ImGuiCol_ButtonActive];


	colors[ImGuiCol_DockingEmptyBg]			= colors[ImGuiCol_WindowBg];
	colors[ImGuiCol_DockingPreview]			= colors[ImGuiCol_ButtonActive];
	colors[ImGuiCol_CheckMark]				= colors[ImGuiCol_ButtonActive];
	colors[ImGuiCol_TextSelectedBg]			= colors[ImGuiCol_ButtonActive];
	colors[ImGuiCol_DragDropTarget]			= colors[ImGuiCol_ButtonActive];

	colors[ImGuiCol_NavHighlight]			= rgba(  0,  0,  0,  128);
	colors[ImGuiCol_NavWindowingHighlight]	= rgba(  0,  0,  0,  128);
	colors[ImGuiCol_NavWindowingDimBg]		= rgba(  0,  0,  0,  128);
	colors[ImGuiCol_ModalWindowDimBg]		= rgba(  0,  0,  0,  128);



	style.FrameBorderSize	= 1;
	style.PopupBorderSize	= 1;

	style.WindowRounding    = 3;
	style.ChildRounding     = 3;
	style.PopupRounding     = 3;
	style.FrameRounding     = 3;
	style.GrabRounding		= 3;
	style.TabRounding		= 3;

	style.ScrollbarRounding = 8;

}
