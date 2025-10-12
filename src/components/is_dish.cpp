#include "is_dish.h"
#include "../dish_types.h"

std::string IsDish::name() const { return get_dish_info(type).name; }

raylib::Color IsDish::color() const { return get_dish_info(type).color; }

int IsDish::price() const { return get_dish_info(type).price; }

FlavorStats IsDish::flavor() const { return get_dish_info(type).flavor; }