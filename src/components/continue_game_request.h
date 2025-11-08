#pragma once

#include <afterhours/ah.h>
#include <afterhours/src/singleton.h>

SINGLETON_FWD(ContinueGameRequest)
struct ContinueGameRequest : afterhours::BaseComponent {
  SINGLETON(ContinueGameRequest)
  bool requested = false;
};
