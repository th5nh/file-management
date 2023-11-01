#include <windows.h>
#include <stdio.h>

int ReadSector(LPCSTR drive, int readPoint, BYTE sector[512])
{
    int retCode = 0;
    DWORD bytesRead;
    HANDLE device = NULL;

    device = CreateFile(drive,                              // Drive to open
                        GENERIC_READ,                       // Access mode
                        FILE_SHARE_READ | FILE_SHARE_WRITE, // Share Mode
                        NULL,                               // Security Descriptor
                        OPEN_EXISTING,                      // How to create
                        0,                                  // File attributes
                        NULL);                              // Handle to template

    if (device == INVALID_HANDLE_VALUE) // Open Error
    {
        printf("CreateFile: %u\n", GetLastError());
    }

    SetFilePointer(device, readPoint, NULL, FILE_BEGIN); // Set a Point to Read

    if (!ReadFile(device, sector, 512, &bytesRead, NULL))
    {
        printf("ReadFile: %u\n", GetLastError());
    }
    else
    {
        printf("Success!\n");
        return 0;
    }
}

void printSector(BYTE *sector, int size)
{
    printf("------Boot Sector Data--------\n");

    for (int row = 0; row < size / 16; row++)
    {
        printf("%04X | ", 0x6000 + row * 16);

        for (int col = 0; col < 16; col++)
        {
            int offset = row * 16 + col;
            if (offset < 512)
            {
                printf("%02X ", sector[offset]);
                if ((col + 1) % 4 == 0)
                {
                    printf(" |");
                }
            }
        }
        printf("\n");
    }
    printf("---------------------------\n");
}

void printInfoOfPartition(BYTE* sector, int size) {
    int bytesPerSector = (sector[12] << 8) | sector[11];
    int sectorsPerCluster = sector[13];
    int reservedSectors = (sector[15] << 8) | sector[14];
    int numberOfFATs = sector[16];
    int sectorsPerTrack = (sector[24] << 8) | sector[23];
    int numberOfHeads = (sector[26] << 8) | sector[25];
    long totalSectors = sector[35] << 24 | sector[34] << 16 | sector[33] << 8 | sector[32];
    long sectorsPerFAT = sector[39] << 24 | sector[38] << 16 | sector[37] << 8 | sector[36];
    printf("--- Detail information of partition ---\n");
    printf("- Bytes per sector: %d\n", bytesPerSector);
    printf("- Sectors per cluster: %d\n", sectorsPerCluster);
    printf("- Reserved sectors: %d\n", reservedSectors);
    printf("- Number of FATs: %d\n", numberOfFATs);
    printf("- Number of heads: %d\n", numberOfHeads);
    printf("- Total sectors: %ld\n", totalSectors);
    printf("- Sectors per FAT: %ld\n", sectorsPerFAT);
}

int main(int argc, char **argv)
{

    // ask for drive's name
    printf("Which drive would you want to open? (C, D, E,...): ");
    // scanf used for readability
    char drive[] = "\\\\.\\?:";
    scanf("%c", drive + 4, 1);
    printf("Disk Name: %s\n", drive);

    // read boot sector and display it
    BYTE sector[512];
    if (ReadSector(drive, 0, sector) == 0)
    {
        //printSector(sector, 512);
        printInfoOfPartition(sector, 512);
    }else {
        printf("Something was wrong! :(((");
    }

    system("pause");
    return 0;
}