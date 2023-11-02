

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <locale.h>
#include <io.h>
#include <fcntl.h>

#pragma pack(push, 1)

using namespace std;

typedef struct {
    // Must be in correct order
    BYTE JMP[3];
    BYTE OEM_NAME[8];
    WORD BytesPerSector;
    BYTE SectorsPerCluster;
    WORD RESERVED_SECTORS;
    BYTE always_zero[3];
    WORD unused1;
    BYTE MEDIA_DESCRIPTOR;
    WORD unused2;
    WORD SectorsPerTrack;
    WORD NumberOfHeads;
    DWORD HIDDEN_SECTORS;
    DWORD unused3;
    DWORD SIGNATURE;
    DWORD64 TOTAL_SECTORS;
    DWORD64 MFT_CLUSTER_NUMBER;
    DWORD64 MFTMirr_CLUSTER_NUMBER;
    DWORD ClusterPerFileRecordSegment;
    DWORD ClusterPerFileIndexBlock;
    DWORD64 VOLUME_SERIAL_NUMBER;
    DWORD CHECKSUM;
} NTFS_BOOTSECTOR, * PNTFS_BOOTSECTOR;
#pragma pack(pop)

struct MFT {
    DWORD64 start;
    DWORD64 type;
    DWORD64 parent;
    DWORD flag;
    WORD filenameLength;
    WORD* filename;
};

void convertToVN(WORD* arr, int len) {
    for (int i = 0; i < len; i++) {
        wprintf(L"%c", arr[i]);
    }
}

void print_VBR_Info(PNTFS_BOOTSECTOR bootsector) {
    wprintf(L"\n(1). VOLUME BOOT RECORD INFORMATION\n\n");

    wprintf(L"\t# JMP instruction ............... %02X %02X %02X\n", bootsector->JMP[0], bootsector->JMP[1], bootsector->JMP[2]);
    wprintf(L"\t# OEM ID ........................ "); 
    for (int i = 0; i < 8; i++) {
        wprintf(L"%c", bootsector->OEM_NAME[i]);
    }
    wprintf(L"\n\t# Bytes per Sector .............. %ld\n", bootsector->BytesPerSector);
    wprintf(L"\t# Sectors per Cluster ........... %d\n", bootsector->SectorsPerCluster);
    wprintf(L"\t# Media description ............. %d\n", bootsector->MEDIA_DESCRIPTOR);
    wprintf(L"\t# Sectors per Track ............. %ld\n", bootsector->SectorsPerTrack);
    wprintf(L"\t# Number of heads ............... %ld\n", bootsector->NumberOfHeads);
    wprintf(L"\t# Hidden sectors ................ %ld\n", bootsector->HIDDEN_SECTORS);
    wprintf(L"\t# Signature ..................... %ld\n", bootsector->HIDDEN_SECTORS);
    wprintf(L"\t# Total Sectors ................. %lld\n", bootsector->TOTAL_SECTORS);
    wprintf(L"\t# MFT cluster number ............ %lld\n", bootsector->MFT_CLUSTER_NUMBER);
    wprintf(L"\t# MFTMirr cluster number ........ %lld\n", bootsector->MFTMirr_CLUSTER_NUMBER);
    wprintf(L"\t# Cluster/File record segment ... %ld\n", bootsector->ClusterPerFileRecordSegment);
    wprintf(L"\t# Cluster/File index block ...... %ld\n", bootsector->ClusterPerFileIndexBlock);
    wprintf(L"\t# Volume serial number .......... %lld\n", bootsector->VOLUME_SERIAL_NUMBER);
    wprintf(L"\t# Checksum ...................... %ld\n", bootsector->CHECKSUM);
}

int ReadSector(LPCWSTR drive, DWORD64 readPoint, BYTE* sector, int sectorSize) {
    int retCode = 0;
    DWORD bytesRead;
    HANDLE device = NULL;

    device = CreateFileW(drive,    // Drive to open
        GENERIC_READ,           // Access mode
        FILE_SHARE_READ | FILE_SHARE_WRITE,        // Share Mode
        NULL,                   // Security Descriptor
        OPEN_EXISTING,          // How to create
        0,                      // File attributes
        NULL);                  // Handle to template

    if (device == INVALID_HANDLE_VALUE) { // Open Error 
        wprintf(L"CreateFile: %u\n", GetLastError());
        return 1;
    }

    LARGE_INTEGER distance{};
    distance.QuadPart = readPoint;
    SetFilePointerEx(device, distance, NULL, FILE_BEGIN); // Set a Point to Read

    if (!ReadFile(device, sector, sectorSize, &bytesRead, NULL)) {
        wprintf(L"ReadFile: %u\n", GetLastError());
        retCode = 2;
    }
    else {
        retCode = 0;
    }

    CloseHandle(device); // Close the device handle
    return retCode;
}

