
0. cd TaskManagerUltraProMax
1. MSVC
vcpkg install cryptopp:x64-windows-static httplib:x64-windows-static nlohmann-json:x64-windows-static

GCC
vcpkg install cryptopp:x64-mingw-static httplib:x64-mingw-static nlohmann-json:x64-mingw-static

2. # Windows PowerShell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"

3. MSVC
cmake --preset=x64-windows-static
cmake --build --preset=x64-windows-static-release

GCC
cmake --preset=x64-mingw-static
cmake --build --preset=x64-mingw-static-release