#include "BatchOnServeTriggersSystem.h"

std::set<int> BatchOnServeTriggersSystem::processed_courses;
std::set<int> BatchOnServeTriggersSystem::onserve_fired_courses;
int BatchOnServeTriggersSystem::last_course_index = -1;
bool BatchOnServeTriggersSystem::global_slidein_created = false;
