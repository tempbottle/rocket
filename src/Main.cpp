/*
 * RocketVM
 * Copyright (c) 2011 Max McGuire
 *
 * See copyright notice in lua.h
 */

#include "Test.h"

#include "lua.h"
#include "lauxlib.h"

#include <stdio.h>

int main(int argc, char* argv[])
{
    Test_RunTests();
    return 0;
} 