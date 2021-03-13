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

#include "Settings.h"

#include <y/serde3/archives.h>
#include <y/io2/File.h>
#include <y/utils/log.h>

namespace editor {

static constexpr std::string_view settings_file = "../settings.dat";

Settings::Settings(bool load) {
    if(load) {
        auto file = io2::File::open(settings_file);
        if(!file) {
            log_msg("Unable to open settings file.", Log::Error);
            return;
        }
        serde3::ReadableArchive arc(file.unwrap());
        if(!arc.deserialize(*this)) {
            log_msg("Unable to read settings file.", Log::Error);
            *this = Settings(false);
        }
    }
}

Settings::~Settings() {
    auto file = io2::File::create(settings_file);
    if(!file) {
        log_msg("Unable to open settings file.", Log::Error);
        return;
    }
    serde3::WritableArchive arc(file.unwrap());
    if(!arc.serialize(*this)) {
        log_msg("Unable to write settings file.", Log::Error);
    }
}

}

