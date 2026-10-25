#include <Windows.h>

#include <iostream>
#include <string>

// CMake探索テストのvswhere代役。ビルドやインストールは実行しない。
int main()
{
    const DWORD size = GetEnvironmentVariableA("SLIMES_TEST_VS_PATH", nullptr, 0);
    if (size == 0) {
        return 1;
    }
    std::string installation(size, '\0');
    const DWORD length = GetEnvironmentVariableA("SLIMES_TEST_VS_PATH", installation.data(), size);
    if (length == 0 || length >= size) {
        return 1;
    }
    installation.resize(length);
    std::cout << installation << '\n';
    return 0;
}
