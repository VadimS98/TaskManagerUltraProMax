cmake_minimum_required(VERSION 3.20)

project(TaskManagerUltraProMax VERSION 1.0.0 LANGUAGES CXX RC)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
endif()

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/external/vcpkg/scripts/buildsystems/vcpkg.cmake" 
        CACHE FILEPATH "vcpkg toolchain file")
endif()

# =============================================================================
# Project Configuration
# =============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# =============================================================================
# Platform-specific Configuration
# =============================================================================

if(WIN32)
    add_compile_definitions(
        WINVER=0x0A00
        _WIN32_WINNT=0x0A00
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
    
    set(CMAKE_WIN32_EXECUTABLE ON)
endif()

# =============================================================================
# Dependencies Management (vcpkg integration)
# =============================================================================

include(FetchContent)

find_package(cryptopp CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# =============================================================================
# Source and Header Files
# =============================================================================

set(HEADER_FILES
    include/ui/MainWindow.hpp
    include/ui/ProcessListView.hpp

    include/config/Config.hpp

    include/core/ModuleEnumerator.hpp
    include/core/PrivilegeManager.hpp
    include/core/ProcessTerminator.hpp
    include/core/ProcessEnumerator.hpp

    include/network/NetworkClient.hpp
    include/network/RequestBuilder.hpp

    include/crypto/CryptoEngine.hpp

    include/utils/UuidGenerator.hpp
    include/utils/WinHandle.hpp
    include/utils/Utils.hpp

    resources/resource.h
)

set(SOURCE_FILES
    src/main.cpp

    src/ui/MainWindow.cpp
    src/ui/ProcessListView.cpp

    src/config/Config.cpp

    src/core/ModuleEnumerator.cpp
    src/core/PrivilegeManager.cpp
    src/core/ProcessTerminator.cpp
    src/core/ProcessEnumerator.cpp

    src/network/NetworkClient.cpp
    src/network/RequestBuilder.cpp

    src/crypto/CryptoEngine.cpp

    src/utils/UuidGenerator.cpp
)

# =============================================================================
# Executable Target
# =============================================================================

add_executable(${PROJECT_NAME} 
    WIN32
    ${SOURCE_FILES}
    ${HEADER_FILES}
    resources/resource.rc
)

# Организация файлов в Visual Studio Solution Explorer
source_group("Header Files\\UI" FILES 
    include/ui/MainWindow.hpp 
    include/ui/ProcessListView.hpp
)
source_group("Header Files\\Core" FILES 
    include/core/ProcessEnumerator.hpp
    include/core/ProcessTerminator.hpp
    include/core/PrivilegeManager.hpp
    include/core/ModuleEnumerator.hpp
)
source_group("Header Files\\Network" FILES 
    include/network/NetworkClient.hpp
    include/network/RequestBuilder.hpp
)
source_group("Header Files\\Crypto" FILES 
    include/crypto/CryptoEngine.hpp
)
source_group("Header Files\\Utils" FILES 
    include/utils/UuidGenerator.hpp
)

source_group("Header Files\\Config" FILES 
    include/config/Config.hpp
)

source_group("Source Files\\UI" FILES 
    src/ui/MainWindow.cpp 
    src/ui/ProcessListView.cpp
)
source_group("Source Files\\Core" FILES 
    src/core/ProcessEnumerator.cpp
    src/core/ProcessTerminator.cpp
    src/core/PrivilegeManager.cpp
    src/core/ModuleEnumerator.cpp
)
source_group("Source Files\\Network" FILES 
    src/network/NetworkClient.cpp
    src/network/RequestBuilder.cpp
)
source_group("Source Files\\Crypto" FILES 
    src/crypto/CryptoEngine.cpp
)
source_group("Source Files\\Utils" FILES 
    src/utils/UuidGenerator.cpp
)

source_group("Source Files\\Config" FILES 
    src/config/Config.cpp
)


# =============================================================================
# Include Directories
# =============================================================================

target_include_directories(${PROJECT_NAME} 
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
)

# =============================================================================
# Link Libraries
# =============================================================================

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    # WSL
    comctl32        # Common Controls (ListView)
    user32          # Window Management
    kernel32        # Process API
    advapi32        # Security/Privileges
    psapi           # Process Status API
    shell32         # ShellExecuteEx для перезапуска с Admin
    ole32           # COM (для CoCreateGuid)
    
    # Third-party Libraries
    cryptopp::cryptopp
    httplib::httplib
    nlohmann_json::nlohmann_json
)

# =============================================================================
# Compiler Options
# =============================================================================

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /utf-8
    )
    
    target_link_options(${PROJECT_NAME} PRIVATE
        /SUBSYSTEM:WINDOWS
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

# =============================================================================
# Postbuild Commands / Copy Config
# =============================================================================

# Копируем конфигурационный файл в output directory

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config/config.json"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/config.json"
    COMMENT "Copying config.json to output directory"
)

# =============================================================================
# Installation & Packaging
# =============================================================================

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(FILES
    resources/config.json
    README.md
    DESTINATION share/${PROJECT_NAME}
)

# =============================================================================
# Status Messages 
# =============================================================================

message(STATUS "=== ProcessManager Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
message(STATUS "Architecture: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "=====================================")
