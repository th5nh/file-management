#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>

using namespace std;
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

class Attribute {
    int startHeader;
    int typeCode;
    int startContent;
    int totalSize;
    

    Attribute(int start, BYTE* MFT) {
        startHeader = start;
        this->typeCode = *((DWORD*)&MFT[this->startHeader]);
        this->startContent = this->startHeader + *((WORD*)&MFT[this->startHeader + 20]);
        this->totalSize = *((DWORD*)&MFT[this->startHeader + 4]);
    }

    virtual int findStartContent(BYTE* MFT) {
        // Byte thu 20 -> 21 se la vi tri bat dau cua content
        this->startContent =  this->startHeader + *((WORD*)&MFT[this->startHeader + 20]);
        return this->startContent;
    }
    virtual DWORD findTotalSize(BYTE* MFT) {
        //Byte thu 4  - 7 la kich thuoc cua attribute
        this->totalSize = *((DWORD*)&MFT[this->startHeader + 4]);
        return this->totalSize;
    }
};

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
        return 1;
    }


    LARGE_INTEGER distance{};
    distance.QuadPart = readPoint;
    SetFilePointerEx(device, distance, NULL, FILE_BEGIN); // Set a Point to Read

    if (!ReadFile(device, sector, sectorSize, &bytesRead, NULL))
    {
        retCode = 2;
    }
    else
    {
        retCode = 0;
    }

    CloseHandle(device); // Close the device handle
    return retCode;
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

vector<int> edgeAttributeStandard(BYTE* MFT) {
    vector <int> result;
    WORD startAttStandard = *((WORD*)&MFT[0x14]);
    result.push_back(startAttStandard);

    // offset bat dau cua Content 0x4C - 0x4D -> 0x18 = 24
    WORD offsetContentAttStandard = *((WORD*)&MFT[0x4C]);

    // Vi tri bat dau :  56 + 24 =  80 ( 0x50)
    int startContentAttStandard = startAttStandard + offsetContentAttStandard;
    return result;
}


bool isNotSystemMFT(LPCWSTR drive, DWORD64 startByte, int sectorSize) {
    BYTE* MFT = new BYTE[sectorSize];
    // Lay thong tin cua MFT 
    int res = ReadSector(drive, startByte, MFT, sectorSize);
    if (res != 0) return 0;
    // Vi tri bat dau cua Standard 0x14 - 0x15 -> 0x38 = 56
    WORD startAttStandard = *((WORD*)&MFT[0x14]);
   
    // offset bat dau cua Content 0x4C - 0x4D -> 0x18 = 24
    WORD offsetContentAttStandard = *((WORD*)&MFT[0x4C]);

    // Vi tri bat dau :  56 + 24 =  80 ( 0x50)
    int startContentAttStandard = startAttStandard + offsetContentAttStandard;
    
    // Doc Flag o 32 - 35 (0x20 - 0x23)
    DWORD flag = *((DWORD*)&MFT[startContentAttStandard+0x20]);
    delete[] MFT;
    if (flag == 0x00 || flag == 0x20) return 1;
    return 0;
}

string readFileNameMFT(LPCWSTR drive, DWORD64 startByte, int sectorSize) {
    BYTE* MFT = new BYTE[sectorSize];
    // Lay thong tin cua MFT 
    int res = ReadSector(drive, startByte, MFT, sectorSize);
    if (res != 0) return "";
    // Vi tri bat dau cua Standard 0x14 - 0x15 -> 0x38 = 56
    WORD startAttStandard = *((WORD*)&MFT[0x14]);
    
    // Doc kich thuoc standard 
    DWORD standardSize = *((DWORD*)&MFT[startAttStandard + 0x4]);

    // Skip qua standard , cung chinh la bat dau FILENAME: 56 + 96 = 152
    DWORD64 startFileNameHeader = startAttStandard + standardSize;

    // Skip qua 16 cua Header, de diem bat dau Content va Length Content
    DWORD startFileNameContent = *((WORD*)&MFT[startFileNameHeader+20]) + startFileNameHeader;
    // Doc chieu dai 
    DWORD length = MFT[startFileNameContent + 64];
    // Moi ky tu cach nhau 0 nen khong tinh
    length = length * 2 - 1;
    // Doc dinh dang tap tin :
    BYTE format = MFT[startFileNameContent + 65];

    // Doc ten tap tin
    string fileName = "";

    for (int i = 0; i < length; i++) {
        char c = static_cast<char>(MFT[startFileNameContent + 66 + i]);
            if(c) fileName += c;
    }
    delete[] MFT;
    return fileName;
}

DWORD64 findFileByName(LPCWSTR drive,DWORD64 startByte, int sectorSize, string fileToFind) {
   DWORD64 curByte = startByte;
    int i = 0;
    while (i < 1000)
    {
        string fileName = readFileNameMFT(drive, curByte, sectorSize);
        size_t found = fileName.find(fileToFind);
        if (found != std::string::npos) {
            return curByte;
        }
        curByte += 1024;
        i++;
    }
    return 0;
}


int main(int argc, char** argv)
{
    // Kich thuoc cua 1 sector 
    int sectorSize = 512;
    LPCWSTR DRIVE = L"\\\\.\\E:";
    BYTE* sector = new BYTE[sectorSize];
    int result = ReadSector(DRIVE, 0, sector, sectorSize);

    if (result != 0)
    {
        return 0;
    }
    
    // So sector trong 1 cluster   :  8 
    int sectorsPerCluster = sector[0xD];

    // Kich thuong cua mot ban ghi MFT
    // So dang bu 2 , nen doi qua int roi dich bit
    int decimalValue = static_cast<int>(static_cast<signed char>(sector[0x40])); 
    int MFTsize = 1 << abs(decimalValue);

    // Cluster bat dau cua MFT 
    DWORD64 startMFTCluster = *((DWORD*)&sector[0x30]);
    // vi tri bat dau cua MFT : 1073741824
    DWORD64 startMFTByte = startMFTCluster * sectorsPerCluster * sectorSize;

    cout << "\nByte bat dau cua MFT: " << startMFTByte;
    // Bo qua 26 MFT dau tien vi no thuoc he thong 
    // Diem tim kiem folder luc nay se la  1073768448
    DWORD64 startMFTSeekFolderByte = startMFTByte + (26 * MFTsize);
    cout << "\nDiem tim kiem : (skip 26 MFT): " << startMFTSeekFolderByte;
    
    cout << "\nStart of Siue: " << findFileByName(DRIVE, startMFTSeekFolderByte, 512, "siue");
    

    return 0;
}


