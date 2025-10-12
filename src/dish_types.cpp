// Data-driven dish registry implementation
#include "dish_types.h"
#include "resources.h"
#include "log.h"
#include <map>
#include <nlohmann/json.hpp>
#include <fstream>
#include <optional>

using json = nlohmann::json;

namespace {
  // Registry for dish metadata
  std::map<DishType, DishInfo> &registry() {
    static std::map<DishType, DishInfo> reg;
    return reg;
  }

  // String table for enum conversion
  const std::vector<std::pair<DishType, const char *>> &dish_enum_names() {
    static const std::vector<std::pair<DishType, const char *>> names = {
        {DishType::Potato, "Potato"},
        {DishType::GarlicBread, "GarlicBread"},
        {DishType::TomatoSoup, "TomatoSoup"},
        {DishType::GrilledCheese, "GrilledCheese"},
        {DishType::ChickenSkewer, "ChickenSkewer"},
        {DishType::CucumberSalad, "CucumberSalad"},
        {DishType::VanillaSoftServe, "VanillaSoftServe"},
        {DishType::CapreseSalad, "CapreseSalad"},
        {DishType::Minestrone, "Minestrone"},
        {DishType::SearedSalmon, "SearedSalmon"},
        {DishType::SteakFlorentine, "SteakFlorentine"},
    };
    return names;
  }
}

std::optional<DishType> dish_type_from_string(const std::string &name) {
  for (const auto &[t, s] : dish_enum_names()) {
    if (name == s) return t;
  }
  return std::nullopt;
}

std::string dish_type_to_string(DishType type) {
  for (const auto &[t, s] : dish_enum_names()) {
    if (t == type) return s;
  }
  return "";
}

DishInfo get_dish_info(DishType type) {
  auto it = registry().find(type);
  if (it != registry().end()) return it->second;
  // Fallback to something sane to avoid crashes; also useful during authoring
  return DishInfo{.name = dish_type_to_string(type)};
}

static raylib::Color color_from_json(const json &j) {
  // Accept either array [r,g,b,a] or object {r,g,b,a}
  if (j.is_array() && j.size() >= 3) {
    int r = j[0].get<int>();
    int g = j[1].get<int>();
    int b = j[2].get<int>();
    int a = (j.size() >= 4) ? j[3].get<int>() : 255;
    return raylib::Color{(unsigned char)r, (unsigned char)g, (unsigned char)b,
                         (unsigned char)a};
  }
  if (j.is_object()) {
    int r = j.value("r", 200);
    int g = j.value("g", 200);
    int b = j.value("b", 200);
    int a = j.value("a", 255);
    return raylib::Color{(unsigned char)r, (unsigned char)g, (unsigned char)b,
                         (unsigned char)a};
  }
  return raylib::Color{200, 200, 200, 255};
}

bool load_dish_types_from_json() {
  const std::string path =
      Files::get().fetch_resource_path("", "dishes.json");
  std::ifstream ifs(path);
  if (!ifs.is_open()) {
    log_warn("dishes.json not found at {} — using compiled defaults", path);
    // Populate with previous hardcoded defaults to keep game working
    registry().clear();
    registry()[DishType::Potato] = DishInfo{.name = "Potato",
                                            .color = raylib::Color{200,150,100,255},
                                            .price = 1,
                                            .flavor = FlavorStats{.satiety = 1}};
    registry()[DishType::GarlicBread] = DishInfo{.name = "Garlic Bread",
                                                 .color = raylib::Color{180,120,80,255},
                                                 .price = 3,
                                                 .flavor = FlavorStats{.satiety = 2, .richness = 2}};
    registry()[DishType::TomatoSoup] = DishInfo{.name = "Tomato Soup",
                                                .color = raylib::Color{160,40,40,255},
                                                .price = 3,
                                                .flavor = FlavorStats{.acidity = 2, .freshness = 2}};
    registry()[DishType::GrilledCheese] = DishInfo{.name = "Grilled Cheese",
                                                   .color = raylib::Color{200,160,60,255},
                                                   .price = 3,
                                                   .flavor = FlavorStats{.richness = 2, .satiety = 2}};
    registry()[DishType::ChickenSkewer] = DishInfo{.name = "Chicken Skewer",
                                                   .color = raylib::Color{150,80,60,255},
                                                   .price = 3,
                                                   .flavor = FlavorStats{.spice = 2, .umami = 2}};
    registry()[DishType::CucumberSalad] = DishInfo{.name = "Cucumber Salad",
                                                   .color = raylib::Color{60,160,100,255},
                                                   .price = 3,
                                                   .flavor = FlavorStats{.freshness = 3}};
    registry()[DishType::VanillaSoftServe] = DishInfo{.name = "Vanilla Soft Serve",
                                                      .color = raylib::Color{220,200,180,255},
                                                      .price = 3,
                                                      .flavor = FlavorStats{.sweetness = 3, .richness = 1}};
    registry()[DishType::CapreseSalad] = DishInfo{.name = "Caprese Salad",
                                                  .color = raylib::Color{120,200,140,255},
                                                  .price = 3,
                                                  .flavor = FlavorStats{.freshness = 2, .acidity = 1, .umami = 1}};
    registry()[DishType::Minestrone] = DishInfo{.name = "Minestrone",
                                                .color = raylib::Color{160,90,60,255},
                                                .price = 3,
                                                .flavor = FlavorStats{.satiety = 2, .freshness = 1}};
    registry()[DishType::SearedSalmon] = DishInfo{.name = "Seared Salmon",
                                                  .color = raylib::Color{230,140,90,255},
                                                  .price = 3,
                                                  .flavor = FlavorStats{.umami = 3, .richness = 2}};
    registry()[DishType::SteakFlorentine] = DishInfo{.name = "Steak Florentine",
                                                     .color = raylib::Color{160,80,80,255},
                                                     .price = 3,
                                                     .flavor = FlavorStats{.satiety = 3, .umami = 2, .richness = 2}};
    return false;
  }

  json root;
  try {
    ifs >> root;
  } catch (const std::exception &e) {
    log_error("failed to parse dishes.json: {}", e.what());
    return false;
  }

  if (!root.is_object() || !root.contains("dishes") || !root["dishes"].is_array()) {
    log_error("dishes.json missing 'dishes' array");
    return false;
  }

  registry().clear();
  for (const auto &jDish : root["dishes"]) {
    std::string key = jDish.value("type", "");
    auto maybeType = dish_type_from_string(key);
    if (!maybeType) {
      log_warn("Unknown dish type '{}' in dishes.json; skipping", key);
      continue;
    }
    DishType dt = *maybeType;

    DishInfo info;
    info.name = jDish.value("name", key);
    info.price = jDish.value("price", 1);
    if (jDish.contains("color")) info.color = color_from_json(jDish["color"]);

    auto jf = jDish.value("flavor", json::object());
    info.flavor.satiety = jf.value("satiety", 0);
    info.flavor.sweetness = jf.value("sweetness", 0);
    info.flavor.spice = jf.value("spice", 0);
    info.flavor.acidity = jf.value("acidity", 0);
    info.flavor.umami = jf.value("umami", 0);
    info.flavor.richness = jf.value("richness", 0);
    info.flavor.freshness = jf.value("freshness", 0);

    registry()[dt] = info;
  }

  // Ensure all enum values exist to avoid surprises
  for (const auto &[dt, name] : dish_enum_names()) {
    if (!registry().count(dt)) {
      registry()[dt] = DishInfo{.name = name};
    }
  }

  log_info("Loaded {} dish definitions from dishes.json", registry().size());
  return true;
}