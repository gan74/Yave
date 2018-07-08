/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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
#ifndef YAVE_UTILS_FILESYSTEMMODEL_H
#define YAVE_UTILS_FILESYSTEMMODEL_H

#include <yave/yave.h>
#include <y/core/Functor.h>

namespace yave {

class FileSystemModelBase : NonCopyable {
	public:
		using for_each_f = core::Function<void(std::string_view)>;

		virtual ~FileSystemModelBase() {
		}


		virtual core::String current_path() const = 0;
		virtual core::String parent_path(std::string_view path) const = 0;
		virtual core::String filename(std::string_view path) const = 0;

		virtual bool exists(std::string_view path) const = 0;
		virtual bool is_directory(std::string_view path) const = 0;

		virtual core::String join(std::string_view path, std::string_view name) const = 0;

		virtual void for_each(std::string_view path, const for_each_f& func) const = 0;
};


#ifndef YAVE_NO_STDFS

class FileSystemModel : public FileSystemModelBase {
	public:
		core::String current_path() const override;
		core::String parent_path(std::string_view path) const override;
		core::String filename(std::string_view path) const override;

		bool exists(std::string_view path) const override;
		bool is_directory(std::string_view path) const override;

		core::String join(std::string_view path, std::string_view name) const override;

		void for_each(std::string_view path, const for_each_f& func) const override;
};

#endif

}


#endif // YAVE_UTILS_FILESYSTEMMODEL_H
