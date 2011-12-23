/*
 * RocketVM
 * Copyright (c) 2011 Max McGuire
 *
 * See copyright notice in lua.h
 */
#include "Test.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static bool DoString(lua_State* L, const char* string)
{
    if (luaL_dostring(L, string) != 0)
    {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        return false;
    }
    return true;
}

static size_t GetTotalBytes(lua_State* L)
{
    return lua_gc(L, LUA_GCCOUNT, 0) * 1024 + lua_gc(L, LUA_GCCOUNTB, 0);
}

// Currently disabled while the GC is being developed.
/*
TEST(GcTest)
{
    
    lua_State* L = luaL_newstate();

    lua_pushstring(L, "garbage string");
    lua_pop(L, 1);

    lua_gc(L, LUA_GCCOLLECT, 0);
    size_t bytes1 = GetTotalBytes(L);
    CHECK(bytes1 > 0);

    // Create a piece of garbage.
    int table = lua_gettop(L);
    lua_newtable(L);
    lua_pop(L, 1);

    // Check that the garbage is cleaned up.
    lua_gc(L, LUA_GCCOLLECT, 0);
    size_t bytes2 = GetTotalBytes(L);
    CHECK( bytes2 <= bytes1 );

    lua_close(L);

}
*/

TEST(ConcatTest)
{

    lua_State* L = luaL_newstate();

    int top = lua_gettop(L);

    lua_pushstring(L, "Hello ");
    lua_pushnumber(L, 5.0);
    lua_pushstring(L, " goodbye");
    lua_concat(L, 3);

    const char* result = lua_tostring(L, -1);
    CHECK( strcmp(result, "Hello 5 goodbye") == 0 );
    CHECK( lua_gettop(L) - top == 1 );

    lua_close(L);

}

TEST(InsertTest)
{

    lua_State* L = luaL_newstate();

    int top = lua_gettop(L);

    lua_pushinteger(L, 1);
    lua_pushinteger(L, 3);
    lua_pushinteger(L, 2);
    lua_insert(L, -2);

    CHECK( lua_tointeger(L, -3) == 1 );
    CHECK( lua_tointeger(L, -2) == 2 );
    CHECK( lua_tointeger(L, -1) == 3 );
    
    CHECK( lua_gettop(L) - top == 3 );

    lua_close(L);

}

TEST(PCallTest)
{

    struct Locals
    {
        static int ErrorFunction(lua_State* L)
        {
            lua_pushstring(L, "Error message");
            lua_error(L);
            return 0;
        }
    };

    lua_State* L = luaL_newstate();

    lua_pushcfunction(L, Locals::ErrorFunction);
    CHECK( lua_pcall(L, 0, 0, 0) == LUA_ERRRUN );
    CHECK( strcmp( lua_tostring(L, -1), "Error message") == 0 );

    lua_close(L);

}

TEST(RawGetITest)
{

    lua_State* L = luaL_newstate();

    lua_newtable(L);
    int table = lua_gettop(L);

    lua_pushstring(L, "extra");
    lua_pushstring(L, "extra");
    lua_settable(L, table);

    lua_pushinteger(L, 1);
    lua_pushstring(L, "one");
    lua_settable(L, table);

    lua_rawgeti(L, table, 1);
    CHECK( strcmp( lua_tostring(L, -1), "one") == 0 );

    lua_close(L);

}

