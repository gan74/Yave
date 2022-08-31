#include <cstdio>

#include "external/LuaJIT/src/lua.hpp"

int main(int argc, char** argv) {
	lua_State* l = lua_open();
	printf("Hello LuaJit !\n");
}