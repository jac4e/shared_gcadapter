cmake_minimum_required(VERSION 3.0.0)
project(shared_gcadapter VERSION 0.1.0)

# Enable testing
include(CTest)
enable_testing()


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
include(CPack)

# Get a list of all sub-folders in the src directory
file(GLOB SUB_FOLDERS "src/*")

# Find libraries
find_library(LIBUSB_LIBRARY NAMES usb-1.0)

# Create a folder to store the compiled binaries

# Loop through each sub-folder
foreach(SUB_FOLDER ${SUB_FOLDERS})
  # Get the name of the sub-folder
  get_filename_component(SUB_FOLDER_NAME ${SUB_FOLDER} NAME)

  # Define the source files for the sub-folder
  file(GLOB SUB_FOLDER_SRC "${SUB_FOLDER}/*.cpp" "${SUB_FOLDER}/*.c" "${SUB_FOLDER}/*.h")

  # Define the executable target
  add_executable(${SUB_FOLDER_NAME} ${SUB_FOLDER_SRC})

  # Link the libusb library to the target
  target_link_libraries(${SUB_FOLDER_NAME} ${LIBUSB_LIBRARY})
endforeach()
