#pragma once

#include <string>
#include <cstring>

template <typename T>
T init_credential(const char *value)
{
    T t{}; // zero initialize
    std::strcpy(t.data(), value);
    return t;
}
