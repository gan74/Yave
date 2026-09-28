/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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
#ifndef YAVE_BLUEPRINTS_BLUEPRINTNODEFACTORY_H
#define YAVE_BLUEPRINTS_BLUEPRINTNODEFACTORY_H

#include "BlueprintNode.h"

#include <memory>

namespace yave {

class BlueprintNodeFactory : NonMovable {
    public:
        virtual ~BlueprintNodeFactory();

        virtual std::string_view name() const = 0;
        virtual std::unique_ptr<BlueprintNode> create_node() const = 0;
};

template<typename T>
class GenericBlueprintNodeFactory : public BlueprintNodeFactory {
    public:
        GenericBlueprintNodeFactory(core::String name) : _name(std::move(name)) {
        }

        std::string_view name() const override {
            return _name;
        }

        std::unique_ptr<BlueprintNode> create_node() const override {
            return std::make_unique<T>(_name);
        }

    private:
        core::String _name;
};

}

#endif // YAVE_BLUEPRINTS_BLUEPRINTNODEFACTORY_H
