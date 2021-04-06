/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <external/imgui/yave_imgui.h>


namespace editor {

ImageImporter::ImageImporter() : ImageImporter(FileSystemModel::local_filesystem()->current_path().unwrap_or("")) {
}

ImageImporter::ImageImporter(const core::String& import_path) :
        Widget("Image importer"),
        _import_path(import_path) {

    _browser.set_selection_filter(false, import::supported_image_extensions());
    _browser.set_selected_callback([this](const auto& filename) { import_async(filename); return true; });
    _browser.set_canceled_callback([this] { close(); return true; });
}

void ImageImporter::on_gui()  {
    if(is_loading()) {
        if(done_loading()) {
            try {
                const auto& imported = _import_future.get();
                import(imported);
            } catch(std::exception& e) {
                log_msg(fmt("Unable to import scene: %" , e.what()), Log::Error);
            }
            close();
        } else {
            ImGui::Text("Loading...");
        }
    } else {
        _browser.draw_gui_inside();
    }
}

bool ImageImporter::is_loading() const {
    return _import_future.valid();
}

bool ImageImporter::done_loading() const {
    return _import_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

void ImageImporter::import_async(const core::String& filename) {
    _import_future = std::async(std::launch::async, [=] {
        return import::import_image(filename, import::ImageImportFlags::GenerateMipmaps);
    });
}

void ImageImporter::import(const Named<ImageData>& asset) {
    const core::String name = asset_store().filesystem()->join(_import_path, asset.name());
    io2::Buffer buffer;
    serde3::WritableArchive arc(buffer);
    if(!arc.serialize(asset.obj())) {
        log_msg(fmt("Unable serialize image"), Log::Error);
        return;
    }
    buffer.reset();
    if(!asset_store().import(buffer, name, AssetType::Image)) {
        log_msg(fmt("Unable import image"), Log::Error);
        // refresh anyway
    }

    refresh_all();
}

}

