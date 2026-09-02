#pragma once
#include <iostream>

#ifdef DEBUG_MODE
    #define DEBUG_LOG(msg) std::cout << "[DEBUG] " << msg << std::endl
#else
    #define DEBUG_LOG(msg) do {} while(0)
#endif