TEST(NextTest)
{

    lua_State* L = luaL_newstate();

    lua_newtable(L);
    int table = lua_gettop(L);

    lua_pushnumber(L, 1);
    lua_setfield(L, table, "first");

    lua_pushnumber(L, 2);
    lua_setfield(L, table, "second");

    lua_pushnumber(L, 3);
    lua_setfield(L, table, "third");

    lua_pushnil(L);

    int count[3] = { 0 };

    while (lua_next(L, table))
    {

        const char* key  = lua_tostring(L, -2);
        lua_Number value = lua_tonumber(L, -1);

        int index = -1;
        if (strcmp(key, "first") == 0)
        {
            CHECK(value == 1.0);
            index = 0;
        }
        else if (strcmp(key, "second") == 0)
        {
            CHECK(value == 2.0);
            index = 1;
        }
        else if (strcmp(key, "third") == 0)
        {
            CHECK(value == 3.0);
            index = 2;
        }

        // Check that we didn't get a key not in the table.
        CHECK(index != -1);
        ++count[index];

        lua_pop(L, 1);

    }

    // Check each element was iterated exactly once.
    CHECK(count[0] == 1);
    CHECK(count[1] == 1);
    CHECK(count[2] == 1);

    lua_close(L);

}

TEST(RemoveTest)
{

    lua_State* L = luaL_newstate();

    lua_pushinteger(L, 1);
    int start = lua_gettop(L);
    lua_pushinteger(L, 2);
    lua_pushinteger(L, 3);
    lua_pushinteger(L, 4);

    lua_remove(L, start);
    CHECK( lua_tointeger(L, start) == 2 );

    lua_remove(L, -1);
    CHECK( lua_tointeger(L, -1) == 3 );

    lua_close(L);

}

TEST(MetatableTest)
{

    lua_State* L = luaL_newstate();

    lua_newtable(L);
    int table = lua_gettop(L);

    lua_pushinteger(L, 2);
    lua_setfield(L, table, "b");

    lua_newtable(L);
    int mt = lua_gettop(L);

    lua_pushvalue(L, mt);
    lua_setfield(L, mt, "__index");

    lua_pushinteger(L, 1);
    lua_setfield(L, mt, "a");

    CHECK( lua_setmetatable(L, table) == 1 );

    // Test the value in the metatable.
    lua_getfield(L, table, "a");
    CHECK( lua_tointeger(L, -1) == 1 );
    lua_pop(L, 1);

    // Test the value in the table.
    lua_getfield(L, table, "b");
    CHECK( lua_tointeger(L, -1) == 2 );
    lua_pop(L, 1);

    // Test a value that doesn't exist in either.
    lua_getfield(L, table, "c");
    CHECK( lua_isnil(L, -1) == 1 );
    lua_pop(L, 1);

    lua_close(L);

}

TEST(CClosure)
{

    struct Locals
    {
        static int Function(lua_State* L)
        {
            Locals* locals = static_cast<Locals*>(lua_touserdata(L, lua_upvalueindex(1)));
            int a = lua_tointeger(L, lua_upvalueindex(2));
            int b = lua_tointeger(L, lua_upvalueindex(3));
            CHECK( a == 10 );
            CHECK( b == 20 );
            CHECK( lua_isnil(L, lua_upvalueindex(4)) == 1 );
            locals->called = true;
            return 0;
        }
        bool called;
    };

    lua_State* L = luaL_newstate();

    int start = lua_gettop(L);

    Locals locals;
    locals.called = false;

    lua_pushlightuserdata(L, &locals);
    lua_pushinteger(L, 10);
    lua_pushinteger(L, 20);
    lua_pushcclosure(L, Locals::Function, 3);

    // The closure should be the only thing left.
    CHECK( lua_gettop(L) - start == 1 );

    lua_call(L, 0, 0);
    CHECK( locals.called );

    lua_close(L);

}

TEST(LightUserData)
{

    lua_State* L = luaL_newstate();

    void* p = reinterpret_cast<void*>(0x12345678);

    lua_pushlightuserdata(L, p);

    CHECK( lua_type(L, -1) == LUA_TLIGHTUSERDATA );
    CHECK( lua_touserdata(L, -1) == p );

    lua_pop(L, 1);

    lua_close(L);

}

