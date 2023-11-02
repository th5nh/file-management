#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>

using namespace std;

std::wstring HexaToUnicode(BYTE* hexa, int size) {

    unsigned char* utf16Bytes = new unsigned char[size];

    for (int i = 0; i < size; i++) {
        utf16Bytes[i] = hexa[i];
    }

    std::wstring fileName(reinterpret_cast<const wchar_t*>(utf16Bytes), size / 2);
    delete[] utf16Bytes;

    return fileName;
}

int main() {
    unsigned char hexa[] = {0x54, 0x75, 0x79, 0xE1, 0xBB, 0x87, 0x74, 0x20, 0x76, 0xE1, 0xBB, 0x9D, 0x69};
    std::wstring utf16String = HexaToUnicode(hexa, sizeof(hexa));
    
    wprintf(L"UTF-16 String: %ls\n", utf16String.c_str());

    return 0;
}
