#pragma once

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>

#include <string>
#include <sstream>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>


#include "Quentlam/Core/Log.h"
#include "spdlog/fmt/ostr.h"
#include <spdlog/fmt/fmt.h> 
#include "Quentlam/Debug/Instrumentor.h"

#ifdef QL_PLATFORM_WINDOWS
#include <Windows.h>
#ifdef State
#undef State
#endif
#endif 