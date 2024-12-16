# Requiere CMake 3.15 o superior
cmake_minimum_required(VERSION 3.15)

# Nombre del proyecto
project(WebSocketServer LANGUAGES CXX)

# Establecer versión de C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ruta al toolchain de vcpkg
set(CMAKE_TOOLCHAIN_FILE "C:/Users/am951/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

# Encontrar bibliotecas necesarias
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(jsoncpp CONFIG REQUIRED)

# Crear el ejecutable
add_executable(server main.cpp)

# Enlazar bibliotecas (usa solo el estilo con palabras clave: PUBLIC, PRIVATE o INTERFACE)
target_link_libraries(server PRIVATE Boost::system Boost::thread jsoncpp_lib)

# Configuración específica para Windows
if(WIN32)
    target_link_libraries(server PRIVATE ws2_32 mswsock)
endif()