TEST(NewIndexMetamethod)
{

    struct Locals
    {
        static int NewIndex(lua_State* L)
        {
            Locals* locals = static_cast<Locals*>(lua_touserdata(L, lua_upvalueindex(1)));
            CHECK( lua_gettop(L) == 3 );
            CHECK_EQ( lua_tostring(L, 2), "key" );
            CHECK_EQ( lua_tostring(L, 3), "value" );
            locals->called = true;
            return 0;
        }
        bool called;
    };

    lua_State* L = luaL_newstate();

    lua_newtable(L);
    int table = lua_gettop(L);

    lua_newtable(L);
    int mt = lua_gettop(L);

    Locals locals;
    locals.called = false;

    lua_pushlightuserdata(L, &locals);
    lua_pushcclosure(L, &Locals::NewIndex, 1);
    lua_setfield(L, mt, "__newindex");

    CHECK( lua_setmetatable(L, table) == 1 );

    lua_pushstring(L, "value");
    lua_setfield(L, table, "key");
    CHECK( locals.called );

    lua_pop(L, 1);

    lua_close(L);

}

/*
TEST(TableConstructor1)
{
    const char* code =
        "t = { 'one', 'two' }";

    lua_State* L = luaL_newstate();
    CHECK( DoString(L, code) );

    lua_getglobal(L, "t");
    CHECK( lua_istable(L, -1) == 1 );

    lua_rawgeti(L, -1, 1);
    CHECK( strcmp(lua_tostring(L, -1), "one") == 0 );
    lua_pop(L, 1);

    lua_rawgeti(L, -1, 2);
    CHECK( strcmp(lua_tostring(L, -1), "two") == 0 );
    lua_pop(L, 1);

    lua_close(L);

}

TEST(TableConstructor2)
{
    const char* code =
        "t = { first = 1 }";

    lua_State* L = luaL_newstate();
    CHECK( DoString(L, code) );

    lua_getglobal(L, "t");
    CHECK( lua_istable(L, -1) == 1 );

    lua_getfield(L, -1, "first");
    CHECK( lua_tonumber(L, -1) == 1.0 );

    lua_close(L);

}
*/

TEST(FunctionMethod)
{

    const char* code =
        "result = false\n"
        "Foo = { }\n"
        "function Foo:Bar()\n"
        "  if self == Foo then result = true end\n"
        "end\n"
        "Foo:Bar()\n";

    lua_State* L = luaL_newstate();
    CHECK( DoString(L, code) );
    
    // Check that the function was properly created.
    lua_getglobal(L, "Foo");
    lua_getfield(L, -1, "Bar");
    CHECK( lua_type(L, -1) == LUA_TFUNCTION );
    lua_pop(L, 2);

    // Check that the function was properly called.
    lua_getglobal(L, "result");
    CHECK( lua_toboolean(L, -1) == 1 );
    
    lua_close(L);

}

TEST(FunctionDefinition)
{
    const char* code = "function Foo() end";

    lua_State* L = luaL_newstate();
    CHECK( luaL_dostring(L, code) == 0 );

    lua_getglobal(L, "Foo");
    CHECK( lua_type(L, -1) == LUA_TFUNCTION );
    
    lua_close(L);

}

TEST(ScopedFunctionDefinition)
{
    
    const char* code = 
        "Foo = { }\n"
        "Foo.Bar = { }\n"
        "function Foo.Bar.Baz() end";
    
    lua_State* L = luaL_newstate();

    CHECK( luaL_dostring(L, code) == 0 );
    
    lua_getglobal(L, "Foo");
    lua_getfield(L, -1, "Bar");
    lua_getfield(L, -1, "Baz");
    
    CHECK( lua_type(L, -1) == LUA_TFUNCTION );
    
    lua_close(L);

}

TEST(FunctionMethodDefinition)
{

    const char* code =
        "Foo = { }\n"
        "function Foo:Bar() end";

    lua_State* L = luaL_newstate();
    CHECK( luaL_dostring(L, code) == 0 );
    
    lua_getglobal(L, "Foo");
    lua_getfield(L, -1, "Bar");
    CHECK( lua_type(L, -1) == LUA_TFUNCTION );
    
    lua_close(L);

}

