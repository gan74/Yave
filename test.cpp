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

core::String floop(int z, float x) {
    return fmt("a returned string (%, %)", z, x);
}

int main(int, char**) {
    VM vm = VM::create();

    vm.bind_type<MetaTest>();

    vm.set_global("globo", lua::bind_function<floop>);

    const char* code = R"#(
        local obj = MetaTest.new(1)

        print(obj)
        print(obj.blap)
        print(obj.name)
        obj.name = "newname"
        print(obj.name)
        print(obj.doesntexist)

        print(globo(999, 3.24))
    )#";

    if(auto r = vm.run(code); r.is_error()) {
        log_msg(r.error().msg, Log::Error);
    }


    return 0;
}
