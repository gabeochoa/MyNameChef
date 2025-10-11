#pragma once

#include <afterhours/ah.h>

namespace afterhours::ui::containers {

using afterhours::Entity;
using afterhours::ui::ComponentSize;
using afterhours::ui::FlexDirection;
using afterhours::ui::Padding;
using afterhours::ui::imm::ComponentConfig;
using afterhours::ui::imm::div;
using afterhours::ui::imm::ElementResult;
using afterhours::ui::imm::mk;

template <typename InputAction>
inline ElementResult
column_left(afterhours::ui::UIContext<InputAction> &context, Entity &parent,
            const std::string &debug_name, int index = 0) {
  return div(
      context, mk(parent, index),
      ComponentConfig{}
          .with_size(ComponentSize{afterhours::ui::screen_pct(0.2f),
                                   afterhours::ui::screen_pct(1.f)})
          .with_padding(Padding{.top = afterhours::ui::screen_pct(0.02f),
                                .left = afterhours::ui::screen_pct(0.02f)})
          .with_flex_direction(FlexDirection::Column)
          .with_debug_name(debug_name));
}

template <typename InputAction>
inline ElementResult
column_right(afterhours::ui::UIContext<InputAction> &context, Entity &parent,
             const std::string &debug_name, int index = 0) {
  return column_left(context, parent, debug_name, index);
}

template <typename InputAction>
inline ElementResult row_top(afterhours::ui::UIContext<InputAction> &context,
                             Entity &parent, const std::string &debug_name,
                             int index = 0) {
  return div(
      context, mk(parent, index),
      ComponentConfig{}
          .with_size(ComponentSize{afterhours::ui::screen_pct(1.f),
                                   afterhours::ui::screen_pct(0.2f)})
          .with_padding(Padding{.top = afterhours::ui::screen_pct(0.02f),
                                .left = afterhours::ui::screen_pct(0.02f)})
          .with_flex_direction(FlexDirection::Row)
          .with_debug_name(debug_name));
}

template <typename InputAction>
inline ElementResult row_center(afterhours::ui::UIContext<InputAction> &context,
                                Entity &parent, const std::string &debug_name,
                                int index = 0) {
  return row_top(context, parent, debug_name, index);
}

} // namespace afterhours::ui::containers
