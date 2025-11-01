
#pragma once

#define FMT_HEADER_ONLY
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif
#include <fmt/args.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#define AFTER_HOURS_REPLACE_LOGGING
#define AFTER_HOURS_LOG_WITH_COLOR
// #define AFTER_HOURS_ENTITY_ALLOC_DEBUG
#include "log/log.h"
