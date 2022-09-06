#include <cstdio>

#include <yave/script/VM.h>

#include <y/utils.h>
#include <y/utils/format.h>
#include <y/utils/log.h>


using namespace y;
using namespace yave::script;

struct Floop {

};

struct MetaTest {
    int foo = 4;
    int blap = 904;
    Floop fl;
    core::String name = "test obj";
    bool deleted = false;

    ~MetaTest() {
        deleted = true;
    }

    y_reflect(MetaTest, foo, blap, fl, name, deleted);
};

core::String floop(int z, float x) {
    return fmt("a returned string (%, %)", z, x);
}

int main(int, char**) {
    const i64 iterations = is_debug_defined ? 100000 : 1000000;

    VM vm = VM::create();

    vm.bind_type<MetaTest>();

    vm.set_global("globo", lua::bind_function<floop>);
    vm.set_global("max", iterations);

    vm.set_global("clear_external", [](lua_State* l) -> int {
        lua::clean_external_objects(l);
        return 0;
    });

    MetaTest meta_test;
    vm.set_global_object("external", &meta_test);

    const char* code = R"#(
        local sum = 0;
        for i = 1, max do
            local obj = MetaTest.new(1)
            -- print(obj)
            local copy = obj
            -- print(copy)
            assert(obj == copy)
            assert(obj ~= external)
            assert(obj.blap == 904)
            assert(obj.name == "test obj")
            obj.name = "newname"
            assert(obj.name == "newname")
            assert(obj.doesntexist == nil)
            sum = sum + obj.foo
            assert(obj.deleted == false)
            assert(external.deleted == false)
            assert(external.foo == 4)
        end
        print(sum)
        external.blap = sum;
        assert(not external:is_stale())
print(external)
        clear_external()
print(external)
        assert(MetaTest.is_stale(external))
        assert(external.blap == nil)
        print(globo(999, 3.24))
    )#";

    if(auto r = vm.run(code); r.is_error()) {
        log_msg(r.error().msg, Log::Error);
    }

    y_debug_assert(meta_test.deleted == false);
    y_debug_assert(meta_test.blap == iterations * 4);

    return 0;
}
