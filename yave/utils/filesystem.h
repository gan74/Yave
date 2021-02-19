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
#ifndef YAVE_UTILS_FILESYSTEM_H
#define YAVE_UTILS_FILESYSTEM_H

#include <y/defines.h>

#ifndef YAVE_NO_STDFS
#if __has_include(<filesystem>)
#define YAVE_STDFS_NAMESPACE std::filesystem
#include <filesystem>
#else
#define YAVE_STDFS_NAMESPACE std::experimental::filesystem
#include <experimental/filesystem>
#endif
#endif // YAVE_NO_STDFS

// fs::copy and fs::copy_file will mess up binary files by replacing '\n' by "\r\n" on windows.
#ifdef Y_OS_WIN
#define YAVE_STDFS_BAD_COPY
#endif

namespace yave {

namespace fs {
#ifndef YAVE_NO_STDFS
using namespace YAVE_STDFS_NAMESPACE;
#endif
}


}

#endif // YAVE_UTILS_FILESYSTEM_H

