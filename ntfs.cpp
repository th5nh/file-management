

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>

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
        return 1;
    }

    LARGE_INTEGER distance;
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

bool isNotSystemMFT(LPCWSTR drive, long startByte, int sectorSize) {
    BYTE* MFT = new BYTE[sectorSize];
    // Lay thong tin cua MFT 
    int res = ReadSector(drive, startByte, MFT, sectorSize);
    if (res != 0) return 0;
    // Vi tri bat dau cua Standard 0x14 - 0x15 -> 0x38 = 56
    WORD startAttStandard = *((WORD*)&MFT[0x14]);

    // offset bat dau cua Content 0x4C - 0x4D -> 0x18 = 24
    WORD offsetContentAttStandard = *((WORD*)&MFT[0x4C]);

    // Vi tri bat dau :  56 + 24 =  80 ( 0x50)
    WORD startContentAttStandard = startAttStandard + offsetContentAttStandard;

    // Doc Flag o 32 - 35 (0x20 - 0x23)
    DWORD flag = *((DWORD*)&MFT[startContentAttStandard + 0x20]);
    if (flag == 0x00 || flag == 0x20) return 1;
    return 0;
}


bool readFileNameMFT(LPCWSTR drive, long long startByte, int sectorSize) {
    
    BYTE* MFT = new BYTE[sectorSize];
    // Lay thong tin cua MFT 
    int res = ReadSector(drive, startByte, MFT, sectorSize);
    if (res != 0) return 0;
    // Vi tri bat dau cua Standard 0x14 - 0x15 -> 0x38 = 56
    WORD startAttStandard = *((WORD*)&MFT[0x14]);
    // Doc kich thuoc standard 
    DWORD standardSize = *((DWORD*)&MFT[startAttStandard + 4]);
    
    // Skip qua standard , cung chinh la bat dau FILENAME: 56 + 96 = 152
    int startFileNameHeader = startAttStandard + standardSize;

    // Skip qua 16 cua Header, de diem bat dau Content va Length Content
    int startFileNameContent = *((WORD*)&MFT[startFileNameHeader + 20]) + startFileNameHeader;
    // Doc chieu dai 
    int length = MFT[startFileNameContent + 64];
    // Doc dinh dang tap tin :
    int format = MFT[startFileNameContent + 65];

    // Doc ten tap tin
    string fileName = "";

    for (int i = 0; i < length; i++) {
        char c = static_cast<char>(MFT[startFileNameContent + 66 + i]);

        if (c == '$') return 0;
        if (c) fileName += c;
    }
    size_t found = fileName.find("System Volume");
    if (found != std::string::npos) {
        return 0;
    }
    cout << "\nTen tap tin la: " << fileName << " | tai : " << startByte << endl;
    return 1;
}

struct File {
    string filename = "";
    DWORD size = 0;
    string type = "";
    DWORD pos = 0;
};

File readFileName(BYTE* sector)
{
    File file;

    WORD flag = *((WORD*)&sector[22]);
    BYTE nameLength;
    if (flag == 1)
    {
        file.type = "[File]";
    }
    else if (flag == 3)
    {
        file.type = "[Folder]";
    }
    else
    {
        return file;
    }
    file.pos = *((DWORD*)&sector[44]);

    DWORD offset = *((WORD*)&sector[20]);

    int x = 1;
    while (x)
    {
        //Tim attribute filename
        if (*((DWORD*)&sector[offset]) != 48)
        {
            offset += *((DWORD*)&sector[offset + 4]);
            continue;
        }
        offset += 24;
        nameLength = *((BYTE*)&sector[offset + 64]);
        file.size = *((DWORD*)&sector[offset + 48]);
        x = 0;
    }
    
    nameLength = nameLength * 2;

    for (int i = offset + 66; i < (nameLength + offset + 66); i=i+2)
    {
        file.filename += sector[i];
    }
    
    return file;
}

