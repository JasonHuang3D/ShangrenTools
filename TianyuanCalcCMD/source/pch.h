//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#endif

#ifndef NOMINMAX
#define NOMINMAX // Urgly. but YEAH!
#endif
#include <windows.h>
#endif // WIN32
