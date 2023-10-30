/*
#include <iostream>
#include <windows.h>

using namespace std;

void displayFolderContents(const std::string& folderPath, const std::string& prefix)
{
    WIN32_FIND_DATAA findData;
    string searchPath = folderPath + "\\*.*";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            string fileName = findData.cFileName;
            if (fileName != "." && fileName != "..") 
            {
                cout << prefix << fileName;
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
                {
                    cout << " [Folder]";
                    string subPath = folderPath + "\\" + fileName;
                    cout << "\n";
                    displayFolderContents(subPath, "  " + prefix);
                }
                else 
                {
                    cout << " [File]";
                    ULONGLONG fileSize = findData.nFileSizeLow + (static_cast<ULONGLONG>(findData.nFileSizeHigh) << 32);
                    cout << " (" << fileSize << " bytes)";
                    cout << "\n";
                }
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
}

int main()
{
    string partition = "F:";
    cout << "Contents of partition " << partition << ":\n";
    displayFolderContents(partition, "--");
    return 0;
}

*/


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


    LARGE_INTEGER distance;
    distance.QuadPart = readPoint;
    SetFilePointerEx(device, distance, NULL, FILE_BEGIN); // Set a Point to Read

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

long long CalculateMFTStartSector(BYTE bootSector[512])
{
    // Extract the MFT cluster number from the boot sector
    DWORD mftClusterNumber = *((DWORD*)&bootSector[0x30]);

    // Extract SectorsPerCluster from the boot sector (typically at offset 0x0D)
    WORD sectorsPerCluster = *((WORD*)&bootSector[0x0D]);
    WORD bytesPerSector = *((WORD*)&bootSector[0x0B]);

    // Calculate the starting sector of the MFT
    long long startingSectorOfMFT = mftClusterNumber * sectorsPerCluster * bytesPerSector;
    return startingSectorOfMFT;
}

long long converClusterToByte(DWORD cluster, BYTE bootSector[512]) {
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
            if (offset < size) {
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


int recordSize(BYTE sector[512])
{
    DWORD size = *((DWORD*)&sector[28]);
    cout << size;
    return (int)size;
} 

string readFileName(BYTE* sector)
{
    string filename;
    DWORD size;
    string type;
    WORD flag = *((WORD*)&sector[22]);
    BYTE nameLength;
    if (flag == 1) 
    {
        type = "[File]";
    } else if(flag == 3) 
    {
        type = "[Folder]";
    }
    else
    {
        return "";
    }
    DWORD offset = *((WORD*)&sector[20]);
    int x = 1;
    while (x)
    {
        if (*((DWORD*)&sector[offset]) != 48)
        {
            offset += *((DWORD*)&sector[offset + 4]);
            continue;
        }
        offset += 24;
        nameLength = *((BYTE*)&sector[offset + 64]); 
        cout << nameLength << endl;
        size= *((DWORD*)&sector[offset + 48]);
        x = 0;
    }

    nameLength = nameLength * 2;
    for (int i = offset + 66; i < (nameLength + offset + 66); i++)
    {
        filename += sector[i];
    }
    cout << type << " | " << filename << " | Size: " << size << " byte" << endl;
    return filename;
}


void rootDirectory(long long startingSectorOfMFT, string prefix, int sizeRecord)
{
    BYTE* sector = new BYTE[1024];
    long long root = startingSectorOfMFT + 1024*5;
    int Sector2Result = ReadSector(L"\\\\.\\F:", root, sector, sizeRecord);

    WORD flag = *((WORD*)&sector[22]);
    if (flag == 1)
    {
        //type = "[File]";
    }
    else if (flag == 3)
    {
        //type = "[Folder]";
    }
    else
    {
        return;
    }

    DWORD offset = *((WORD*)&sector[20]);

    int x = 1;
    while (x)
    {
        if (*((DWORD*)&sector[offset]) != 160)
        {
            offset += *((DWORD*)&sector[offset + 4]);
            continue;
        }
        DWORD headerlength = *((DWORD*)&sector[offset + 32]);
        offset += headerlength;
        x = 0;
    }

    WORD RD= *((WORD*)&sector[offset+2]);
    long long readroot = RD * 512;

    BYTE* RS = new BYTE[1024];
    
    int Result = ReadSector(L"\\\\.\\F:", readroot, RS, sizeRecord);

    DWORD rootSize = *((DWORD*)&RS[28]);
    int nRecord = (rootSize / 1024);

    
    int offsetLength = 88; 

    vector<DWORD> r;

    while(offsetLength < rootSize)
    {
        
        DWORD offsetIndex = *((DWORD*)&RS[offsetLength]);
        r.push_back(offsetIndex);
        offsetLength+= *((WORD*)&RS[offsetLength + 8]);


        if (offsetLength>=1024 && nRecord>0)
        {
            ReadSector(L"\\\\.\\F:", readroot+1024, RS, sizeRecord);
            offsetLength -= 1024;
            rootSize -= 1024;
            nRecord--;
        }            
    }
    
    for (int i = 0; i < r.size(); i++)
    {
        BYTE* In = new BYTE [sizeRecord];
        int Sector2Result = ReadSector(L"\\\\.\\F:", startingSectorOfMFT+(r[i]*1024), In, sizeRecord);
        readFileName(In);
        delete[] In;
    }
        
    
    delete[] RS;
    delete[] sector;
}

int main(int argc, char** argv)
{
    BYTE sector[512];
    int result = ReadSector(L"\\\\.\\F:", 0, sector, 512);

    if (result == 0)  
    {
        int bytesOfSector = *((WORD*)&sector[0x0B]);
        printSector(sector, 512);
        long long startingSectorOfMFT = CalculateMFTStartSector(sector);
        int sizeOfMFTEntry = calculateRecordSizeInMFT(sector);
        vector<BYTE> mft;
        cout << endl << startingSectorOfMFT << endl;
        BYTE* mftSector = new BYTE[512];
        int numSectors = sizeOfMFTEntry / 512;
        // skip 16 entries : 1024*16

        BYTE* sector2 = new BYTE[512];
        int Sector2Result = ReadSector(L"\\\\.\\F:", startingSectorOfMFT, sector2, 512);

        int sizeRecord = recordSize(sector2);
        delete[] sector2;
        rootDirectory(startingSectorOfMFT,"  ",sizeRecord);

        startingSectorOfMFT += 512 * 54;
        
        /*
        for(int i=0;i<50;i++)
        {
            BYTE* sector3 = new BYTE[sizeRecord];
            int Sector2Result = ReadSector(L"\\\\.\\F:", startingSectorOfMFT, sector3, sizeRecord);
            readFileName(sector3);
            startingSectorOfMFT += 1024;
            delete[] sector3;
        }
        */
    }

    return 0;
}

