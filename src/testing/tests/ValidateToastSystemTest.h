#pragma once

#include "../../components/toast_message.h"
#include "../../components/transform.h"
#include "../../query.h"
#include "../../shop.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

namespace ValidateToastSystemTestHelpers {

static afterhours::OptEntity find_toast(const std::string &message) {
  return EQ({.force_merge = true})
      .whereHasComponent<ToastMessage>()
      .whereLambda([&message](const afterhours::Entity &e) {
        return e.get<ToastMessage>().message == message;
      })
      .gen_first();
}

} // namespace ValidateToastSystemTestHelpers

TEST(validate_toast_creation) {
  using namespace ValidateToastSystemTestHelpers;
  app.launch_game();

  app.once([&] { make_toast("Test toast message"); });

  app.wait_for_frames(5);

  afterhours::OptEntity toast_opt = find_toast("Test toast message");
  app.expect_true(toast_opt.has_value(), "toast entity exists");

  const ToastMessage &toast = toast_opt.asE().get<ToastMessage>();
  app.expect_eq(toast.message, std::string("Test toast message"),
                "toast message");
  app.expect_entity_has_component<Transform>(toast_opt.asE().id);
  app.expect_entity_has_component<ToastMessage>(toast_opt.asE().id);
}

TEST(validate_toast_lifetime_countdown) {
  using namespace ValidateToastSystemTestHelpers;
  app.launch_game();

  app.once([&] { make_toast("Countdown test", 1.0f); });

  app.wait_for_frames(5);

  app.once([&] {
    afterhours::OptEntity toast_opt = find_toast("Countdown test");
    app.expect_true(toast_opt.has_value(), "toast entity exists");
    const ToastMessage &toast = toast_opt.asE().get<ToastMessage>();
    app.expect_true(toast.lifetime <= toast.initialLifetime,
                    "lifetime counts down from initial lifetime");
    app.expect_true(toast.initialLifetime > 1.0f,
                    "lifetime includes enter/exit duration");
    app.set_test_int("lifetime_ms",
                     static_cast<int>(toast.lifetime * 1000.0f));
  });

  app.wait_for_frames(3);

  afterhours::OptEntity toast_opt = find_toast("Countdown test");
  bool decreased =
      !toast_opt.has_value() ||
      static_cast<int>(toast_opt.asE().get<ToastMessage>().lifetime *
                       1000.0f) < app.get_test_int("lifetime_ms").value();
  app.expect_true(decreased, "lifetime decreased");
}

TEST(validate_toast_cleanup_after_expiry) {
  using namespace ValidateToastSystemTestHelpers;
  app.launch_game();

  app.once([&] {
    make_toast("Short lived toast", 0.1f);
    app.expect_true(find_toast("Short lived toast").has_value(),
                    "toast entity created");
  });

  app.wait_for_frames(72);

  app.expect_false(find_toast("Short lived toast").has_value(),
                   "toast entity cleaned up");
}

TEST(validate_multiple_toasts) {
  using namespace ValidateToastSystemTestHelpers;
  app.launch_game();

  app.once([&] { make_toast("First toast"); });
  app.wait_for_frames(5);
  app.once([&] { make_toast("Second toast"); });
  app.wait_for_frames(5);
  app.once([&] { make_toast("Third toast"); });

  app.wait_for_frames(10);

  int toast_count = 0;
  for (const std::string &message :
       {"First toast", "Second toast", "Third toast"}) {
    if (find_toast(message).has_value()) {
      toast_count++;
    }
  }
  app.expect_count_gte(toast_count, 3, "multiple toasts exist");
}

TEST(validate_toast_custom_duration) {
  using namespace ValidateToastSystemTestHelpers;
  app.launch_game();

  app.once([&] { make_toast("Custom duration", 5.0f); });

  app.wait_for_frames(5);

  afterhours::OptEntity toast_opt = find_toast("Custom duration");
  app.expect_true(toast_opt.has_value(), "toast entity exists");

  const ToastMessage &toast = toast_opt.asE().get<ToastMessage>();
  const float expectedInitialLifetime = 5.0f + 0.3f + 0.3f;
  app.expect_eq(toast.initialLifetime, expectedInitialLifetime,
                "custom duration lifetime");
}
