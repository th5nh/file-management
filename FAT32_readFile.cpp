// 21120344 Nguyen Trong Tri
// PROJECT 1: QUAN LY HE THONG TAP TIN
// FAT32
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <string>
#include <wchar.h>

#pragma pack(push, 1)
// Cau truc Boot Sector
typedef struct
{
    BYTE BS_jmpBoot[3];
    BYTE BS_OEMName[8];
    WORD BPB_BytsPerSec;
    BYTE BPB_SecPerClus;
    WORD BPB_RsvdSecCnt;
    BYTE BPB_NumFATs;
    WORD BPB_RootEntCnt;
    WORD BPB_TotSec16;
    BYTE BPB_Media;
    WORD BPB_FATSz16;
    WORD BPB_SecPerTrk;
    WORD BPB_NumHeads;
    DWORD BPB_HiddSec;
    DWORD BPB_TotSec32;
    DWORD BPB_FATSz32;
    WORD BPB_ExtFlags;
    WORD BPB_FSVer;
    DWORD BPB_RootClus;
    WORD BPB_FSInfo;
    WORD BPB_BkBootSec;
    BYTE BPB_Reserved[12];
    BYTE BS_DrvNum;
    BYTE BS_Reserved1;
    BYTE BS_BootSig;
    DWORD BS_VolID;
    BYTE BS_VolLab[11];
    BYTE BS_FilSysType[8];
    BYTE BS_BootCode[420];
    WORD BS_End;
} FAT32_BOOTSECTOR, *PFAT32_BOOTSECTOR;
#pragma pack(pop)

#pragma pack(push, 1)
// Cau truc entry chinh
typedef struct
{
    BYTE fileName[8];
    BYTE fileExtension[3];
    BYTE fileAttribute;
    BYTE reserved[8];
    WORD startingClusterHI;
    WORD lastModifiedTime;
    WORD lastModifiedDate;
    WORD startingClusterLO;
    DWORD fileSize;
} FAT32_MAINENTRY, *PFAT32_MAINENTRY;
#pragma pack(pop)

#pragma pack(push, 1)
// Cau truc entry phu
typedef struct
{
    BYTE entryIndex;
    BYTE fileName1[10];
    BYTE fileAttribute;
    BYTE reserved;
    BYTE checkSum;
    BYTE fileName2[12];
    WORD startingCluster;
    BYTE fileName3[4];
} FAT32_SUBENTRY, *PFAT32_SUBENTRY;
#pragma pack(pop)

/* Ham ho tro */
void clean_stdin(void)
{
    int c;
    do
    {
        c = getchar();
    } while (c != '\n' && c != EOF);
}

/* Ham in bang FAT32 boot sector */
void printTable(BYTE sector[512])
{
    printf("----- BOOT SECTOR TABLE OF FAT32 -----\n\n");

    for (int i = 0; i < 22; i++)
    {
        if (i < 6)
            printf(" ");
        else
            printf(" %0X ", i - 6);
    }

    printf("\n");

    for (int i = 0; i < 53; i++)
    {
        if (i < 5)
            printf(" ");
        else
            printf("_");
    }

    printf("\n");

    for (int i = 0; i < 512; i++)
    {
        if (i % 16 == 0)
        {
            printf("\n");
            printf("%03X | ", i);
        }
        printf("%02X ", sector[i]);
    }
}

