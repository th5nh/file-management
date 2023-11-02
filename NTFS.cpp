/*
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
    DWORD father = 0;
};

File readFileName(BYTE* sector, int sizeRecord)
{
    File file;
    DWORD fpermission;
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
        //Tim attribute standard
        if (*((DWORD*)&sector[offset]) == 16)
            //File permission la 00 00 hoac 20 00  thi moi doc
            if (*((DWORD*)&sector[offset + 56]) != 0 && *((DWORD*)&sector[offset + 56]) != 32)
                return file;
        //Tim attribute filename
        if (*((DWORD*)&sector[offset]) != 48)
        {
            offset += *((DWORD*)&sector[offset + 4]);
            continue;
        }
        //Bo qua header
        DWORD header = *((WORD*)&sector[offset+20]);
        offset += header;
        nameLength = *((BYTE*)&sector[offset + 64]);
        file.size = *((DWORD*)&sector[offset + 48]);
        x = 0;
    }
    //Nhan doi do dai vi co dau cham
    nameLength = nameLength * 2;
    DWORD father = *((WORD*)&sector[offset]);
    file.father = father;
    for (int i = offset + 66; i < (nameLength + offset + 66); i = i + 2)
    {
        //Bo qua cac dau cham
        file.filename += sector[i];
    }
    return file;
}



vector<File> subFolder(LPCWSTR DRIVE, long long startMFTByte, int sizeRecord, DWORD father)
{
    vector<File> VF;
    //Nhay qua 26 file MFT
    DWORD ID = 26;
    int x = 1;
    do {
        BYTE* sector = new BYTE[sizeRecord];
        ReadSector(DRIVE, startMFTByte + ID*sizeRecord, sector, sizeRecord);
        ID++;
        DWORD Sign = *((WORD*)&sector[0]);
        //Kiem tra co phai la file
        if (Sign == 0)
            break;
        File f=readFileName(sector,sizeRecord);
        if (f.father == father)
            VF.push_back(f);

    } while (x);
    return VF;
}


string file_extension(const string& file_path) {
    size_t dot_pos = file_path.find_last_of(".");
    if (dot_pos != string::npos) {
        return file_path.substr(dot_pos);
    }
    return "";
}

void readFile(LPCWSTR DRIVE, long long startMFTByte, int sizeRecord, DWORD pos)
{
    BYTE* sector = new BYTE[sizeRecord];
    ReadSector(DRIVE, startMFTByte + pos * sizeRecord, sector, sizeRecord);
    File f = readFileName(sector, sizeRecord);

    string fex= file_extension(f.filename);
    if (fex == ".txt")
    {
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
                    cout << readSec[j];
                begin++;
            }

            count = *((BYTE*)&sector[offset + 1]);
            next = *((BYTE*)&sector[offset + 2]);
            begin += next;

            offset += 3;
        }
        return;
    }
    
    if (fex == ".jpg" || fex == ".png") 
    {
        cout << "Hay mo file bang Paint!" << endl;
    }
    else if (fex == ".doc") 
    {
        cout << "Hay mo file bang MS Word!" << endl;
    }
    else if (fex == ".pptx") 
    {
        cout << "Hay mo file bang MS PowerPoint!" << endl;
    }
    else if (fex == ".cpp") 
    {
        cout << "Hay mo file bang Visual Studio" << endl;
    }
    else 
    {
        cout << "Hay mo file bang Paint!" << endl;
    }
}

void subDirectory(LPCWSTR DRIVE, vector<string> path, long long startMFTByte, string prefix, int sizeRecord, int deep, DWORD father)
{
    vector<File> branche = subFolder(DRIVE, startMFTByte, sizeRecord, father);
    for (int i = 0; i < branche.size(); i++)
    {
        if (branche[i].filename == path[deep])
        {
            
            if(branche[i].type == "[Folder]" && deep < path.size() - 1)
            {
                cout << prefix << branche[i].filename << endl;
                deep = deep + 1;
                subDirectory(DRIVE, path, startMFTByte, "        " + prefix, sizeRecord, deep,branche[i].pos);
                return;
            }

            if (branche[i].type == "[Folder]")
            {
                cout << prefix << branche[i].filename << endl;
                vector<File> G;
                G = subFolder(DRIVE, startMFTByte, sizeRecord, branche[i].pos);
                for (int j = 0; j < G.size(); j++)
                {
                    if (G[j].type == "[File]")
                        cout << "       " + prefix << G[j].type << " | " << G[j].filename << " | Size: " << G[j].size << " bytes | Cluster: " << G[j].pos << endl;
                    else if (branche[i].type == "[Folder]")
                        cout << "       " + prefix << G[j].type << " | " << G[j].filename << " | Cluster: " << G[j].pos << endl;
                }
                return;
            }

            if (branche[i].type == "[File]")
            {                
                cout << prefix << branche[i].type << " | " << branche[i].filename << " | Size: " << branche[i].size << " bytes | Cluster: " << branche[i].pos << endl; 
                readFile(DRIVE, startMFTByte, sizeRecord, branche[i].pos);
                return;
            }
        }
    }
    

}

LPCWSTR convertToLPCWSTR(const string& str) {
    LPCWSTR result = NULL;
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (size_needed > 0) {
        LPWSTR wide_string = new WCHAR[size_needed];
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wide_string, size_needed);
        result = wide_string;
    }
    return result;
}

int main(int argc, char** argv)
{
    // Kich thuoc cua 1 sector 
    DWORD sectorSize = 512;
    
    string dr;
    cout << "Nhap o dia ( vi du \\\\.\\F:)" << endl;
    cin >> dr;

    LPCWSTR DRIVE = convertToLPCWSTR(dr);

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

    cout << "Byte bat dau cua MFT: " << startMFTByte << endl;

    

    // Bo qua 26 MFT dau tien vi no thuoc he thong 
    // Diem tim kiem folder luc nay se la  1073768448
    long long startMFTSeekFolderByte = startMFTByte + (26 * 1024);
    cout << "Diem tim kiem : (skip 26 MFT): " << startMFTSeekFolderByte << endl;;
    
    while (!readFileNameMFT(DRIVE, startMFTSeekFolderByte, 1024)) {
        startMFTSeekFolderByte += 1024;
    }
    

    BYTE* sector1 = new BYTE[1024];
    ReadSector(DRIVE, startMFTByte + 40 * 1024, sector1, 1024);
    readFileName(sector1,1024);
    DWORD root = 5;

    vector<File> rootD = subFolder(DRIVE, startMFTByte, 1024, root);
    cout << "Cay thu muc goc " << DRIVE << " la: " << endl;
    for (int i = 0; i < rootD.size(); i++)
    {
        if (rootD[i].type == "[File]")
            cout << "  --" << rootD[i].type << " | " << rootD[i].filename << " | Size: " << rootD[i].size << " bytes | Cluster: " << rootD[i].pos << endl;
        else if (rootD[i].type == "[Folder]")
            cout << "  --" << rootD[i].type << " | " << rootD[i].filename << " | Cluster: " << rootD[i].pos << endl;
    }

    size_t pos = 0;
    size_t last_pos = 0;
    string Path;
    cout << "Nhap duong dan file (vi du Folder1\\Folder3 ): " << endl;
    cin >> Path;
    vector<string> path;

    //Tach duong dan ra thanh tung phan
    while (pos != string::npos)
    {
        pos = Path.find("\\", last_pos);
        string part = Path.substr(last_pos, pos - last_pos);
        path.push_back(part);
        last_pos = pos + 1;
    }
   
    
    int deep = 0;
    cout << "Cay thu muc con co duong dan " << Path << " la: " << endl;
    subDirectory(DRIVE, path, startMFTByte, "--", 1024, deep, root);


    return 0;
}
*/

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <locale.h>

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
                    DWORD64 content = attri + 24;
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
                printf("    |");
            }
            for (int j = 0; j < MFTs[i].filenameLength; j++) {
                printf("%c", MFTs[i].filename[j]);
            }
            printf("\n");
            if (MFTs[i].type == 3) {
                printRootDirectory(MFTs, MFTs[i].start, numtab + 1);
            }
        }
    }
}