vector<File> readSubFolder(LPCWSTR drive, long long startingSectorOfMFT, int sizeRecord, DWORD pos)
{
    vector<File> VF;
    vector<DWORD> r;
    DWORD64 read = startingSectorOfMFT + (pos * sizeRecord);
    BYTE* sector = new BYTE[sizeRecord];

    ReadSector(drive, read,sector, sizeRecord);

    DWORD offset = *((WORD*)&sector[20]);
    DWORD Size = *((DWORD*)&sector[28]);
    int nRecord = (Size / sizeRecord);

    int x = 1;
    while (x)
    {
        //Tim $90 chua vi tri file luu cay thu muc con
        if (*((DWORD*)&sector[offset]) != 144)
        {
            offset += *((DWORD*)&sector[offset + 4]);
            continue;
        }
        DWORD headerlength = *((DWORD*)&sector[offset + 4]) - *((DWORD*)&sector[offset + 16]);
        offset += headerlength;
        x = 0;
    }

    offset += 32;
    while (offset < Size)
    {
        WORD offsetIndex = *((DWORD*)&sector[offset]);
        if (offsetIndex > 26 && offsetIndex<65535)
        {
            r.push_back(offsetIndex);
        }
        if (offsetIndex == 0) break;
        offset += *((WORD*)&sector[offset + 8]);
        if (offset >= 1024 && nRecord > 0)
        {
            read += sizeRecord;
            ReadSector(drive, read, sector, sizeRecord);
            offset -= sizeRecord;
            Size -= sizeRecord;
            nRecord--;
        }
    }
    for (int i = 0; i < r.size(); i++)
    {
        File f;
        BYTE* In = new BYTE[sizeRecord];
        int Sector2Result = ReadSector(drive, startingSectorOfMFT + (r[i] * sizeRecord), In, sizeRecord);
        f = readFileName(In);
        VF.push_back(f);
        delete[] In;
    }
    return VF;
}

vector<File> rootDirectory(LPCWSTR drive, long long startingSectorOfMFT, int sizeRecord)
{
    vector<File> VF;

    BYTE* sector = new BYTE[sizeRecord];
    //Nhay toi root
    long long root = startingSectorOfMFT + sizeRecord * 5;
    int Sector2Result = ReadSector(drive, root, sector, sizeRecord);

    WORD flag = *((WORD*)&sector[22]);

    DWORD offset = *((WORD*)&sector[20]);

    int x = 1;
    while (x)
    {
        //Tom $A0 chua vi tri file luu cay thu muc goc
        if (*((DWORD*)&sector[offset]) != 160)
        {
            offset += *((DWORD*)&sector[offset + 4]);
            continue;
        }
        DWORD headerlength = *((DWORD*)&sector[offset + 32]);
        offset += headerlength;
        x = 0;
    }

    WORD RD = *((WORD*)&sector[offset + 2]);
    DWORD64 readroot = RD * 512;

    BYTE* RS = new BYTE[1024];

    int Result = ReadSector(drive, readroot, RS, sizeRecord);

    DWORD rootSize = *((DWORD*)&RS[28]);
    int nRecord = (rootSize / sizeRecord);


    int offsetLength = 88;

    vector<DWORD> r;

    while (offsetLength < rootSize)
    {

        DWORD offsetIndex = *((DWORD*)&RS[offsetLength]);
        if (offsetIndex > 26 && offsetIndex<65535)
        {
            r.push_back(offsetIndex);
        }

        offsetLength += *((WORD*)&RS[offsetLength + 8]);

        if (offsetLength >= 1024 && nRecord > 0)
        {
            readroot += sizeRecord;
            ReadSector(drive, readroot, RS, sizeRecord);
            offsetLength -= sizeRecord;
            rootSize -= sizeRecord;
            nRecord--;
        }
    }
    for (int i = 0; i < r.size(); i++)
    {
        File f;
        BYTE* In = new BYTE[sizeRecord];
        int Sector2Result = ReadSector(drive, startingSectorOfMFT + (r[i] * sizeRecord), In, sizeRecord);
        f=readFileName(In);
        
        VF.push_back(f);
        delete[] In;
    }
    

    delete[] RS;
    delete[] sector;
    return VF;
}

bool subFolder(LPCWSTR drive, vector<string> Path, long long startMFTByte, DWORD pos, string prefix, int sizeRecord, int deep)
{
    if (Path.size() == deep)
        return false;
    vector<File> VF;
    VF = readSubFolder(drive, startMFTByte, sizeRecord, pos);

    for (int i = 0; i < VF.size(); i++)
    {
        if (VF[i].filename == Path[deep])
        {
            cout << prefix << VF[i].filename << endl;
            if (VF[i].type == "[Folder]" && deep < Path.size()-1)
            {
                deep = deep + 1;
                return subFolder(drive, Path, startMFTByte, VF[i].pos, "        " + prefix, sizeRecord, deep);
            }

            if (VF[i].type == "[Folder]")
            {
                vector<File> G;
                G = readSubFolder(drive, startMFTByte, sizeRecord, VF[i].pos);                
                for (int j = 0; j < G.size(); j++)
                    cout << "       " + prefix << G[j].filename << endl;
                return true;
            }

            if (VF[i].type == "[File]")
            {
                return true;
            }
                


        }
    }
        
    
    
    return true;
}

