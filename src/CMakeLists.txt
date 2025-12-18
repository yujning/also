include_directories(commands)

file(GLOB_RECURSE FILENAMES *.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(also also.cpp ${FILENAMES})

# STP bi-decomposition resynthesis headers live under lib/stp/src/include
# Add the include directory so the also executable can find them when the
# lut_resyn command is built with STP support.
target_include_directories(also PRIVATE 
  ${CMAKE_SOURCE_DIR}/lib/stp/src/include
)

# Link against STP algorithms library to get other dependencies
if(ALSO_HAS_STP)
  message(STATUS "STP support is enabled, adding Eigen3 includes")
  # Eigen3 is required for STP. Try to find it; if not found, use standard location.
  # Users can override by setting CMAKE_PREFIX_PATH or Eigen3_DIR.
  find_package(Eigen3 QUIET NO_MODULE)
  if(TARGET Eigen3::Eigen)
    # Try to get include dirs from target
    get_target_property(_eigen_includes Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)
    if(_eigen_includes)
      target_include_directories(also PRIVATE ${_eigen_includes})
      message(STATUS "Using Eigen3 from target: ${_eigen_includes}")
    else()
      # Fallback: check standard locations
      foreach(_eigen_path "/usr/include/eigen3" "/usr/local/include/eigen3" "${CMAKE_INSTALL_PREFIX}/include/eigen3")
        if(EXISTS "${_eigen_path}")
          target_include_directories(also PRIVATE ${_eigen_path})
          message(STATUS "Adding Eigen3 fallback location: ${_eigen_path}")
          break()
        endif()
      endforeach()
    endif()
  else()
    # Fallback: check standard locations  
    foreach(_eigen_path "/usr/include/eigen3" "/usr/local/include/eigen3" "${CMAKE_INSTALL_PREFIX}/include/eigen3")
      if(EXISTS "${_eigen_path}")
        target_include_directories(also PRIVATE ${_eigen_path})
        message(STATUS "Adding Eigen3 fallback location: ${_eigen_path}")
        break()
      endif()
    endforeach()
  endif()
  
  target_link_libraries(also alice mockturtle stp_algorithms)
else()
  message(STATUS "STP support is NOT enabled")
  target_link_libraries(also alice mockturtle)
endif()