void printDirectory(std::vector<MFT> MFTs, DWORD64 root) {
    for (int i = 0; i < MFTs.size(); i++) {
        if (MFTs[i].flag != 6 && MFTs[i].parent == root) {
            printf("  ");
            for (int j = 0; j < MFTs[i].filenameLength; j++) {
                printf("%c", MFTs[i].filename[j]);
            }
            printf("\n");
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
                cout << readSec[j];
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
    }

    if (fex == ".jpg" || fex == ".png")
    {
        cout << "Hay mo file bang Paint!" << endl;
    }
    else if (fex == ".doc")
    {
        cout << "Hay mo file bang MS Word!" << endl;
    }
    else if (fex == ".pptx")
    {
        cout << "Hay mo file bang MS PowerPoint!" << endl;
    }
    else if (fex == ".cpp")
    {
        cout << "Hay mo file bang Visual Studio!" << endl;
    }
    else
    {
        cout << "Dinh dang file chua biet!" << endl;
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
    DWORD sizeMFT = 1024;//1 << abs((long long)256 - bootSector->ClusterPerFileRecordSegment);

    // READ needed MFT ENTRY #0
    printf("\n(2). ROOT DIRECTORY\n\n");

    std::vector<MFT> MFTs = getMFTs(Drive, startMFT, sizeMFT);
    int root = 5;
    printRootDirectory(MFTs, 5, 1);

    while (1) {
        printf("\n\nTHU MUC HIEN TAI\n\n");
        printDirectory(MFTs, root);
        int choice = 0;
        std::cout << "\n\n0. Thoat\n";
        std::cout << "1. Hien thi cay thu muc\n";
        std::cout << "2. Di den thu muc / tap tin\n";
        std::cout << "> Nhap lua chon: ";
        std::cin >> choice; std::cin.ignore();
        printf("\n--------------------------------------------------------------\n");
        if (choice == 0) {
            return 1;
        }
        else if (choice == 1) {
            printRootDirectory(MFTs, root, 1);
        }
        else {
            printDirectory(MFTs, root);
            std::string tmp = "";
            std::cout << "\n> Nhap ten thu muc / file: ";
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
                        readFile(Drive, startMFT, sizeMFT,MFTs[i], MFTs[i].start);
                    }
                    break;
                }
            }
        }


    }




    return 0;
}