bool subDirectory(LPCWSTR drive, string Path, long long startMFTByte, string prefix, int sizeRecord, int deep)
{
    vector<File> root;
    vector<string> path;

    size_t pos = 0;
    size_t last_pos = 0;

    while (pos != string::npos)
    {
        pos = Path.find("\\", last_pos); // tim vi tri cua ky tu "\"
        string part = Path.substr(last_pos, pos - last_pos); // lay mot phan tu duong dan
        path.push_back(part); // them phan tu vao vector
        last_pos = pos + 1; // chuyen đen vi tri tiep theo
    }

    root = rootDirectory(drive, startMFTByte, sizeRecord);
    

    for (int i = 0; i < root.size(); i++)
    {
        if (root[i].filename == path[0])
        {
            cout << prefix << root[i].filename << endl;
            if (path.size() == 1)
            {
                vector<File> G;
                G = readSubFolder(drive, startMFTByte, sizeRecord, root[i].pos);
                for (int j = 0; j < G.size(); j++)
                    cout << "  " << prefix << G[j].filename << endl;
                return true;
            }
            else
            {
                deep = deep + 1;
                return subFolder(drive, path, startMFTByte, root[i].pos, "      " + prefix, sizeRecord, deep);
            }
        }
        else
        {
            cout << "Khong tim thay!" << endl;
            return false;
        }
    }
       
}


int main(int argc, char** argv)
{
    // Kich thuoc cua 1 sector 
    DWORD sectorSize = 512;
    LPCWSTR DRIVE = L"\\\\.\\F:";
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
    
    
    
    // skip 16 entries : 1024*16

    
    // Cluster bat dau cua MFT 
    long long startMFTCluster = *((DWORD*)&sector[0x30]);
    
    // vi tri bat dau cua MFT : 1073741824
    long long startMFTByte = startMFTCluster * sectorsPerCluster * sectorSize;

    cout << "Byte bat dau cua MFT: " << startMFTByte<<endl;

    

    // Bo qua 26 MFT dau tien vi no thuoc he thong 
    // Diem tim kiem folder luc nay se la  1073768448
    long long startMFTSeekFolderByte = startMFTByte + (26 * 1024);
    cout << "Diem tim kiem : (skip 26 MFT): " << startMFTSeekFolderByte << endl;;
    /*
    while (!readFileNameMFT(DRIVE, startMFTSeekFolderByte, 1024)) {
        startMFTSeekFolderByte += 1024;
    }
    */
    
    vector<File> r=rootDirectory(DRIVE,startMFTByte, 1024);
    for (int i = 0; i < r.size(); i++)
    {
        if(r[i].type=="[File]")
            cout << "--" << r[i].type << " | " << r[i].filename << " | Size: " << r[i].size << " bytes | Cluster: " << r[i].pos << endl;
        else if(r[i].type == "[Folder]")
            cout << "--" << r[i].type << " | " << r[i].filename << " | Cluster: " << r[i].pos << endl;
    }
        
    

    LPCWSTR subRoot = L"\\\\.\\F:";
    string Path= "Folder1\\Folder3";
    int deep = 0;
    subDirectory(subRoot, Path, startMFTByte, "--", 1024, deep);
    
    
    return 0;
}

/*
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
    DWORD64 readroot = RD * 512;

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
            readroot += sizeRecord;
            ReadSector(L"\\\\.\\F:", readroot, RS, sizeRecord);
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
        
        
        for(int i=0;i<50;i++)
        {
            BYTE* sector3 = new BYTE[sizeRecord];
            int Sector2Result = ReadSector(L"\\\\.\\F:", startingSectorOfMFT, sector3, sizeRecord);
            readFileName(sector3);
            startingSectorOfMFT += 1024;
            delete[] sector3;
        }
        
    }

    return 0;
}
*/