TEST(LocalFunctionDefinition)
{
    const char* code =
        "local function Foo() end\n"
        "Bar = Foo";

    lua_State* L = luaL_newstate();
    CHECK( luaL_dostring(L, code) == 0 );
    
    lua_getglobal(L, "Bar");
    CHECK( lua_type(L, -1) == LUA_TFUNCTION );

    lua_getglobal(L, "Foo");
    CHECK( lua_type(L, -1) == LUA_TNIL );

    lua_close(L);

}

TEST(LocalScopedFunctionDefinition)
{
    
    const char* code = 
        "Foo = { }\n"
        "local function Foo.Bar() end";

    // Scoping makes no sense when we're defining a local.
    lua_State* L = luaL_newstate();
    CHECK( luaL_dostring(L, code) == 1 );
    
    lua_close(L);

}

TEST(WhileLoop)
{

    const char* code = 
        "index = 0\n"
        "while index < 10 do\n"
        "  index = index + 1\n"
        "end";

    lua_State* L = luaL_newstate();

    CHECK( DoString(L, code) );

    lua_getglobal(L, "index");
    CHECK( lua_type(L, -1) == LUA_TNUMBER );
    CHECK( lua_tointeger(L, -1) == 10 );

    lua_close(L);

}

TEST(ForLoop1)
{

    const char* code = 
        "index = 0\n"
        "for i = 1,10 do\n"
        "  index = index + 1\n"
        "end";

    lua_State* L = luaL_newstate();

    CHECK( DoString(L, code) );

    lua_getglobal(L, "index");
    CHECK( lua_type(L, -1) == LUA_TNUMBER );
    CHECK( lua_tointeger(L, -1) == 10 );

    // The index for the loop shouldn't be in the global space.
    lua_getglobal(L, "i");
    CHECK( lua_isnil(L, -1) != 0 );

    lua_close(L);

}

TEST(ForLoop2)
{

    const char* code = 
        "index = 0\n"
        "for i = 1,10,2 do\n"
        "  index = index + 1\n"
        "end";

    lua_State* L = luaL_newstate();

    CHECK( DoString(L, code) );

    lua_getglobal(L, "index");
    CHECK( lua_type(L, -1) == LUA_TNUMBER );
    CHECK( lua_tointeger(L, -1) == 5 );

    // The index for the loop shouldn't be in the global space.
    lua_getglobal(L, "i");
    CHECK( lua_isnil(L, -1) != 0 );

    lua_close(L);

}

TEST(ForLoop3)
{

    const char* code = 
        "values = { first=1, second=2 }\n"
        "results = { }\n"
        "index = 0\n"
        "for k,v in pairs(values) do\n"
        "  index = index + 1\n"
        "  results[v] = k\n"
        "end";

    lua_State* L = luaL_newstate();

    CHECK( DoString(L, code) );

    lua_getglobal(L, "index");
    CHECK( lua_type(L, -1) == LUA_TNUMBER );
    CHECK( lua_tointeger(L, -1) == 2 );

    // The index for the loop shouldn't be in the global space.
    lua_getglobal(L, "k");
    CHECK( lua_isnil(L, -1) != 0 );
    lua_getglobal(L, "v");
    CHECK( lua_isnil(L, -1) != 0 );

    lua_getglobal(L, "results");

    lua_rawgeti(L, -1, 1);
    CHECK( strcmp( lua_tostring(L, -1), "first" ) == 0 );
    lua_pop(L, 1);

    lua_rawgeti(L, -1, 2);
    CHECK( strcmp( lua_tostring(L, -1), "second" ) == 0 );
    lua_pop(L, 1);

    lua_close(L);

}

TEST(RepeatLoop)
{

    const char* code = 
        "index = 0\n"
        "repeat\n"
        "  index = index + 1\n"
        "until index == 10";

    lua_State* L = luaL_newstate();

    CHECK( DoString(L, code) );

    lua_getglobal(L, "index");
    CHECK( lua_type(L, -1) == LUA_TNUMBER );
    CHECK( lua_tointeger(L, -1) == 10 );

    lua_close(L);

}