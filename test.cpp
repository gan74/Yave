#include <cstdio>

#include <yave/script/VM.h>

#include <y/utils.h>
#include <y/utils/format.h>
#include <y/utils/log.h>



using namespace y;
using namespace yave::script;


struct MetaTest {
    int foo = 4;
    int blap = 904;
    core::String name = "test obj";

    y_reflect(MetaTest, foo, blap, name);
};


int main(int, char**) {
    VM vm = VM::create();

    vm.bind_type<MetaTest>();
    lua_register(vm.state(), "new", lua::create_object<MetaTest>);


    const char* code = R"#(
        local obj = new()

        print(obj.blap)
        obj.blap = 123;
        print(obj.blap)

        print(obj.name)
        obj.name = 'new name'
        print(obj.name)
    )#";

    if(auto r = vm.run(code); r.is_error()) {
        log_msg(r.error().msg, Log::Error);
    }

    return 0;
}
