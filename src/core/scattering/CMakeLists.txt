add_library(scattering STATIC
   integration.cc
   scattering_species.cc
   properties.cc
   psd.cc
   sht.cc
   phase_matrix.cc
)

target_include_directories(scattering PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(scattering PUBLIC auto_partfun matpack artscore)

if (ENABLE_ARTS_LGPL OR CMAKE_CXX_COMPILER_ID MATCHES MSVC)
target_compile_definitions(scattering PUBLIC ARTS_NO_SHTNS)
else()
target_include_directories(scattering PRIVATE "${FFTW_INCLUDE_DIR}" "${CMAKE_SOURCE_DIR}/3rdparty/shtns")
target_link_libraries(scattering PRIVATE fftw3 shtns)
endif()