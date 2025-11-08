#pragma once

#include <afterhours/ah.h>
#include <afterhours/src/singleton.h>

SINGLETON_FWD(ContinueButtonDisabled)
struct ContinueButtonDisabled : afterhours::BaseComponent {
  SINGLETON(ContinueButtonDisabled)
};
