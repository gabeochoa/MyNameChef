#pragma once

#include "../../components/toast_message.h"
#include "../../components/transform.h"
#include "../../query.h"
#include "../../shop.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

TEST(validate_toast_creation) {
  app.launch_game();

  make_toast("Test toast message");

  app.wait_for_frames(5);

  auto toast_opt = EQ().whereHasComponent<ToastMessage>().gen_first();
  app.expect_true(toast_opt.has_value(), "toast entity exists");

  const auto &toast = toast_opt.asE().get<ToastMessage>();
  app.expect_eq(toast.message, std::string("Test toast message"),
                "toast message");
  app.expect_entity_has_component<Transform>(toast_opt.asE().id);
  app.expect_entity_has_component<ToastMessage>(toast_opt.asE().id);
}

TEST(validate_toast_lifetime_countdown) {
  app.launch_game();

  make_toast("Countdown test", 1.0f);

  app.wait_for_frames(5);

  auto toast_opt = EQ().whereHasComponent<ToastMessage>().gen_first();
  app.expect_true(toast_opt.has_value(), "toast entity exists");

  auto &toast = toast_opt.asE().get<ToastMessage>();
  const float initialLifetime = toast.initialLifetime;
  const float initialLifetimeValue = toast.lifetime;

  app.expect_eq(toast.lifetime, initialLifetime, "initial lifetime");
  app.expect_true(initialLifetime > 1.0f,
                  "lifetime includes enter/exit duration");

  app.wait_for_frames(30);

  const float updatedLifetime = toast.lifetime;
  app.expect_true(updatedLifetime < initialLifetimeValue, "lifetime decreased");
}

TEST(validate_toast_cleanup_after_expiry) {
  app.launch_game();

  make_toast("Short lived toast", 0.1f);

  app.wait_for_frames(5);

  auto initial_toast_opt = EQ().whereHasComponent<ToastMessage>().gen_first();
  app.expect_true(initial_toast_opt.has_value(), "toast entity created");

  const auto &toast = initial_toast_opt.asE().get<ToastMessage>();
  const float totalDuration = toast.initialLifetime;

  const int framesToWait = static_cast<int>((totalDuration + 0.5f) * 60.0f);
  app.wait_for_frames(framesToWait);

  auto expired_toast_opt = EQ().whereHasComponent<ToastMessage>().gen_first();
  app.expect_false(expired_toast_opt.has_value(), "toast entity cleaned up");
}

TEST(validate_multiple_toasts) {
  app.launch_game();

  make_toast("First toast");
  app.wait_for_frames(5);
  make_toast("Second toast");
  app.wait_for_frames(5);
  make_toast("Third toast");

  app.wait_for_frames(10);

  auto toasts = EQ().whereHasComponent<ToastMessage>().gen();
  app.expect_count_gte(static_cast<int>(toasts.size()), 3,
                       "multiple toasts exist");
}

TEST(validate_toast_custom_duration) {
  app.launch_game();

  make_toast("Custom duration", 5.0f);

  app.wait_for_frames(5);

  auto toast_opt = EQ().whereHasComponent<ToastMessage>().gen_first();
  app.expect_true(toast_opt.has_value(), "toast entity exists");

  const auto &toast = toast_opt.asE().get<ToastMessage>();
  const float expectedInitialLifetime = 5.0f + 0.3f + 0.3f;
  app.expect_eq(toast.initialLifetime, expectedInitialLifetime,
                "custom duration lifetime");
}
