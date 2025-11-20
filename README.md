
0. Перейдите в папку проекта / cd TaskManagerUltraProMax
1. Для MSVC (Visual Studio). MSVC:
     vcpkg install cryptopp:x64-windows-static httplib:x64-windows-static nlohmann-json:x64-windows-static

   Для GCC (MinGW). GCC:
     vcpkg install cryptopp:x64-mingw-static httplib:x64-mingw-static nlohmann-json:x64-mingw-static

2. Установка переменной окружения `VCPKG_ROOT` (Windows PowerShell запустить от админа)
    setx VCPKG_ROOT "C:\path\to\vcpkg" /M

3. Для MSVC (Visual Studio). MSVC:
    cmake --preset=x64-windows-static
    cmake --build --preset=x64-windows-static-release

   Для GCC (MinGW). GCC:
    cmake --preset=x64-mingw-static
    cmake --build --preset=x64-mingw-static-release
