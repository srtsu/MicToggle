cmake_minimum_required(VERSION 3.20)
project(mic_toggle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(mic_toggle
  src/main.cpp
  src/audio_endpoint.cpp
  src/wav_resource.cpp
  res/resources.rc
)

target_include_directories(mic_toggle PRIVATE src res)

target_compile_definitions(mic_toggle PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

if(WIN32)
  target_link_libraries(mic_toggle PRIVATE ole32 winmm uuid)
endif()

if(MINGW)
  target_link_options(mic_toggle PRIVATE -mwindows -municode)
endif()

if(MSVC)
  target_compile_options(mic_toggle PRIVATE
    $<$<CONFIG:Release>:/O2 /Oi /GL /DNDEBUG>
    $<$<CONFIG:RelWithDebInfo>:/O2 /Oi /GL>
  )
  target_link_options(mic_toggle PRIVATE
    $<$<CONFIG:Release>:/LTCG /INCREMENTAL:NO>
    $<$<CONFIG:RelWithDebInfo>:/LTCG>
  )
else()
  target_compile_options(mic_toggle PRIVATE
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
    $<$<CONFIG:RelWithDebInfo>:-O3>
  )
  
  if(NOT (MINGW AND CMAKE_CROSSCOMPILING))
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
      set_property(TARGET mic_toggle PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    endif()
  endif()
endif()
