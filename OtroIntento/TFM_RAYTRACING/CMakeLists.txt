cmake_minimum_required(VERSION 3.20)

# Nombre de la solución
project(TFM_RAYTRACING)

# Configuración de Vulkan y GLFW
set(CMAKE_PREFIX_PATH "C:/Users/carlos/source/repos/glfw")
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

# Directorios de código fuente
set(SRC_DIR "src")
set(LIB_DIR "lib")

# Crear la librería estática
add_library(Vulkanlib STATIC ${LIB_DIR}/mylibrary.cpp ${LIB_DIR}/mylibrary.h)

target_include_directories(Vulkanlib PUBLIC ${LIB_DIR})
target_link_libraries(Vulkanlib Vulkan::Vulkan)

# Crear el ejecutable
add_executable(VulkanProject ${SRC_DIR}/main.cpp)

target_include_directories(VulkanProject PUBLIC ${LIB_DIR})
target_link_libraries(VulkanProject Vulkanlib Vulkan::Vulkan glfw)
# Configuración de salida
set_target_properties(VulkanProject PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(Vulkanlib PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

