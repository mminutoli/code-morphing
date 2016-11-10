add_library(CMPasses SHARED ForceLinking.cpp)
target_link_libraries(CMPasses CodeMorphing ${llvm_libs})
