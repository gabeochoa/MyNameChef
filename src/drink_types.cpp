#include "drink_types.h"

DrinkInfo get_drink_info(DrinkType type) {
  switch (type) {
  case DrinkType::Water:
    return DrinkInfo{"Water", 1, {7, 5}}; // Water sprite at column 7, row 5
  }
  return DrinkInfo{"Water", 1, {7, 5}};
}

DrinkType get_random_drink() {
  // TODO: Add more drink types (soda, juice, etc.)
  return DrinkType::Water;
}
