#include <windows.h>
#include <stdio.h>

// Co thay doi CreateFile -> CreateFileW phu hop voi LPCWSTR o ham ReadSector

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

void print_VBR_Info(PNTFS_BOOTSECTOR bootsector) {
    printf("\n\n VOLUME BOOT RECORD INFORMATION\n\n");

    printf("[.] JMP instruction ............... %02X %02X %02X\n", bootsector->JMP[0], bootsector->JMP[1], bootsector->JMP[2]);
    printf("[.] OEM ID ........................ "); puts((char*)bootsector->OEM_NAME);
    printf("[.] Bytes per Sector .............. %ld\n", bootsector->BytesPerSector);
    printf("[.] Sectors per Cluster ........... %d\n", bootsector->SectorsPerCluster);
    printf("[.] Media description ............. %d\n", bootsector->MEDIA_DESCRIPTOR);
    printf("[.] Sectors per Track ............. %ld\n", bootsector->SectorsPerTrack);
    printf("[.] Number of heads ............... %ld\n", bootsector->NumberOfHeads);
    printf("[.] Hidden sectors ................ %ld\n", bootsector->HIDDEN_SECTORS);
    printf("[.] Signature ..................... %ld\n", bootsector->HIDDEN_SECTORS);
    printf("[.] Total Sectors ................. %lld\n", bootsector->TOTAL_SECTORS);
    printf("[.] MFT cluster number ............ %lld\n", bootsector->MFT_CLUSTER_NUMBER);
    printf("[.] MFTMirr cluster number ........ %lld\n", bootsector->MFTMirr_CLUSTER_NUMBER);
    printf("[.] Cluster/File record segment ... %ld\n", bootsector->ClusterPerFileRecordSegment);
    printf("[.] Cluster/File index block ...... %ld\n", bootsector->ClusterPerFileIndexBlock);
    printf("[.] Volume serial number .......... %lld\n", bootsector->VOLUME_SERIAL_NUMBER);
    printf("[.] Checksum ...................... %ld\n", bootsector->CHECKSUM);
}

int ReadSector(LPCWSTR drive, DWORD64 readPoint, BYTE* sector, int sectorSize)
{
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

    if (device == INVALID_HANDLE_VALUE) // Open Error
    {
        printf("CreateFile: %u\n", GetLastError());
        return 1;
    }


    LARGE_INTEGER distance{};
    distance.QuadPart = readPoint;
    SetFilePointerEx(device, distance, NULL, FILE_BEGIN); // Set a Point to Read

    if (!ReadFile(device, sector, sectorSize, &bytesRead, NULL))
    {
        printf("ReadFile: %u\n", GetLastError());
        retCode = 2;
    }
    else
    {
        printf("Success!\n");
        retCode = 0;
    }

    CloseHandle(device); // Close the device handle
    return retCode;
}

void printSector(BYTE* sector, int size) {

    for (int row = 0; row < size / 16; row++) {
        printf("%04X | ", 0x6000 + row * 16);

        for (int col = 0; col < 16; col++) {
            int offset = row * 16 + col;
            if (offset < size) {
                printf("%02X ", sector[offset]);
                if ((col + 1) % 4 == 0) {
                    printf("|");
                }
            }
        }
        printf("\n");
        if (row == 31) {
            printf("\n");
        }
    }
}

int main()
{
    // INPUT DRIVE
    wchar_t volume[2] = L"";
    printf("Input volume: ");
    wscanf_s(L"%s", &volume, 2);
    wchar_t Drive[10] = L"";
    wcscat_s(Drive, 10, L"\\\\.\\");
    wcscat_s(Drive, 10, volume);
    wcscat_s(Drive, 10, L":");

    // READ BOOT SECTOR
    BYTE sector[512];
    ReadSector(Drive, 0, sector, 512);
    PNTFS_BOOTSECTOR bootSector = (PNTFS_BOOTSECTOR)sector; // tao Boot sector tu array sector
    print_VBR_Info(bootSector);

    // CALC MFT CLUSTER NUMBER
    DWORD64 startMFT = bootSector->BytesPerSector * bootSector->SectorsPerCluster * bootSector->MFT_CLUSTER_NUMBER;
    // CALC RECORD SIZE IN MFT
    DWORD sizeMFT = 1 << (256 - bootSector->ClusterPerFileRecordSegment);

    // READ MFT ENTRY #0
    printf("\n\n\t\tMFT ENTRY 0\n\n");
    BYTE* MFT_0 = new BYTE[sizeMFT];
    ReadSector(Drive, startMFT, MFT_0, sizeMFT);
    printSector(MFT_0, sizeMFT);

    return 0;
}