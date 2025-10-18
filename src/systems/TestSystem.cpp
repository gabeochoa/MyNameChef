#include "TestSystem.h"

std::unordered_map<std::string, std::function<void()>>
    TestSystem::test_registry;
