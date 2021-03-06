cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(SceShellDispBattery)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -fno-inline -fno-builtin -fshort-wchar")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

enable_language(C ASM)

add_executable(${PROJECT_NAME}
  src/main.c
  src/getCpuUsagePercent.S
  src/taihen_extra.c
)

target_link_libraries(${PROJECT_NAME}
  SceLibKernel_stub
  SceKernelModulemgr_stub
  ScePaf_stub_weak
  ScePower_stub
  taihen_stub
)

set_target_properties(${PROJECT_NAME}
  PROPERTIES LINK_FLAGS "-nostdlib"
)

vita_create_self(shellbat.suprx ${PROJECT_NAME}
  CONFIG exports.yml
  UNSAFE
)

vita_create_stubs(stubs ${PROJECT_NAME} exports.yml)
