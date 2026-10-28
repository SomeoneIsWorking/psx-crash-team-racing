cmake_minimum_required(VERSION 3.24)
project(ctr_port LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(PSXPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/psxport" CACHE PATH
    "psxport framework source directory")
if(NOT EXISTS "${PSXPORT_DIR}/cmake/psxport.cmake")
  message(FATAL_ERROR
          "PSXPORT_DIR=${PSXPORT_DIR} has no cmake/psxport.cmake; run the Python framework resolver")
endif()
message(STATUS "psxport framework: ${PSXPORT_DIR}")

add_subdirectory("${PSXPORT_DIR}" psxport_build)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/titles/ctr/executable.json" CTR_EXECUTABLE_MANIFEST)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/titles/ctr/executable.json")
string(JSON CTR_EXECUTABLE_SHA256 GET "${CTR_EXECUTABLE_MANIFEST}" sha256)
string(JSON CTR_EXECUTABLE_SIZE GET "${CTR_EXECUTABLE_MANIFEST}" file_size)
string(JSON CTR_DISC_EXECUTABLE GET "${CTR_EXECUTABLE_MANIFEST}" disc_executable)
string(JSON CTR_EXECUTABLE_ENTRY GET "${CTR_EXECUTABLE_MANIFEST}" header entry)
string(JSON CTR_EXECUTABLE_GP GET "${CTR_EXECUTABLE_MANIFEST}" header gp)
string(JSON CTR_EXECUTABLE_TEXT GET "${CTR_EXECUTABLE_MANIFEST}" header text_address)
string(JSON CTR_EXECUTABLE_TEXT_SIZE GET "${CTR_EXECUTABLE_MANIFEST}" header text_size)
string(JSON CTR_EXECUTABLE_STACK GET "${CTR_EXECUTABLE_MANIFEST}" header stack_address)
string(JSON CTR_EXECUTABLE_STACK_OFFSET GET "${CTR_EXECUTABLE_MANIFEST}" header stack_offset)
configure_file(game/product/executable_identity.h.in identity/executable_identity.h @ONLY)

set(CTR_OVERLAY_CATALOG "${CMAKE_CURRENT_BINARY_DIR}/identity/overlay_catalog.h")
add_custom_command(
  OUTPUT "${CTR_OVERLAY_CATALOG}"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/extract_overlays.py"
          --emit-header "${CTR_OVERLAY_CATALOG}"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/titles/ctr/overlays.json"
          "${CMAKE_CURRENT_SOURCE_DIR}/tools/extract_overlays.py"
  VERBATIM
)

# The title's owners; the product and every test link them.
add_library(
  ctr_game STATIC
  "${CTR_OVERLAY_CATALOG}"
  game/boot/startup_audio_wait.cpp
  game/boot/startup_resource_load.cpp
  game/boot/startup_resource_pump.cpp
  game/debug/debug_menu.cpp
  game/debug/level_warp.cpp
  game/disc/async_disc_owner.cpp
  game/disc/overlay_image_owner.cpp
  game/execution/guest_turns.cpp
  game/execution/platform_leaves.cpp
  game/execution/spu_dma_callback_registration.cpp
  game/frame/field_boundary.cpp
  game/frame/field_override_scope.cpp
  game/frame/frame_callback_owner.cpp
  game/frame/frame_driver.cpp
  game/frame/frame_suffix.cpp
  game/frame/retail_return.cpp
  game/frame/vsync_bridge.cpp
  game/runtime/runtime.cpp
  game/runtime/session.cpp
  game/video/presentation_owner.cpp
  game/video/projection_owner.cpp
  game/video/scene_cut.cpp
  game/video/widescreen_owner.cpp
  game/video/view_frustum_owner.cpp
)
target_include_directories(ctr_game PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/game" "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_features(ctr_game PUBLIC cxx_std_20)
target_link_libraries(ctr_game PUBLIC psxport)

psxport_add_product(
  ctr_port
  CATALOG ctr::createCatalog
  HEADER product/catalog.h
  SOURCES game/product/catalog.cpp
  LIBRARIES ctr_game
)
target_compile_features(ctr_port PRIVATE cxx_std_20)
set_target_properties(ctr_port PROPERTIES ENABLE_EXPORTS ON)
add_dependencies(ctr_port gen_gpu_shaders)
