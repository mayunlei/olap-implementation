 g++ --std=c++11  build_call_virtual_func.cpp    `llvm-config --cxxflags --ldflags --system-libs --libs core mcjit native orcjit`   -rdynamic  -ffast-math  -g