/* Ham in thong tin boot sector */
void printBootSectorInformation(PFAT32_BOOTSECTOR bs)
{

    printf("\n\n\n				   BOOT SECTOR INFORMATION OF FAT 32\n\n");

    printf("# Bytes per sector:						  0x%04X (%d)\n", bs->BPB_BytsPerSec, bs->BPB_BytsPerSec);
    printf("# Sectors per cluster:					  0x%02X (%d)\n", bs->BPB_SecPerClus, bs->BPB_SecPerClus);
    printf("# Reserved sectors count:				  0x%04X (%d)\n", bs->BPB_RsvdSecCnt, bs->BPB_RsvdSecCnt);
    printf("# FATS count:							  0x%02X (%d)\n", bs->BPB_NumFATs, bs->BPB_NumFATs);
    printf("# Root entries count (FAT12, FAT16 only): 0x%04X (%d)\n", bs->BPB_RootEntCnt, bs->BPB_RootEntCnt);
    printf("# Total sectors (FAT12, FAT16 only):	  0x%04X (%d)\n", bs->BPB_TotSec16, bs->BPB_TotSec16);

    printf("# Sectors per FAT (FAT12, FAT16 only):	  0x%04X (%d)\n", bs->BPB_FATSz16, bs->BPB_FATSz16);

    printf("# Total sectors (FAT32 only):			  0x%08X (%d)\n", bs->BPB_TotSec32, bs->BPB_TotSec32);
    printf("# Sectors per FAT (FAT32 only):			  0x%08X (%d)\n", bs->BPB_FATSz32, bs->BPB_FATSz32);

    printf("# Root cluster number (FAT32 only):		  0x%08X (%d)\n", bs->BPB_RootClus, bs->BPB_RootClus);

    printf("# Drive number (FAT32):					  0x%02X (%d)\n", bs->BS_DrvNum, bs->BS_DrvNum);

    printf("# FAT type:								  %s\n", bs->BS_FilSysType);
}

/* Ham in thong tin entry chinh */
void printMainEntryInformation(PFAT32_MAINENTRY me)
{
    printf("# File name:					  %s\n", me->fileName);
    printf("# File extension:				  %s\n", me->fileExtension);
    printf("# File attribute:				  0x%02X (%d)\n", me->fileAttribute, me->fileAttribute);

    printf("# Starting cluster (high word):   0x%04X (%d)\n", me->startingClusterHI, me->startingClusterHI);
    printf("# Last modified time:			  0x%04X (%d)\n", me->lastModifiedTime, me->lastModifiedTime);
    printf("# Last modified date:			  0x%04X (%d)\n", me->lastModifiedDate, me->lastModifiedDate);
    printf("# Starting cluster (low word):	  0x%04X (%d)\n", me->startingClusterLO, me->startingClusterLO);
    printf("# File size:					  0x%08X (%d)\n", me->fileSize, me->fileSize);
}

/* Ham in thong tin entry phu */
void printSubEntryInformation(PFAT32_SUBENTRY se)
{
    printf("# Entry index:					  0x%02X (%d)\n", se->entryIndex, se->entryIndex);
    printf("# File name:					  %s%s%s\n", se->fileName1, se->fileName2, se->fileName3);
    printf("# File attribute:				  0x%02X (%d)\n", se->fileAttribute, se->fileAttribute);
    printf("# Starting cluster:				  0x%04X (%d)\n", se->startingCluster, se->startingCluster);
}

/* Ham doc boot sector */
int readBootSector(LPCWSTR drive, int readPoint, BYTE sector[512])
{
    int retCode = 0;
    DWORD bytesRead;
    HANDLE device = NULL;

    device = CreateFile(drive,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);

    if (device == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile: %u\n", GetLastError());
        return 1;
    }

    SetFilePointer(device, readPoint, NULL, FILE_BEGIN); // Chi dinh diem doc

    if (!ReadFile(device, sector, 512, &bytesRead, NULL))
    {
        printf("ReadFile: %u\n", GetLastError());
        return 2;
    }
    else
    {
        printf("Success!\n");
        return 0;
    }
}

/* Ham doc bang FAT */
int readFAT(LPCWSTR drive, BYTE *buffer, DWORD bufferSize, FAT32_BOOTSECTOR *bs)
{

    HANDLE hFile = NULL;

    hFile = CreateFile(drive,
                       GENERIC_READ,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       NULL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile: %u\n", GetLastError());
        return 1;
    }

    LONG firstFAT = (LONG)bs->BPB_RsvdSecCnt * 512;
    DWORD dwPointer = SetFilePointer(hFile, firstFAT, NULL, FILE_BEGIN);

    if (dwPointer == 0xFFFFFFFF)
    {
        printf("SetPointer: %u\n", GetLastError());
        return 2;
    }

    DWORD bytesRead;
    if (!ReadFile(hFile, buffer, 1024, &bytesRead, NULL))
    {
        printf("ReadFile: %u\n", GetLastError());
        return 2;
    }
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
    }

    CloseHandle(hFile);
    hFile = NULL;

    hFile = CreateFile(L"FAT.txt",
                       GENERIC_WRITE,
                       NULL,
                       NULL,
                       CREATE_ALWAYS,
                       NULL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile: %u\n", GetLastError());
        return 1;
    }
    DWORD bytesWrite;
    if (!WriteFile(hFile, buffer, bufferSize, &bytesWrite, NULL))
    {
        printf("WriteFile: %u\n", GetLastError());
        return 3;
    }
    CloseHandle(hFile);
    return 0;
}

