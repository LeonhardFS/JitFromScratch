cmake_minimum_required(VERSION 3.10)

project(JitFromScratch)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

if(NOT CMAKE_TOOLCHAIN_FILE)
  include(DefaultToolchain)
endif()

include(LLVM)

if(LLVM_ENABLE_RTTI)
  message(WARNING "LLVM built with RTTI; enabling it for JitFromScratch to avoid quirks")
  set(rtti_flags ${HOST_ENABLE_RTTI})
else()
  set(rtti_flags ${HOST_DISABLE_RTTI})
endif()

# If we have a LLVM source-tree, symlink it for debug source maps
if(LLVM_BUILD_MAIN_SRC_DIR AND EXISTS "${LLVM_BUILD_MAIN_SRC_DIR}" AND UNIX)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${LLVM_BUILD_MAIN_SRC_DIR}
                                               ${CMAKE_BINARY_DIR}/llvm-project
  )
endif()

# Import extra CMake helpers from LLVM
list(APPEND CMAKE_MODULE_PATH ${LLVM_DIR})
include(AddLLVM)

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "No explicit CMAKE_BUILD_TYPE provided; JitFromScratch defaults to the build type of LLVM")
  set(CMAKE_BUILD_TYPE ${LLVM_BUILD_TYPE} CACHE STRING "" FORCE)
elseif(NOT ${LLVM_BUILD_TYPE} STREQUAL ${CMAKE_BUILD_TYPE})
  message(WARNING "JitFromScratch set to build as ${CMAKE_BUILD_TYPE}, which differs from LLVM's build type")
endif()

add_executable(JitFromScratch
  main.cpp
  JitFromScratch.cpp
  SimpleOptimizer.cpp
  SimpleObjectCache.cpp
)

target_include_directories(JitFromScratch PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${LLVM_INCLUDE_DIRS}
)

set(llvm_libs
  LLVMAggressiveInstCombine
  LLVMAnalysis
  LLVMAsmParser
  LLVMAsmPrinter
  LLVMBinaryFormat
  LLVMBitReader
  LLVMBitWriter
  LLVMBitstreamReader
  LLVMCodeGen
  LLVMCore
  LLVMDebugInfoCodeView
  LLVMDebugInfoDWARF
  LLVMDebugInfoMSF
  LLVMDemangle
  LLVMExecutionEngine
  LLVMGlobalISel
  LLVMInstCombine
  LLVMInstrumentation
  LLVMipo
  LLVMJITLink
  LLVMLinker
  LLVMIRReader
  LLVMMC
  LLVMMCDisassembler
  LLVMMCParser
  LLVMObject
  LLVMOrcJIT
  LLVMProfileData
  LLVMRemarks
  LLVMRuntimeDyld
  LLVMScalarOpts
  LLVMSelectionDAG
  LLVMSupport
  LLVMTarget
  LLVMTransformUtils
  LLVMVectorize
  LLVMX86AsmParser
  LLVMX86CodeGen
  LLVMX86Desc
  LLVMX86Info
  LLVMX86Utils
)

separate_arguments(LLVM_DEFINITIONS)
target_compile_definitions(JitFromScratch PRIVATE ${LLVM_DEFINITIONS})
target_link_libraries(JitFromScratch PRIVATE ${llvm_libs})
target_compile_options(JitFromScratch PRIVATE ${rtti_flags})

# Allow dlopen() to access exported symbols.
# This will add linker flags like -Wl,--export-dynamic -rdynamic
export_executable_symbols(JitFromScratch)

function(dump_target_properties target property)
  get_target_property(values ${target} ${property})
  if(values)
    if(NOT "${values}" STREQUAL "${property}-NOTFOUND")
      message(STATUS "${target} ${property}=${values}")
    endif()
  endif()
endfunction()

dump_target_properties(JitFromScratch INCLUDE_DIRECTORIES)
dump_target_properties(JitFromScratch COMPILE_OPTIONS)
dump_target_properties(JitFromScratch COMPILE_DEFINITIONS)
dump_target_properties(JitFromScratch LINK_LIBRARIES)
dump_target_properties(JitFromScratch LINK_FLAGS)

# Only LLVM Debug builds can parse debug arguments
if(${LLVM_BUILD_TYPE} STREQUAL "Debug" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(debug_args -debug -debug-only=jitfromscratch)
endif()

add_custom_target(run
  COMMAND $<TARGET_FILE:JitFromScratch> ${debug_args}
  COMMENT "Running JitFromScratch"
)

add_custom_target(run-cached
  COMMAND $<TARGET_FILE:JitFromScratch> ${debug_args} -cache-dir=cache/
  COMMENT "Running JitFromScratch (with object cache)"
)

add_subdirectory(test)
