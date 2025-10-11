#pragma once

#include <afterhours/src/ecs.h>

void register_ui_systems(afterhours::SystemManager &systems);
void enforce_ui_singletons(afterhours::SystemManager &systems);
void add_ui_singleton_components(afterhours::Entity &entity);