/* Ham doc cluster */
int readCluster(LPCWSTR drive, BYTE *buffer, DWORD bufferSize, FAT32_BOOTSECTOR *bs, int clusterNum)
{
    LONG firstDataSector = (LONG)bs->BPB_RsvdSecCnt + (LONG)(bs->BPB_FATSz32 * bs->BPB_NumFATs);
    LONG firstSectorOfCluster = (LONG)((clusterNum - 2) * bs->BPB_SecPerClus) + firstDataSector;
    LONG firstSectorOfClusterInBytes = firstSectorOfCluster * 512;

    HANDLE hFile = NULL;

    hFile = CreateFile(drive,
                       GENERIC_READ,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       NULL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile: %u\n", GetLastError());
        return 1;
    }

    DWORD dwPointer = SetFilePointer(hFile, firstSectorOfClusterInBytes, NULL, FILE_BEGIN);

    if (dwPointer == 0xFFFFFFFF)
    {
        printf("SetPointer: %u\n", GetLastError());
        return 2;
    }

    DWORD bytesRead;
    if (!ReadFile(hFile, buffer, bufferSize, &bytesRead, NULL))
    {
        printf("ReadFile: %u\n", GetLastError());
        return 2;
    }
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
    }

    CloseHandle(hFile);
    hFile = NULL;

    char fileName[15];
    char num[5];
    _itoa_s(clusterNum, num, 5, 10);
    strcpy_s(fileName, 15, "CLUSTER");
    strcat_s(fileName, 15, num);
    strcat_s(fileName, 15, ".txt");
    wchar_t wfileName[15];
    size_t outSize;
    mbstowcs_s(&outSize, wfileName, 15, fileName, strlen(fileName) + 1);

    hFile = CreateFile((LPCWSTR)wfileName,
                       GENERIC_WRITE,
                       NULL,
                       NULL,
                       CREATE_ALWAYS,
                       NULL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile: %u\n", GetLastError());
        return 1;
    }

    DWORD bytesWrite;
    if (!WriteFile(hFile, buffer, bufferSize, &bytesWrite, NULL))
    {
        printf("WriteFile: %u\n", GetLastError());
        return 3;
    }

    CloseHandle(hFile);

    return 0;
}

/* Ham doc tat ca entry trong mot cluster */
DWORD getEntries(BYTE *buffer, DWORD bufferSize, BYTE ***entriesBuffer)
{
    DWORD entriesSize = 0;
    int deletedFound = 0;
    for (DWORD i = 0; i < bufferSize; i += 32)
    {
        if (buffer[i] == 0xE5)
            deletedFound++;

        if (buffer[i] == 0x00)
        {
            entriesSize = (i / 32) - deletedFound;
            break;
        }
    }

    (*entriesBuffer) = (BYTE **)malloc(entriesSize * sizeof(BYTE *));

    int k = 0;
    for (int i = 0; i < entriesSize; i++)
    {
        while (buffer[32 * k] == 0xE5)
        {
            k++;
        }

        (*entriesBuffer)[i] = (BYTE *)malloc(sizeof(BYTE) * 32);
        for (int j = 0; j < 32; j++)
        {
            (*entriesBuffer)[i][j] = buffer[32 * k + j];
        }

        k++;
    }

    return entriesSize;
}

