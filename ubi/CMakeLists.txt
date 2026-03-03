cmake_minimum_required(VERSION 3.20)
project(ubi LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(UBI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(NAPI_V8_ROOT "${UBI_ROOT}/../napi/v8")
set(PROJECT_ROOT "${UBI_ROOT}/../")
set(NAPI_INCLUDE_ROOT "${PROJECT_ROOT}/napi/include")

include("${PROJECT_ROOT}/napi/v8/cmake/NapiBuildCache.cmake")
napi_enable_sccache_if_available()

set(NAPI_V8_NODE_ROOT_DEFAULT "${PROJECT_ROOT}/node")
if(DEFINED ENV{NAPI_V8_NODE_ROOT})
  set(NAPI_V8_NODE_ROOT_DEFAULT "$ENV{NAPI_V8_NODE_ROOT}")
endif()
set(NAPI_V8_NODE_ROOT
  "${NAPI_V8_NODE_ROOT_DEFAULT}"
  CACHE PATH "Path to Node source root (for raw Node tests/fixtures)"
)
option(UBI_EXTERNAL_NAPI_V8
  "Treat napi-v8 as an external dependency (do not build/bundle it in this tree)"
  OFF
)
option(UBI_ALLOW_UNDEFINED_IMPORTS
  "Allow undefined imports at link time (useful for external runtime deps in wasm/wasix)"
  OFF
)

if(CMAKE_SYSTEM_NAME MATCHES "WASI|WASIX" OR
   CMAKE_C_COMPILER MATCHES "wasixcc" OR
   CMAKE_CXX_COMPILER MATCHES "wasixcc\\+\\+")
  set(UBI_ALLOW_UNDEFINED_IMPORTS ON CACHE BOOL "" FORCE)
endif()

# Hard gate: ubi runtime/library sources must never directly reference V8.
file(GLOB_RECURSE UBI_PURE_SRC_FILES
  "${UBI_ROOT}/src/*.h"
  "${UBI_ROOT}/src/*.cc"
  "${UBI_ROOT}/src/*.c"
)
foreach(UBI_CODE_FILE IN LISTS UBI_PURE_SRC_FILES)
  file(READ "${UBI_CODE_FILE}" UBI_CODE_CONTENTS)
  if(UBI_CODE_CONTENTS MATCHES "#include <v8\\.h>" OR
     UBI_CODE_CONTENTS MATCHES "#include <libplatform/libplatform\\.h>" OR
     UBI_CODE_CONTENTS MATCHES "v8::")
    message(FATAL_ERROR
      "V8 usage is forbidden by policy in ubi/src. "
      "Found in: ${UBI_CODE_FILE}")
  endif()
endforeach()

# Build napi-v8 runtime only; node-api logic is owned by ubi.
if(NOT UBI_EXTERNAL_NAPI_V8)
  set(NAPI_V8_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  add_subdirectory("${NAPI_V8_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/napi-v8")
else()
  add_library(napi_v8 INTERFACE)
  target_include_directories(napi_v8 INTERFACE "${NAPI_INCLUDE_ROOT}")
endif()

# Use Node's bundled libuv (same as Node) for consistent behavior (e.g. mkdtemp).
set(LIBUV_BUILD_SHARED OFF CACHE BOOL "" FORCE)
add_subdirectory("${PROJECT_ROOT}/deps/uv" "${CMAKE_CURRENT_BINARY_DIR}/deps/uv")
find_package(OpenSSL REQUIRED)

set(UBI_EMBEDDED_V8_VERSION "0.0.0-node.0")
if(DEFINED NAPI_V8_INCLUDE_DIR AND EXISTS "${NAPI_V8_INCLUDE_DIR}/v8-version.h")
  file(STRINGS "${NAPI_V8_INCLUDE_DIR}/v8-version.h" _v8_major_line
       REGEX "^#define V8_MAJOR_VERSION [0-9]+$")
  file(STRINGS "${NAPI_V8_INCLUDE_DIR}/v8-version.h" _v8_minor_line
       REGEX "^#define V8_MINOR_VERSION [0-9]+$")
  file(STRINGS "${NAPI_V8_INCLUDE_DIR}/v8-version.h" _v8_build_line
       REGEX "^#define V8_BUILD_NUMBER [0-9]+$")
  file(STRINGS "${NAPI_V8_INCLUDE_DIR}/v8-version.h" _v8_patch_line
       REGEX "^#define V8_PATCH_LEVEL [0-9]+$")
  if(_v8_major_line AND _v8_minor_line AND _v8_build_line AND _v8_patch_line)
    string(REGEX REPLACE "^#define V8_MAJOR_VERSION ([0-9]+)$" "\\1"
           _v8_major "${_v8_major_line}")
    string(REGEX REPLACE "^#define V8_MINOR_VERSION ([0-9]+)$" "\\1"
           _v8_minor "${_v8_minor_line}")
    string(REGEX REPLACE "^#define V8_BUILD_NUMBER ([0-9]+)$" "\\1"
           _v8_build "${_v8_build_line}")
    string(REGEX REPLACE "^#define V8_PATCH_LEVEL ([0-9]+)$" "\\1"
           _v8_patch "${_v8_patch_line}")
    set(UBI_EMBEDDED_V8_VERSION
      "${_v8_major}.${_v8_minor}.${_v8_build}.${_v8_patch}-node.0")
  endif()
endif()

add_library(ubi_node_api STATIC
  src/node_api.cc
  src/unofficial_napi_testing_until_gc.cc
)

add_library(ubi_ncrypto STATIC
  "${PROJECT_ROOT}/deps/ncrypto/ncrypto.cc"
  "${PROJECT_ROOT}/deps/ncrypto/engine.cc"
)
target_include_directories(ubi_ncrypto
  PUBLIC
    "${PROJECT_ROOT}/deps/ncrypto"
)
target_link_libraries(ubi_ncrypto
  PUBLIC
    OpenSSL::Crypto
    OpenSSL::SSL
)

add_library(ubi_simdutf STATIC
  "${PROJECT_ROOT}/deps/simdutf/simdutf.cpp"
)
add_library(ubi_ada STATIC
  "${PROJECT_ROOT}/deps/ada/ada.cpp"
)
target_include_directories(ubi_ada
  PUBLIC
    "${PROJECT_ROOT}/deps/ada"
)

target_include_directories(ubi_simdutf
  PUBLIC
    "${PROJECT_ROOT}/deps/simdutf"
)

target_include_directories(ubi_node_api
  PRIVATE
    "${NAPI_INCLUDE_ROOT}"
)

target_link_libraries(ubi_node_api
  PUBLIC
    napi_v8
  PRIVATE
    uv_a
)

add_library(ubi_runtime STATIC
  src/ubi_runtime.cc
  src/ubi_runtime_platform_v8.cc
  src/ubi_process.cc
  src/ubi_module_loader.cc
  "${PROJECT_ROOT}/deps/llhttp/src/api.c"
  "${PROJECT_ROOT}/deps/llhttp/src/http.c"
  "${PROJECT_ROOT}/deps/llhttp/src/llhttp.c"
  src/ubi_fs.cc
  src/ubi_buffer.cc
  src/ubi_os.cc
  src/ubi_encoding.cc
  src/ubi_string_decoder.cc
  src/ubi_http_parser.cc
  src/ubi_stream_wrap.cc
  src/ubi_process_wrap.cc
  src/ubi_tcp_wrap.cc
  src/ubi_tty_wrap.cc
  src/ubi_pipe_wrap.cc
  src/ubi_signal_wrap.cc
  src/ubi_cares_wrap.cc
  src/ubi_udp_wrap.cc
  src/ubi_url.cc
  src/ubi_util.cc
  src/ubi_spawn_sync.cc
  src/ubi_timers_host.cc
  src/crypto/ubi_crypto_binding.cc
  src/ubi_crypto.cc
)
target_include_directories(ubi_runtime
  PUBLIC
    "${UBI_ROOT}/src"
  PRIVATE
    "${NAPI_INCLUDE_ROOT}"
    "${PROJECT_ROOT}/deps"
    "${PROJECT_ROOT}/deps/llhttp/include"
    "${PROJECT_ROOT}/deps/ncrypto"
)
target_link_libraries(ubi_runtime
  PUBLIC
    napi_v8
    ubi_node_api
  PRIVATE
    ubi_simdutf
    ubi_ada
    ubi_ncrypto
    uv_a
    OpenSSL::Crypto
)
target_compile_definitions(ubi_runtime
  PRIVATE
    UBI_EMBEDDED_V8_VERSION="${UBI_EMBEDDED_V8_VERSION}"
)

add_library(ubi_cli STATIC
  src/ubi_cli.cc
)
target_include_directories(ubi_cli
  PUBLIC
    "${UBI_ROOT}/src"
  PRIVATE
    "${NAPI_INCLUDE_ROOT}"
)
target_link_libraries(ubi_cli
  PUBLIC
    ubi_runtime
)

option(UBI_BUILD_CLI "Build ubi CLI binary" ON)
if(UBI_BUILD_CLI)
  add_executable(ubi
    "${UBI_ROOT}/src/ubi_main.cc"
  )
  target_include_directories(ubi
    PRIVATE
      "${UBI_ROOT}/src"
      "${NAPI_INCLUDE_ROOT}"
  )
  target_link_libraries(ubi
    PRIVATE
      ubi_cli
  )
  if(NAPI_V8_DEFINES)
    target_compile_definitions(ubi PRIVATE ${NAPI_V8_DEFINES})
  endif()
  if(UBI_ALLOW_UNDEFINED_IMPORTS)
    target_link_options(ubi PRIVATE
      "-Wl,--allow-undefined"
      "-Wl,--import-undefined"
    )
  endif()
endif()

enable_testing()
add_subdirectory(tests)
