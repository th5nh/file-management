#include <windows.h>
#include <stdio.h>
#include <string>
#include <vector>

#pragma pack(push, 1)
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
    BYTE filenameLength;
    WORD* filename;
};

void print_VBR_Info(PNTFS_BOOTSECTOR bootsector) {
    printf("\n(1). VOLUME BOOT RECORD INFORMATION\n\n");

    printf("\t# JMP instruction ............... %02X %02X %02X\n", bootsector->JMP[0], bootsector->JMP[1], bootsector->JMP[2]);
    printf("\t# OEM ID ........................ "); puts((char*)bootsector->OEM_NAME);
    printf("\t# Bytes per Sector .............. %ld\n", bootsector->BytesPerSector);
    printf("\t# Sectors per Cluster ........... %d\n", bootsector->SectorsPerCluster);
    printf("\t# Media description ............. %d\n", bootsector->MEDIA_DESCRIPTOR);
    printf("\t# Sectors per Track ............. %ld\n", bootsector->SectorsPerTrack);
    printf("\t# Number of heads ............... %ld\n", bootsector->NumberOfHeads);
    printf("\t# Hidden sectors ................ %ld\n", bootsector->HIDDEN_SECTORS);
    printf("\t# Signature ..................... %ld\n", bootsector->HIDDEN_SECTORS);
    printf("\t# Total Sectors ................. %lld\n", bootsector->TOTAL_SECTORS);
    printf("\t# MFT cluster number ............ %lld\n", bootsector->MFT_CLUSTER_NUMBER);
    printf("\t# MFTMirr cluster number ........ %lld\n", bootsector->MFTMirr_CLUSTER_NUMBER);
    printf("\t# Cluster/File record segment ... %ld\n", bootsector->ClusterPerFileRecordSegment);
    printf("\t# Cluster/File index block ...... %ld\n", bootsector->ClusterPerFileIndexBlock);
    printf("\t# Volume serial number .......... %lld\n", bootsector->VOLUME_SERIAL_NUMBER);
    printf("\t# Checksum ...................... %ld\n", bootsector->CHECKSUM);
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
        printf("CreateFile: %u\n", GetLastError());
        return 1;
    }

    LARGE_INTEGER distance{};
    distance.QuadPart = readPoint;
    SetFilePointerEx(device, distance, NULL, FILE_BEGIN); // Set a Point to Read

    if (!ReadFile(device, sector, sectorSize, &bytesRead, NULL)) {
        printf("ReadFile: %u\n", GetLastError());
        retCode = 2;
    }
    else {
        retCode = 0;
    }

    CloseHandle(device); // Close the device handle
    return retCode;
}

void printSector(BYTE* sector, int size) {
    printf("\t         00 01 02 03  04 05 06 07  08 09 0A 0B  0C 0D 0E 0F\n\n");
    for (int row = 0; row < size / 16; row++) {
        printf("\t%06X | ", 0x0 + row * 16);

        for (int col = 0; col < 16; col++) {
            int offset = row * 16 + col;
            if (offset < size) {
                printf("%02X ", sector[offset]);
                if ((col + 1) % 4 == 0) {
                    printf(" ");
                }
            }
        }
        printf("\n");
        if (row == 31) {
            printf("\n");
        }
    }
}

std::vector<MFT> getMFTs(LPCWSTR Drive, DWORD64 startMFT, DWORD sizeMFT) {
    std::vector<MFT> MFTs; // Find MFTs we need
    MFT e;
    for (int i = 0; i < 255; i++) {
        e.start = startMFT;
        BYTE* _MFT = new BYTE[sizeMFT];
        ReadSector(Drive, startMFT, _MFT, sizeMFT);
        e.type = _MFT[0x16] + _MFT[0x17] * 16 * 16;
        DWORD64 attri = _MFT[0x14] + _MFT[0x15] * 16 * 16;

        if (_MFT[0] == 0x46 && _MFT[1] == 0x49 && _MFT[2] == 0x4C && _MFT[3] == 0x45) {
            while (1) {
                DWORD64 attriLength = _MFT[attri + 4] + _MFT[attri + 5] * 256 + _MFT[attri + 6] * 256 * 256 + _MFT[attri + 7] * 256 * 256 * 256;
                if (attriLength == 0) break;
                if (_MFT[attri] != 0x30) {
                    if (_MFT[attri] == 0x00) break;
                    attri += attriLength;
                    continue;
                }
                else {
                    DWORD64 content = attri + 24;
                    e.parent = _MFT[content] + _MFT[content + 1] * 256 + _MFT[content + 2] * 256 * 256 + _MFT[content + 3] * 256 * 256 * 256;
                    e.flag = _MFT[content + 56];
                    e.filenameLength = _MFT[content + 64];
                    WORD* arr = new WORD[e.filenameLength];
                    for (int i = 0; i < (long long)(e.filenameLength * 2); i += 2) {
                        WORD a = _MFT[content + 66 + i] + _MFT[content + 66 + i + 1] * 256;
                        arr[i / 2] = a;
                    }
                    e.filename = arr;
                    break;
                }
            }
            MFTs.push_back(e);
        }
        startMFT += 1024;
    }
    return MFTs;
}

void printRootDirectory(std::vector<MFT> MFTs) {
    for (int i = 0; i < MFTs.size(); i++) {
        if (MFTs[i].flag != 6 && MFTs[i].parent == 5) {
            for (int j = 0; j < MFTs[i].filenameLength; j++) {
                printf("%c", MFTs[i].filename[j]);
            }
            printf("\n");
        }
    }
}

int main() {
    // INPUT DRIVE
    printf("Input volume: ");
    wchar_t volume[2] = L""; wscanf_s(L"%s", &volume, 2);
    wchar_t Drive[10] = L"\\\\.\\";
    wcscat_s(Drive, 10, volume); wcscat_s(Drive, 10, L":");

    // READ BOOT SECTOR
    BYTE sector[512];
    ReadSector(Drive, 0, sector, 512);
    PNTFS_BOOTSECTOR bootSector = (PNTFS_BOOTSECTOR)sector; // tao Boot sector tu array sector
    print_VBR_Info(bootSector);

    // CALCULATE MFT CLUSTER NUMBER
    DWORD64 startMFT = bootSector->BytesPerSector * bootSector->SectorsPerCluster * bootSector->MFT_CLUSTER_NUMBER;
    // CALCULATE RECORD SIZE IN MFT
    DWORD sizeMFT = 1 << abs((long long) 256 - bootSector->ClusterPerFileRecordSegment);
    
    // READ needed MFT ENTRY #0
    printf("\n(2). ROOT DIRECTORY\n\n");

    std::vector<MFT> MFTs = getMFTs(Drive, startMFT, sizeMFT);
    printRootDirectory(MFTs);

    return 0;
}