void printSector(BYTE* sector, int size) {
    wprintf(L"\t         00 01 02 03  04 05 06 07  08 09 0A 0B  0C 0D 0E 0F\n\n");
    for (int row = 0; row < size / 16; row++) {
        wprintf(L"\t%06X | ", 0x0 + row * 16);

        for (int col = 0; col < 16; col++) {
            int offset = row * 16 + col;
            if (offset < size) {
                wprintf(L"%02X ", sector[offset]);
                if ((col + 1) % 4 == 0) {
                    wprintf(L" ");
                }
            }
        }
        wprintf(L"\n");
        if (row == 31) {
            wprintf(L"\n");
        }
    }
}

std::vector<MFT> getMFTs(LPCWSTR Drive, DWORD64 startMFT, DWORD sizeMFT) {
    DWORD64 start = startMFT;
    std::vector<MFT> MFTs; // Find MFTs we need
    MFT e;
    for (int i = 0; i < 255; i++) {
        e.start = (start - startMFT) / sizeMFT;
        BYTE* _MFT = new BYTE[sizeMFT];
        ReadSector(Drive, start, _MFT, sizeMFT);
        e.type = _MFT[0x16] + _MFT[0x17] * 16 * 16;
        DWORD64 attri = _MFT[0x14] + _MFT[0x15] * 16 * 16;

        if (_MFT[0] == 0x46 && _MFT[1] == 0x49 && _MFT[2] == 0x4C && _MFT[3] == 0x45) {
            while (attri > 0 && attri < 1024) {
                DWORD64 attriLength = _MFT[attri + 4] + _MFT[attri + 5] * 256 + _MFT[attri + 6] * 256 * 256 + _MFT[attri + 7] * 256 * 256 * 256;
                if (attriLength == 0) break;
                if (_MFT[attri] != 0x30) {
                    if (_MFT[attri] == 0x00) break;
                    attri += attriLength;
                    continue;
                }
                else {
                    DWORD64 contentStart = _MFT[attri + 20] + _MFT[attri + 21] * 256;
                    DWORD64 content = attri + contentStart;
                    e.parent = _MFT[content] + _MFT[content + 1] * 256 + _MFT[content + 2] * 256 * 256 + _MFT[content + 3] * 256 * 256 * 256;
                    e.flag = _MFT[content + 56];
                    e.filenameLength = _MFT[content + 64];
                    WORD* arr = new WORD[e.filenameLength];
                    for (int j = 0; j < e.filenameLength * 2; j += 2) {
                        WORD a = _MFT[content + 66 + j] + _MFT[content + 66 + j + 1] * 256;
                        arr[j / 2] = a;
                    }
                    e.filename = arr;
                    break;
                }
            }
            MFTs.push_back(e);
        }
        start += 1024;
    }
    return MFTs;
}

void printRootDirectory(std::vector<MFT> MFTs, DWORD64 root, int numtab) {
    for (int i = 0; i < MFTs.size(); i++) {

        if (MFTs[i].flag != 6 && MFTs[i].parent == root) {
            for (int t = 0; t < numtab; t++) {
                wprintf(L"    |");
            }
            convertToVN(MFTs[i].filename, MFTs[i].filenameLength);
            wprintf(L"\n");
            if (MFTs[i].type == 3) {
                printRootDirectory(MFTs, MFTs[i].start, numtab + 1);
            }
        }
    }
}

void printDirectory(std::vector<MFT> MFTs, DWORD64 root) {
    for (int i = 0; i < MFTs.size(); i++) {
        if (MFTs[i].flag != 6 && MFTs[i].parent == root) {
            wprintf(L"  ");
            for (int j = 0; j < MFTs[i].filenameLength; j++) {
                wprintf(L"%c", MFTs[i].filename[j]);
            }
            wprintf(L"\n");
        }
    }
}

bool compare(WORD* a, WORD* b, int len) {
    bool check = true;
    for (int i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            check = false;
        }
    }
    return check;
}

string file_extension(const string& file) {
    size_t dot_pos = file.find_last_of(".");
    if (dot_pos != string::npos) {
        return file.substr(dot_pos);
    }
    return "";
}



