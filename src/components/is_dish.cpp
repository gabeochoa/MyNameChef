#include "is_dish.h"
#include "../dish_types.h"

std::string IsDish::name() const { return get_dish_info(type).name; }

FlavorStats IsDish::flavor() const { return get_dish_info(type).flavor; }