/* Ham in cay thu muc*/
void printDir(LPCWCHAR drive, BYTE **entriesBuffer, DWORD entriesSize, PFAT32_BOOTSECTOR bs, BYTE *fatBuffer, DWORD fatSize, int indent)
{
    for (int i = entriesSize - 1; i > 0; i--)
    {
        char fileName[14];
        char isLongName = 0;

        PFAT32_MAINENTRY me = (PFAT32_MAINENTRY)entriesBuffer[i];
        if (me->fileAttribute == 0x20 || (me->fileAttribute == 0x10 && entriesBuffer[i][0] != 0x2E))
        {
            for (int i = 0; i < indent; i++)
            {
                printf("  ");
            }
            for (int j = i - 1; j >= 0; j--)
            {
                PFAT32_SUBENTRY se = (PFAT32_SUBENTRY)entriesBuffer[j];
                if (se->fileAttribute == 0x0F)
                {
                    isLongName = 1;

                    int c = 0;
                    for (int k = 0; k < 10; k++)
                    {
                        if (k % 2 == 0)
                        {
                            if (se->fileName1[k] == '\0')
                            {
                                break;
                            }
                            else
                            {
                                fileName[c++] = se->fileName1[k];
                            }
                        }
                    }
                    for (int k = 0; k < 12; k++)
                    {
                        if (k % 2 == 0 && se->fileName1[k] != '\0')
                        {
                            if (se->fileName1[k] == '\0')
                            {
                                break;
                            }
                            else
                            {
                                fileName[c++] = se->fileName2[k];
                            }
                        }
                    }
                    for (int k = 0; k < 4; k++)
                    {
                        if (k % 2 == 0 && se->fileName1[k] != '\0')
                        {
                            if (se->fileName1[k] == '\0')
                            {
                                break;
                            }
                            else
                            {
                                fileName[c++] = se->fileName3[k];
                            }
                        }
                    }
                    fileName[c] = '\0';

                    printf("%s", fileName);

                    i--;
                }
                else
                {
                    break;
                }
            }
            if (!isLongName)
            {
                int len = 0;
                for (int k = 0; k < 8; k++)
                {
                    if (me->fileName[k] != ' ')
                    {
                        fileName[len++] = me->fileName[k];
                    }
                    else
                    {
                        break;
                    }
                }

                if (me->fileAttribute != 0x10)
                {
                    fileName[len++] = '.';
                    for (int k = 0; k < 3; k++)
                    {
                        if (me->fileExtension[k] != ' ')
                        {
                            fileName[len++] = me->fileExtension[k];
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                fileName[len] = '\0';
                printf("%s", fileName);
            }

            if (me->fileAttribute != 0x10)
            {
                printf("		%d bytes", me->fileSize);
            }
            printf("\n");

            if (me->fileAttribute == 0x10)
            {
                int clusterNum = me->startingClusterLO;
                if (clusterNum < 2)
                {
                    break;
                }

                BYTE *dataBuffer = NULL;
                DWORD dataSize = bs->BPB_BytsPerSec * bs->BPB_SecPerClus + 1;
                dataBuffer = (BYTE *)malloc(dataSize * sizeof(BYTE));
                readCluster(drive, dataBuffer, dataSize - 1, bs, clusterNum);

                BYTE **entriesBuffer = NULL;
                DWORD entriesSize = getEntries(dataBuffer, dataSize, &entriesBuffer);

                printDir(drive, entriesBuffer, entriesSize, bs, fatBuffer, fatSize, indent + 1);

                DWORD *fat = (DWORD *)fatBuffer;
                memcpy_s(fat, fatSize - 1, fatBuffer, fatSize - 1);

                while (fat[clusterNum] != 0x00000000 && fat[clusterNum] != 0xfffffff7 && fat[clusterNum] != 0xfffffff8 && fat[clusterNum] != 0xffffffff && fat[clusterNum] != 0x0fffffff)
                {

                    // Don dep
                    free(dataBuffer);
                    dataBuffer = NULL;
                    for (int i = 0; i < entriesSize; i++)
                    {
                        free(entriesBuffer[i]);
                    }
                    free(entriesBuffer);
                    entriesBuffer = NULL;

                    // Lay cluster tiep theo
                    clusterNum = fat[clusterNum];

                    // Doc cluster
                    dataBuffer = NULL;
                    dataSize = bs->BPB_BytsPerSec * bs->BPB_SecPerClus + 1;
                    dataBuffer = (BYTE *)malloc(dataSize * sizeof(BYTE));
                    readCluster(drive, dataBuffer, dataSize - 1, bs, clusterNum);

                    // Doc va in cac entry
                    entriesBuffer = NULL;
                    entriesSize = getEntries(dataBuffer, dataSize, &entriesBuffer);
                    printDir(drive, entriesBuffer, entriesSize, bs, fatBuffer, fatSize, indent + 1);
                }

                // Don dep
                free(dataBuffer);
                for (int i = 0; i < entriesSize; i++)
                {
                    free(entriesBuffer[i]);
                }
                free(entriesBuffer);
            }
        }
    }
}

// ham ho tro
wchar_t *cut_to_first_slash(wchar_t *wchar_t_str)
{
    wchar_t *slash_pos = wcschr(wchar_t_str, L'\\');

    if (slash_pos == NULL)
    {
        return wchar_t_str;
    }

    return slash_pos + 1;
}

// tim file
void findFileInRDET(LPCWCHAR drive, BYTE **entriesBuffer, DWORD entriesSize, PFAT32_BOOTSECTOR bs, BYTE *fatBuffer, DWORD fatSize, int indent, wchar_t *wPathToOpen)
{

    // tao copy cua path path name truoc khi split
    wchar_t *wPathToOpenClone = new wchar_t[wcslen(wPathToOpen) + 1];
    wcscpy(wPathToOpenClone, wPathToOpen);

    // cat ten duong dan de tim de quy
    wchar_t *splittedPathName;
    wchar_t *pTemp;
    splittedPathName = wcstok_s(wPathToOpen, L"\\", &pTemp);
    std::wstring wSplittedPathName(splittedPathName);
    std::wstring wPathToOpenString(wPathToOpen);

    for (int i = entriesSize - 1; i > 0; i--)
    {
        char fileName[14];
        char isLongName = 0;

        PFAT32_MAINENTRY me = (PFAT32_MAINENTRY)entriesBuffer[i];
        if (me->fileAttribute == 0x20 || (me->fileAttribute == 0x10 && entriesBuffer[i][0] != 0x2E))
        {
            for (int i = 0; i < indent; i++)
            {
                printf("  ");
            }
            for (int j = i - 1; j >= 0; j--)
            {
                PFAT32_SUBENTRY se = (PFAT32_SUBENTRY)entriesBuffer[j];

                // truong hop ten dai, can doc cac entry phu
                if (se->fileAttribute == 0x0F)
                {
                    isLongName = 1;

                    int c = 0;
                    for (int k = 0; k < 10; k++)
                    {
                        if (k % 2 == 0)
                        {
                            if (se->fileName1[k] == '\0')
                            {
                                break;
                            }
                            else
                            {
                                fileName[c++] = se->fileName1[k];
                            }
                        }
                    }
                    for (int k = 0; k < 12; k++)
                    {
                        if (k % 2 == 0 && se->fileName1[k] != '\0')
                        {
                            if (se->fileName1[k] == '\0')
                            {
                                break;
                            }
                            else
                            {
                                fileName[c++] = se->fileName2[k];
                            }
                        }
                    }
                    for (int k = 0; k < 4; k++)
                    {
                        if (k % 2 == 0 && se->fileName1[k] != '\0')
                        {
                            if (se->fileName1[k] == '\0')
                            {
                                break;
                            }
                            else
                            {
                                fileName[c++] = se->fileName3[k];
                            }
                        }
                    }
                    fileName[c] = '\0';

                    // neu la file va ten file la parameter o cuoi duong dan
                    if (me->fileAttribute != 0x10 && !wcsstr(wPathToOpenClone, L"\\"))
                    {
                        // convert file name to wcs to compare with the the one we found in sub-entry
                        std::string fileNameString(fileName);
                        std::wstring fileNameWString(fileNameString.begin(), fileNameString.end());

                        if (fileNameWString.compare(wSplittedPathName) == 0)
                        {
                            printf("Xem extension va xu ly mo file neu ten dai");
                        }
                    }
                    // printf("%s", fileName);

                    i--;
                }
                else
                {
                    break;
                }
            }

            // truong hop ten ngan, chi can doc entry chinh
            if (!isLongName)
            {
                int len = 0;
                for (int k = 0; k < 8; k++)
                {
                    if (me->fileName[k] != ' ')
                    {
                        fileName[len++] = me->fileName[k];
                    }
                    else
                    {
                        break;
                    }
                }

                if (me->fileAttribute != 0x10)
                {
                    fileName[len++] = '.';
                    for (int k = 0; k < 3; k++)
                    {
                        if (me->fileExtension[k] != ' ')
                        {
                            fileName[len++] = me->fileExtension[k];
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                fileName[len] = '\0';

                // neu la file va ten file nam o cuoi duong dan
                if (me->fileAttribute != 0x10 && !wcsstr(wPathToOpenClone, L"\\"))
                {
                    // convert file name to wcs to compare with the the one we found in sub-entry
                    std::string fileNameString(fileName);
                    std::wstring fileNameWString(fileNameString.begin(), fileNameString.end());

                    if (fileNameWString.compare(wSplittedPathName) == 0)
                    {
                        printf("Xem extension va xu ly mo file neu ten ngan");
                    }
                }
            }

            // neu la folder thi doc de quy truoc khi chuyen sang entry khac
            if (me->fileAttribute == 0x10)
            {
                int clusterNum = me->startingClusterLO;
                if (clusterNum < 2)
                {
                    break;
                }

                BYTE *dataBuffer = NULL;
                DWORD dataSize = bs->BPB_BytsPerSec * bs->BPB_SecPerClus + 1;
                dataBuffer = (BYTE *)malloc(dataSize * sizeof(BYTE));
                readCluster(drive, dataBuffer, dataSize - 1, bs, clusterNum);

                BYTE **entriesBuffer = NULL;
                DWORD entriesSize = getEntries(dataBuffer, dataSize, &entriesBuffer);

                wchar_t *wSubPath = cut_to_first_slash(wPathToOpenClone);
                findFileInRDET(drive, entriesBuffer, entriesSize, bs, fatBuffer, fatSize, indent + 1, wSubPath);

                DWORD *fat = (DWORD *)fatBuffer;
                memcpy_s(fat, fatSize - 1, fatBuffer, fatSize - 1);

                while (fat[clusterNum] != 0x00000000 && fat[clusterNum] != 0xfffffff7 && fat[clusterNum] != 0xfffffff8 && fat[clusterNum] != 0xffffffff && fat[clusterNum] != 0x0fffffff)
                {

                    // Don dep
                    free(dataBuffer);
                    dataBuffer = NULL;
                    for (int i = 0; i < entriesSize; i++)
                    {
                        free(entriesBuffer[i]);
                    }
                    free(entriesBuffer);
                    entriesBuffer = NULL;

                    // Lay cluster tiep theo
                    clusterNum = fat[clusterNum];

                    // Doc cluster
                    dataBuffer = NULL;
                    dataSize = bs->BPB_BytsPerSec * bs->BPB_SecPerClus + 1;
                    dataBuffer = (BYTE *)malloc(dataSize * sizeof(BYTE));
                    readCluster(drive, dataBuffer, dataSize - 1, bs, clusterNum);

                    // Doc va in cac entry
                    entriesBuffer = NULL;
                    entriesSize = getEntries(dataBuffer, dataSize, &entriesBuffer);
                    findFileInRDET(drive, entriesBuffer, entriesSize, bs, fatBuffer, fatSize, indent + 1, wSubPath);
                }

                // Don dep
                free(dataBuffer);
                for (int i = 0; i < entriesSize; i++)
                {
                    free(entriesBuffer[i]);
                }
                free(entriesBuffer);
            }
        }
    }
    // printf("Could not open file with this path file '%ls'", wPathToOpen);
}

//// Ham mo file
// void openFile(wchar_t* wDriveToOpen, wchar_t* wPathToOpen) {
//	while (1) {
//		printf("\nSelect file to open (eg. New Folder\\text.txt) - Ctrl + C to cancel: ");
//		fgetws(wPathToOpen, 102, stdin);
//		wPathToOpen[wcslen(wPathToOpen) - 1] = '\0';
//		wchar_t wFullPathToOpen[105] = L"\0";
//		wcscat_s(wFullPathToOpen, 105, wDriveToOpen);
//		wcscat_s(wFullPathToOpen, 105, wPathToOpen);
//		ShellExecute(NULL, NULL, wFullPathToOpen, NULL, NULL, SW_SHOW);
//	}
// }

/* Ham main */
int main(int argc, char **argv)
{

    // Doc volume
    char volume[2];
    printf("Input volume: ");
    scanf_s("%s", &volume, 2);
    clean_stdin();

    // Duong dan de doc file
    char driveToOpen[4];
    wchar_t wPathToOpen[102];
    strcpy_s(driveToOpen, 5, volume);
    strcat_s(driveToOpen, 5, ":\\");
    wchar_t wDriveToOpen[4];
    size_t outSize;
    mbstowcs_s(&outSize, wDriveToOpen, strlen(driveToOpen) + 1, driveToOpen, 5);

    // Chuyen kieu char* sang wchar_t*
    char drive[10];
    strcpy_s(drive, 10, "\\\\.\\");
    strcat_s(drive, 10, volume);
    strcat_s(drive, 10, ":");
    wchar_t wDrive[10];
    mbstowcs_s(&outSize, wDrive, strlen(drive) + 1, drive, 10);

    // Doc boot sector
    BYTE sectorBuffer[512];
    readBootSector(wDrive, 0, sectorBuffer);
    printTable(sectorBuffer);

    // In thong tin boot sector
    PFAT32_BOOTSECTOR bs = (PFAT32_BOOTSECTOR)sectorBuffer;
    bs->BS_OEMName[7] = '\0';
    bs->BS_FilSysType[7] = '\0';
    printBootSectorInformation(bs);

    printf("\n");

    // Doc FAT
    BYTE *fatBuffer = NULL;
    DWORD fatSize = bs->BPB_FATSz32 * 512 + 1;
    fatBuffer = (BYTE *)malloc(fatSize * sizeof(BYTE));
    readFAT(wDrive, fatBuffer, fatSize, bs);

    printf("\n");

    // Doc cluster
    int clusterNum = bs->BPB_RootClus;
    BYTE *dataBuffer = NULL;
    DWORD dataSize = bs->BPB_BytsPerSec * bs->BPB_SecPerClus + 1;
    dataBuffer = (BYTE *)malloc(dataSize * sizeof(BYTE));
    readCluster(wDrive, dataBuffer, dataSize - 1, bs, clusterNum);

    // Doc cac entry
    BYTE **entriesBuffer = NULL;
    DWORD entriesSize = getEntries(dataBuffer, dataSize, &entriesBuffer);

    // In cay thu muc hien tai
    printf("DIRECTORY TREE:\n");
    printDir(wDrive, entriesBuffer, entriesSize, bs, fatBuffer, fatSize, 0);

    DWORD *fat = (DWORD *)fatBuffer;
    memcpy_s(fat, fatSize - 1, fatBuffer, fatSize - 1);

    while (fat[clusterNum] != 0x00000000 && fat[clusterNum] != 0xfffffff7 && fat[clusterNum] != 0xfffffff8 && fat[clusterNum] != 0xffffffff && fat[clusterNum] != 0x0fffffff)
    {

        // Don dep
        free(dataBuffer);
        dataBuffer = NULL;
        for (int i = 0; i < entriesSize; i++)
        {
            free(entriesBuffer[i]);
        }
        free(entriesBuffer);
        entriesBuffer = NULL;

        // Lay cluster tiep theo
        clusterNum = fat[clusterNum];

        // Doc cluster
        dataBuffer = NULL;
        dataSize = bs->BPB_BytsPerSec * bs->BPB_SecPerClus + 1;
        dataBuffer = (BYTE *)malloc(dataSize * sizeof(BYTE));
        readCluster(wDrive, dataBuffer, dataSize - 1, bs, clusterNum);

        // Doc va in cac entry
        entriesBuffer = NULL;
        entriesSize = getEntries(dataBuffer, dataSize, &entriesBuffer);
        printDir(wDrive, entriesBuffer, entriesSize, bs, fatBuffer, fatSize, 0);
    }

    //// Goi ham mo file
    // openFile(wDriveToOpen, wPathToOpen);

    while (1)
    {
        printf("\nSelect file to open (eg. New Folder\\text.txt) - Ctrl + C to cancel: ");
        fgetws(wPathToOpen, 102, stdin);
        wPathToOpen[wcslen(wPathToOpen) - 1] = '\0';

        findFileInRDET(wDrive, entriesBuffer, entriesSize, bs, fatBuffer, fatSize, 0, wPathToOpen);
    }

    // Don dep
    free(fatBuffer);
    free(dataBuffer);
    for (int i = 0; i < entriesSize; i++)
    {
        free(entriesBuffer[i]);
    }
    free(entriesBuffer);
    return 0;
}
