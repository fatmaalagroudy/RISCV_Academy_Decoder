#pragma once

#include <cstdlib>
#include <string>

#ifndef TEST_INPUTS_DIR
#define TEST_INPUTS_DIR "tests/lit/Inputs"
#endif

inline std::string TestInputPath(const char* filename) {
  const char* from_env = std::getenv("TEST_INPUTS_DIR");
  const std::string dir = (from_env != nullptr && from_env[0] != '\0') ? from_env : TEST_INPUTS_DIR;
  return dir + "/" + filename;
}
