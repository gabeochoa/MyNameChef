

#pragma once

#include <memory>

#include <afterhours/src/library.h>
#include <afterhours/src/singleton.h>

SINGLETON_FWD(Preload)
struct Preload {
  SINGLETON(Preload)

  Preload();
  ~Preload();

  Preload(const Preload &) = delete;
  void operator=(const Preload &) = delete;

  Preload &init(const char *title);
  Preload &init(const char *title, bool headless);
  Preload &make_singleton();
};