void printText(LPCWSTR DRIVE, DWORD64 startMFTByte, int sizeRecord,DWORD pos)
{
    BYTE* sector = new BYTE[sizeRecord];
    ReadSector(DRIVE, startMFTByte + pos * sizeRecord, sector, sizeRecord);
    DWORD offset = *((WORD*)&sector[20]);
    int x = 1;
    while (x)
    {
        //Tim attribute filename
        if (*((DWORD*)&sector[offset]) != 128)
        {
            offset += *((DWORD*)&sector[offset + 4]);
            continue;
        }
        //Bo qua header
        WORD header = *((WORD*)&sector[offset + 32]);
        offset += header;
        x = 0;
    }
    BYTE count = *((BYTE*)&sector[offset + 1]);
    WORD begin = *((WORD*)&sector[offset + 2]);
    BYTE next;
    offset += 5;
    BYTE ptr;
    while (count != 0)
    {
        for (int i = 0; i < count; i++)
        {
            BYTE* readSec = new BYTE[512];
            ReadSector(DRIVE, begin * 512, readSec, 512);
            for (int j = 0; j < 512; j++)
                wprintf(L"%c",readSec[j]);
            begin++;
        }

        count = *((BYTE*)&sector[offset + 1]);
        next = *((BYTE*)&sector[offset + 2]);
        begin += next;

        offset += 3;
    }
    return;
}
void readFile(LPCWSTR DRIVE, DWORD64 startMFTByte, int sizeRecord, MFT MFTs, DWORD pos)
{
    
    //File f = readFileName(sector, sizeRecord);
    wstring wstr(MFTs.filenameLength, L'\0');

    // Chuyển đổi từng ký tự trong chuỗi
    for (size_t i = 0; i < MFTs.filenameLength; i++) {
        wstr[i] = static_cast<wchar_t>(MFTs.filename[i]);
    }


    string str_name = string(wstr.begin(), wstr.end());
    string fex = file_extension(str_name);
    if (fex == ".txt")
    {
        printText(DRIVE, startMFTByte, sizeRecord, pos);
        return;
    }

    if (fex == ".jpg" || fex == ".png")
    {
        wcout << "Hay mo file bang Paint!" << endl;
    }
    else if (fex == ".doc")
    {
        wcout << "Hay mo file bang MS Word!" << endl;
    }
    else if (fex == ".pptx")
    {
        wcout << "Hay mo file bang MS PowerPoint!" << endl;
    }
    else if (fex == ".cpp")
    {
        wcout << "Hay mo file bang Visual Studio!" << endl;
    }
    else
    {
        wcout << "Dinh dang file chua biet!" << endl;
    }
}


int main() {
    _setmode(_fileno(stdout), _O_U16TEXT);
    // INPUT DRIVE
    wprintf(L"Input volume: ");
    wchar_t volume[2] = L""; wscanf_s(L"%s", &volume, 2);
    wchar_t Drive[10] = L"\\\\.\\";
    wcscat_s(Drive, 10, volume); wcscat_s(Drive, 10, L":");

    // READ BOOT SECTOR
    BYTE sector[512];
    ReadSector(Drive, 0, sector, 512);
    PNTFS_BOOTSECTOR bootSector = (PNTFS_BOOTSECTOR)sector; // tao Boot sector tu array sector
    print_VBR_Info(bootSector);

    // CALCULATE MFT CLUSTER NUMBER
    //DWORD sizeMFT = 1 << abs(static_cast<int>(static_cast<signed char>(bootSector->ClusterPerFileRecordSegment)));
    DWORD64 startMFT = bootSector->BytesPerSector * bootSector->SectorsPerCluster * bootSector->MFT_CLUSTER_NUMBER;
    // CALCULATE RECORD SIZE IN MFT
    DWORD sizeMFT = bootSector->BytesPerSector * bootSector->ClusterPerFileRecordSegment * bootSector->SectorsPerCluster;

    // READ needed MFT ENTRY #0
    wprintf(L"\n(2). ROOT DIRECTORY\n\n");

    std::vector<MFT> MFTs = getMFTs(Drive, startMFT, sizeMFT);
    int root = 5;
    printRootDirectory(MFTs, 5, 1);

    while (1) {
        wprintf(L"\n\nTHU MUC HIEN TAI\n\n");
        printDirectory(MFTs, root);
        int choice = 0;
        std::wcout << "\n\n0. Thoat\n";
        std::wcout << "1. Hien thi cay thu muc\n";
        std::wcout << "2. Di den thu muc / tap tin\n";
        std::wcout << "> Nhap lua chon: ";
        std::cin >> choice; std::cin.ignore();
        wprintf(L"\n--------------------------------------------------------------\n");
        if (choice == 0) {
            return 1;
        }
        else if (choice == 1) {
            printRootDirectory(MFTs, root, 1);
        }
        else {
            printDirectory(MFTs, root);
            std::string tmp = "";
            std::wcout << "\n> Nhap ten thu muc / file: ";
            getline(std::cin, tmp);

            WORD* a = new WORD[tmp.length()];
            for (int i = 0; i < tmp.length(); i++) {
                a[i] = (WORD)tmp[i];
            }

            for (int i = 0; i < MFTs.size(); i++) {
                if (compare(a, MFTs[i].filename, tmp.length())) {
                    
                    root = MFTs[i].start;
                    if (MFTs[i].type == 1)
                    {
                        root = MFTs[i].parent;
                        readFile(Drive, startMFT, sizeMFT,MFTs[i], MFTs[i].start);
                    }
                    break;
                }
            }
        }


    }




    return 0;
}
