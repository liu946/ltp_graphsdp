add_subdirectory (boost)
add_subdirectory (maxent)
add_subdirectory (tinyxml)
if (UNIX)
add_subdirectory (gtest)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cnn)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/eigen-3.2.4)

set(WITH_EIGEN_BACKEND 1)
add_subdirectory(cnn/cnn)
