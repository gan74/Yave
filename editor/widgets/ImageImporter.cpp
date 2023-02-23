/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include "ImageImporter.h"

#include <editor/import/import.h>

#include <yave/assets/AssetStore.h>
#include <yave/utils/FileSystemModel.h>

#include <y/io2/Buffer.h>
#include <y/serde3/archives.h>

#include <editor/utils/ui.h>
#include <y/utils/format.h>
#include <y/utils/log.h>



namespace editor {

ImageImporter::ImageImporter() : ImageImporter(asset_store().filesystem()->current_path().unwrap_or(".")) {
}

ImageImporter::ImageImporter(std::string_view import_path) :
        Widget("Image importer"),
        _import_path(import_path) {

    _browser.set_selection_filter(import::supported_image_extensions());
    _browser.set_selected_callback([this](const auto& filename) { import(filename); return true; });
    _browser.set_canceled_callback([this] { close(); return true; });
}

ImageImporter::~ImageImporter() {
}

void ImageImporter::on_gui()  {
    switch(_state) {
        case State::Browsing:
            _browser.draw_gui_inside();
        break;

        case State::Importing:
            ImGui::Text("Loading%s", imgui::ellipsis());
            if(_thread_pool.is_empty()) {
                _state = State::Done;
            }
        break;

        case State::Done:
            close();
            refresh_all();
        break;
    }
}

bool ImageImporter::should_keep_alive() const {
     return !_thread_pool.is_empty();
}

void ImageImporter::import(const core::String& filename) {
    y_debug_assert(_state == State::Browsing);
    _state = State::Importing;

    _thread_pool.schedule([=] {
        auto result = import::import_image(filename, import::ImageImportFlags::GenerateMipmaps | import::ImageImportFlags::Compress);
        if(result.is_error()) {
            log_msg("Unable to import image", Log::Error);
            return;
        }


        io2::Buffer buffer;
        serde3::WritableArchive arc(buffer);
        if(!arc.serialize(result.unwrap())) {
            log_msg("Unable serialize image", Log::Error);
            return;
        }
        buffer.reset();

        const core::String full_name = asset_store().filesystem()->join(_import_path, import::clean_asset_name(filename));
        if(!asset_store().import(buffer, full_name, AssetType::Image)) {
            log_msg("Unable import image", Log::Error);
        }
    });
}



}

