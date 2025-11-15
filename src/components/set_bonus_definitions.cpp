#include "set_bonus_definitions.h"
#include <magic_enum/magic_enum.hpp>

namespace synergy_bonuses {
namespace {
const DishEffect *make_effect_array(TriggerHook hook, EffectOperation op,
                                    TargetScope scope, int amount) {
  static const DishEffect effect(hook, op, scope, amount);
  static const DishEffect effects[] = {effect};
  return effects;
}
} // namespace

const std::map<CuisineTagType, std::map<int, SetBonusDefinition>> &
get_cuisine_bonus_definitions() {
  static const std::map<CuisineTagType, std::map<int, SetBonusDefinition>>
      definitions = {
          {CuisineTagType::American,
           {{2, SetBonusDefinition(TargetScope::AllAllies, 0, 1,
                                   "+1 Body to all your dishes")},
            {4, SetBonusDefinition(
                    TargetScope::AllAllies, 0, 2,
                    "+2 Body to all your dishes. OnBiteTaken: +1 Body to self",
                    make_effect_array(TriggerHook::OnBiteTaken,
                                      EffectOperation::AddCombatBody,
                                      TargetScope::Self, 1),
                    1)},
            {6, SetBonusDefinition(
                    TargetScope::AllAllies, 0, 3,
                    "+3 Body to all your dishes. OnBiteTaken: +2 Body to self",
                    make_effect_array(TriggerHook::OnBiteTaken,
                                      EffectOperation::AddCombatBody,
                                      TargetScope::Self, 2),
                    1)}}},
          {CuisineTagType::Thai,
           {{2, SetBonusDefinition(TargetScope::AllAllies, 1, 0,
                                   "+1 Zing to all your dishes")},
            {4, SetBonusDefinition(TargetScope::AllAllies, 2, 0,
                                   "+2 Zing to all your dishes")},
            {6, SetBonusDefinition(TargetScope::AllAllies, 3, 0,
                                   "+3 Zing to all your dishes")}}},
          {CuisineTagType::Italian,
           {{2, SetBonusDefinition(TargetScope::AllAllies, 0, 1,
                                   "+1 Body to all your dishes")},
            {4, SetBonusDefinition(TargetScope::AllAllies, 1, 1,
                                   "+1 Zing, +1 Body to all your dishes")},
            {6, SetBonusDefinition(TargetScope::AllAllies, 2, 2,
                                   "+2 Zing, +2 Body to all your dishes")}}},
          {CuisineTagType::Japanese,
           {{2, SetBonusDefinition(TargetScope::AllAllies, 1, 0,
                                   "+1 Zing to all your dishes")},
            {4, SetBonusDefinition(TargetScope::AllAllies, 1, 1,
                                   "+1 Zing, +1 Body to all your dishes")},
            {6, SetBonusDefinition(TargetScope::AllAllies, 2, 2,
                                   "+2 Zing, +2 Body to all your dishes")}}},
          {CuisineTagType::Mexican,
           {{2, SetBonusDefinition(TargetScope::AllOpponents, -1, 0,
                                   "-1 Zing to all opponent dishes")},
            {4, SetBonusDefinition(TargetScope::AllOpponents, -2, 0,
                                   "-2 Zing to all opponent dishes")},
            {6, SetBonusDefinition(TargetScope::AllOpponents, -3, 0,
                                   "-3 Zing to all opponent dishes")}}},
          {CuisineTagType::French,
           {{2, SetBonusDefinition(TargetScope::AllAllies, 0, 1,
                                   "+1 Body to all your dishes")},
            {4, SetBonusDefinition(TargetScope::AllAllies, 0, 2,
                                   "+2 Body to all your dishes")},
            {6, SetBonusDefinition(TargetScope::AllAllies, 0, 3,
                                   "+3 Body to all your dishes")}}},
          {CuisineTagType::Chinese,
           {{2, SetBonusDefinition(TargetScope::AllAllies, 1, 0,
                                   "+1 Zing to all your dishes")},
            {4, SetBonusDefinition(TargetScope::AllAllies, 2, 0,
                                   "+2 Zing to all your dishes")},
            {6, SetBonusDefinition(TargetScope::AllAllies, 3, 0,
                                   "+3 Zing to all your dishes")}}},
          {CuisineTagType::Indian,
           {{2, SetBonusDefinition(TargetScope::AllAllies, 1, 0,
                                   "+1 Zing to all your dishes")},
            {4, SetBonusDefinition(TargetScope::AllOpponents, -1, 0,
                                   "-1 Zing to all opponent dishes")},
            {6, SetBonusDefinition(TargetScope::AllOpponents, -2, 0,
                                   "-2 Zing to all opponent dishes")}}},
          {CuisineTagType::Korean,
           {{2, SetBonusDefinition(TargetScope::AllAllies, 1, 0,
                                   "+1 Zing to all your dishes")},
            {4, SetBonusDefinition(TargetScope::AllAllies, 1, 1,
                                   "+1 Zing, +1 Body to all your dishes")},
            {6, SetBonusDefinition(TargetScope::AllAllies, 2, 2,
                                   "+2 Zing, +2 Body to all your dishes")}}},
          {CuisineTagType::Vietnamese,
           {{2, SetBonusDefinition(TargetScope::AllAllies, 1, 0,
                                   "+1 Zing to all your dishes")},
            {4, SetBonusDefinition(TargetScope::AllAllies, 2, 0,
                                   "+2 Zing to all your dishes")},
            {6, SetBonusDefinition(TargetScope::AllAllies, 3, 0,
                                   "+3 Zing to all your dishes")}}}};
  return definitions;
}
} // namespace synergy_bonuses
