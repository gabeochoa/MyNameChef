#pragma once

#include <afterhours/ah.h>

namespace afterhours::ui::controls {

using afterhours::Entity;
using afterhours::ui::imm::button;
using afterhours::ui::imm::checkbox;
using afterhours::ui::imm::ComponentConfig;
using afterhours::ui::imm::ElementResult;
using afterhours::ui::imm::mk;
using afterhours::ui::imm::slider;
using afterhours::ui::imm::SliderHandleValueLabelPosition;

template <typename InputAction>
inline ElementResult
button_labeled(afterhours::ui::UIContext<InputAction> &context, Entity &parent,
               const std::string &label, std::function<void()> on_click,
               int index = 0) {
  if (button(context, mk(parent, index),
             ComponentConfig{}.with_debug_name(label).with_label(label))) {
    on_click();
    return {true, parent};
  }
  return {false, parent};
}

template <typename InputAction>
inline ElementResult
slider_labeled(afterhours::ui::UIContext<InputAction> &context, Entity &parent,
               const std::string &label, float &value,
               std::function<void(float)> on_change, int index = 0) {
  if (auto result = slider(context, mk(parent, index), value,
                           ComponentConfig{}.with_label(label),
                           SliderHandleValueLabelPosition::OnHandle)) {
    value = result.template as<float>();
    on_change(value);
    return {true, parent};
  }
  return {false, parent};
}

template <typename InputAction>
inline bool checkbox_labeled(afterhours::ui::UIContext<InputAction> &context,
                             Entity &parent, const std::string &label,
                             bool checked, int index = 0) {
  return checkbox(context, mk(parent, index), checked,
                  ComponentConfig{}.with_label(label));
}

} // namespace afterhours::ui::controls
