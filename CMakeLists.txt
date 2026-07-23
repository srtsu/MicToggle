cmake_minimum_required(VERSION 3.20)
project(mic_toggle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MIC_TOGGLE_VERSION "0.0.0-dev" CACHE STRING
    "Version embedded into the Windows executable")
string(REGEX REPLACE "^v" "" MIC_TOGGLE_VERSION_NORMALIZED
       "${MIC_TOGGLE_VERSION}")

if(NOT MIC_TOGGLE_VERSION_NORMALIZED MATCHES
   "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(\\.([0-9]+))?([+-][0-9A-Za-z.-]+)?$")
  message(FATAL_ERROR
    "MIC_TOGGLE_VERSION must be a version such as 1.2.3 or 1.2.3-beta.1")
endif()

set(MIC_TOGGLE_VERSION_MAJOR "${CMAKE_MATCH_1}")
set(MIC_TOGGLE_VERSION_MINOR "${CMAKE_MATCH_2}")
set(MIC_TOGGLE_VERSION_PATCH "${CMAKE_MATCH_3}")
if(CMAKE_MATCH_5)
  set(MIC_TOGGLE_VERSION_BUILD "${CMAKE_MATCH_5}")
else()
  set(MIC_TOGGLE_VERSION_BUILD 0)
endif()

configure_file(
  res/version.rc.in
  "${CMAKE_CURRENT_BINARY_DIR}/version.rc"
  @ONLY
)

add_executable(mic_toggle WIN32
  src/main.cpp
  src/audio_endpoint.cpp
  src/wav_resource.cpp
  res/resources.rc
  "${CMAKE_CURRENT_BINARY_DIR}/version.rc"
)

target_include_directories(mic_toggle PRIVATE src res)

target_compile_definitions(mic_toggle PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

if(WIN32)
  target_link_libraries(mic_toggle PRIVATE ole32 winmm uuid)
endif()

if(MINGW)
  target_compile_options(mic_toggle PRIVATE
    $<$<CONFIG:Release>:-ffunction-sections -fdata-sections>
  )
  target_link_options(mic_toggle PRIVATE
    -mwindows
    -municode
    $<$<CONFIG:Release>:-Wl,--gc-sections -s>
  )
endif()

if(MSVC)
  target_compile_options(mic_toggle PRIVATE
    $<$<CONFIG:Release>:/O1 /Oi /GL /Gy /Gw /GF /GR- /EHs-c- /Zc:inline /DNDEBUG>
    $<$<CONFIG:RelWithDebInfo>:/O2 /Oi /GL>
  )
  target_link_options(mic_toggle PRIVATE
    $<$<CONFIG:Release>:/LTCG /INCREMENTAL:NO /OPT:REF /OPT:ICF /RELEASE>
    $<$<CONFIG:RelWithDebInfo>:/LTCG>
  )
else()
  target_compile_options(mic_toggle PRIVATE
    -fno-exceptions
    -fno-rtti
    $<$<CONFIG:Release>:-Os -DNDEBUG>
    $<$<CONFIG:RelWithDebInfo>:-O2>
  )

  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
  if(ipo_supported)
    set_property(TARGET mic_toggle PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
  endif()
endif()
