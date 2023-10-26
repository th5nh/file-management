#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <vector>

using namespace std;

int ReadSector(LPCWSTR drive, long long readPoint, BYTE* sector, int sectorSize)
{
    int retCode = 0;
    DWORD bytesRead;
    HANDLE device = NULL;

    device = CreateFile(drive,    // Drive to open
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

    SetFilePointer(device, readPoint, NULL, FILE_BEGIN); // Set a Point to Read

    if (!ReadFile(device, sector, sectorSize, &bytesRead, NULL))
    {
        printf("ReadFile: %u\n", GetLastError());
        retCode = 2;
    }
    else
    {
        cout << endl << bytesRead << endl;
        printf("Success!\n");
        retCode = 0;
    }

    CloseHandle(device); // Close the device handle
    return retCode;
}

int CalculateMFTStartSector(BYTE bootSector[512])
{
    // Extract the MFT cluster number from the boot sector
    DWORD mftClusterNumber = *((DWORD*)&bootSector[0x30]);

    // Extract SectorsPerCluster from the boot sector (typically at offset 0x0D)
    BYTE sectorsPerCluster = *((BYTE*)&bootSector[0x0D]);
    WORD bytesPerSector = *((WORD*)&bootSector[0x0B]);

    // Calculate the starting sector of the MFT
    int startingSectorOfMFT = mftClusterNumber * sectorsPerCluster * bytesPerSector;
    return startingSectorOfMFT;
}

int converClusterToByte(DWORD cluster, BYTE bootSector[512]) {
    BYTE sectorsPerCluster = *((BYTE*)&bootSector[0x0D]);
    WORD bytesPerSector = *((WORD*)&bootSector[0x0B]);
    // Calculate the starting sector of the MFT
    long long numByte = cluster * sectorsPerCluster * bytesPerSector;
    return numByte;
}

int calculateRecordSizeInMFT(BYTE bootSector[512]) {
    BYTE recordSize = bootSector[0x40];
    char s = static_cast<char>(recordSize);
    int leftShift = static_cast<int>(s);
    return 1 << abs(leftShift);
}

void printSector(BYTE* sector, int size) {
    printf("Boot Sector Data:\n");

    for (int row = 0; row < size / 16; row++) {
        printf("%04X | ", 0x6000 + row * 16);

        for (int col = 0; col < 16; col++) {
            int offset = row * 16 + col;
            if (offset < 512) {
                printf("%02X ", sector[offset]);
                if ((col + 1) % 4 == 0) {
                    printf("|");
                }
            }
        }
        printf("\n");
    }
}

DWORD GetRootDirectoryCluster(LPCWSTR drive)
{
    // Initialize necessary variables
    BYTE mftEntry[1024]; // Assume the MFT entry size is 1024 bytes
    DWORD bytesRead;
    HANDLE device = CreateFile(drive, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (device == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile: %u\n", GetLastError());
        return 0; // Return 0 on error
    }

    // Read the MFT entry for the root directory (cluster 5)
    SetFilePointer(device, 5 * 1024, NULL, FILE_BEGIN); // Assuming cluster size is 1024 bytes
    if (!ReadFile(device, mftEntry, sizeof(mftEntry), &bytesRead, NULL))
    {
        printf("ReadFile: %u\n", GetLastError());
        CloseHandle(device);
        return 0; // Return 0 on error
    }

    CloseHandle(device);

    // Extract the cluster number for the root directory (typically at a specific offset in the MFT entry)
    // This offset depends on the structure of the MFT entry and varies depending on the NTFS version.
    // You should consult NTFS documentation for the exact offset.
    // Assuming it's at offset 0x02 (DWORD little-endian format)
    DWORD rootCluster = *((DWORD*)&mftEntry[0x02]);

    return rootCluster;
}

int main(int argc, char** argv)
{
    BYTE sector[512];
    int result = ReadSector(L"\\\\.\\E:", 0, sector, 512);

    if (result == 0)
    {



        int bytesOfSector = *((WORD*)&sector[0x0B]);
        printSector(sector, 512);
        long long startingSectorOfMFT = CalculateMFTStartSector(sector);
        int sizeOfMFTEntry = calculateRecordSizeInMFT(sector);
        std::vector<BYTE> mft;
        cout << endl << startingSectorOfMFT << endl;
        BYTE* mftSector = new BYTE[512];
        int numSectors = sizeOfMFTEntry / 512;
        // skip 16 entries : 1024*16
        startingSectorOfMFT = 2097230 * 512;

        for (int i = 0; i < numSectors; i++)
        {
            int mftSectorResult = ReadSector(L"\\\\.\\E:", startingSectorOfMFT, mftSector, 512);
            printSector(mftSector, bytesOfSector);
            startingSectorOfMFT += 512;
            delete[] mftSector;
            mftSector = new BYTE[512];
        }
    }

    return 0;
}
