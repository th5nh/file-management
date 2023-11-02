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
    cout << fex << endl;
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
            cout << count << endl;
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
    /*
    while (!readFileNameMFT(DRIVE, startMFTSeekFolderByte, 1024)) {
        startMFTSeekFolderByte += 1024;
    }
    */

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