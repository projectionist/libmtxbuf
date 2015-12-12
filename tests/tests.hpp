#pragma once
#undef NDEBUG

#include <iostream>

#define step(msg) std::cerr << "\t- " << #msg << std::endl

#define TEST(test) std::cerr << std::endl << "TEST: " << #test << std::endl; \
  try {\
    test(); std::cerr << "PASS" << std::endl;\
  } catch(logic_error e) {\
    std::cerr << "FAIL: " << e.what() << std::endl;\
  }\
  std::cerr << std::endl;
