# VST3-Fassungen der beiden CLAP-Plugins über clap-wrapper (free-audio, MIT) und das VST3-SDK (Steinberg, MIT seit 3.8).
# Derselbe Plugin-Code wie die CLAP-Fassung (engine/clap/plugin.cpp, yplugin.cpp), als statische Bibliothek; der Wrapper
# übersetzt VST3 ↔ CLAP. Beide SDKs werden beim ersten Konfigurieren in festen Versionen aus GitHub geholt.
#
#   cmake -S engine/vst3 -B engine/build/vst3 -G Ninja -DCMAKE_BUILD_TYPE=Release
#   cmake --build engine/build/vst3
#   → engine/build/vst3/Laney_AOR50.vst3, engine/build/vst3/Yamaha_B100II.vst3   (Ordner-Bundles, so nach ~/.vst3 kopieren)
cmake_minimum_required(VERSION 3.21)
project(tonestack-vst3 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# clap-wrapper fest auf einen geprüften Stand (2026-10-20); er holt CLAP 1.2.10 und VST3-SDK 3.8.0 selbst (CPM)
set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES TRUE CACHE BOOL "" FORCE)
set(CLAP_WRAPPER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(clap-wrapper
  GIT_REPOSITORY https://github.com/free-audio/clap-wrapper.git
  GIT_TAG 806bba0a5f792e3966b99c1561ab9952d3316d7c)
FetchContent_MakeAvailable(clap-wrapper)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GUI REQUIRED IMPORTED_TARGET cairo x11)

set(E ${CMAKE_CURRENT_SOURCE_DIR}/..)
function(tonestack_plugin name src entry outname bundle version)
  add_library(${name}-impl STATIC ${E}/clap/${src})
  target_include_directories(${name}-impl PRIVATE ${E}/src ${E}/gen ${E}/third_party)
  target_compile_definitions(${name}-impl PRIVATE TONESTACK_CLAP_FIRST=1)
  target_compile_options(${name}-impl PRIVATE -O3 -fmacro-prefix-map=${E}/third_party/=)
  target_link_libraries(${name}-impl PUBLIC PkgConfig::GUI clap)   # clap: Header für die Einstiegsdatei
  make_clapfirst_plugins(
    TARGET_NAME ${name}
    IMPL_TARGET ${name}-impl
    OUTPUT_NAME "${outname}"
    ENTRY_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${entry}
    BUNDLE_IDENTIFIER "${bundle}"
    BUNDLE_VERSION "${version}"
    COPY_AFTER_BUILD FALSE
    PLUGIN_FORMATS CLAP VST3   # CLAP verlangt der Wrapper hier; ausgeliefert wird die CLAP aus engine/build.sh
    ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  # wie die CLAP-Fassung: libstdc++/libgcc statisch, damit nur glibc ≥ 2.38 nötig ist
  target_link_options(${name}_vst3 PRIVATE -static-libgcc -static-libstdc++ -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports.map)
endfunction()

tonestack_plugin(laney  plugin.cpp  laney_entry.cpp  Laney_AOR50   de.tonestack.laney-aor50.vst3   3.10)
tonestack_plugin(yamaha yplugin.cpp yamaha_entry.cpp Yamaha_B100II de.tonestack.yamaha-b100ii.vst3 1.3)
