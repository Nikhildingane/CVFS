//  New Features added to the project
//
//  1. cleaner() function is added to deallocate the allocated memory for resources                         //  done
//  2. optimization in GetFDFromName() function                                                             //  done
//  3. Implementation of cat command of linux                                                               //  done
//  4. Added filter in Get_Inode() function                                                                 //  done
//  5. Files backup on exiting and restore them after starting                                              //  done
//  6. shortcut of original filename and new file name accept and craeted new file with new file name       //  done
//  7. special file i.e. directory                                                                          //  pending
//  8. dynamically file size increment by double                                                            //  done
#include<stdio.h>
#include<fcntl.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>      // required on linux platform
#include<io.h>          //only for windows

#define MAXINODE 50     // number of files will be created

#define READ 1
#define WRITE 2

#define BLOCKSIZE 2014
#define MAXFILESIZE 2048

#define REGULAR 1
#define SPECIAL 2

#define START 0
#define CURRENT 1
#define END 2

typedef struct superblock
{
    int TotalInodes;
    int FreeInode;
}SUPERBLOCK,*PSUPERBLOCK;

typedef struct myfile
{
    char FileName[50];
    int FileSize;
    int FileActualSize;
    int FileType;
    int permission;
}MYFILE;

typedef struct inode        //self referencetial structure
{
    char FileName[50];
    int InodeNumber;
    int FileSize;           // 2048 bytes
    int FileActualSize;     // number of character in a file
    int FileType;           // regular or special
    char *Buffer;           // pointer which points to our actual file
    int LinkCount;          // links kiti ahet te dakhawanyasathi
    int ReferenceCount;     // number of users of file
    int permission;         //1(read) 2(write) 3(read and write)
    struct inode *next;     //pointer to next node
}INODE,*PINODE,**PPINODE;

typedef struct filetable
{
    int readoffset;
    int writeoffset;
    int count;              // always 1
    int mode;//1 2 3
    PINODE ptrinode;        // this pointer is used to point the inode of iit
}FILETABLE,*PFILETABLE;

typedef struct ufdt         // array which stores the address of filetable
{
    PFILETABLE ptrfiletable;
}UFDT;

UFDT UFDTArr[MAXINODE];         //ufdt  global
SUPERBLOCK SUPERBLOCKobj;       // global object of superblock structure
PINODE head=NULL;               // global

void man(char *name)
{
    if(name==NULL)
    {
        return;
    }

    if(strcmp(name,"create") == 0)
    {
        printf("Description : Used to create new regular file\n");
        printf("Usage : create File_name Permission\n");
    }
    else if(strcmp(name,"read") == 0)
    {
        printf("Description : Used to read data from regular file\n");
        printf("Usage : read File_name No_Of_Bytes_To_Read\n");
    }
    else if(strcmp(name,"write") == 0)
    {
        printf("Description : Used to write into regular file\n");
        printf("Usage : write File_name\n After this enter the data that we want to write\n");
    }
    else if(strcmp(name,"ls") == 0)
    {
        printf("Description : Used to list all information of files\n");
        printf("Usage : ls\n");
    }
    else if(strcmp(name,"stat") == 0)
    {
        printf("Description : Used to display information of file\n");
        printf("Usage : stat File_name\n");
    }
    else if(strcmp(name,"fstat") == 0)
    {
        printf("Description : Used to display information of file\n");
        printf("Usage : stat File_Descriptor\n");
    }
    else if(strcmp(name,"truncate") == 0)
    {
        printf("Description : Used to remove data from file\n");
        printf("Usage : truncate File_name\n");
    }
    else if(strcmp(name,"open") == 0)
    {
        printf("Description : Used to open existing file\n");
        printf("Usage : open File_name mode\n");
    }
    else if(strcmp(name,"close") == 0)
    {
        printf("Description : Used to close opened file\n");
        printf("Usage : close File_name\n");
    }
    else if(strcmp(name,"closeall") == 0)
    {
        printf("Description : Used to close all opened file\n");
        printf("Usage : closeall\n");
    }
    else if(strcmp(name,"lseek") == 0)
    {
        printf("Description : Used to change file offset\n");
        printf("Usage : lseek File_Name ChangeInOffset StartPoint\n");
    }
    else if(strcmp(name,"rm") == 0)
    {
        printf("Description : Used to delete the file\n");
        printf("Usage : rm File_Name\n");
    }
    else
    {
        printf("ERROR : No manual entry available.\n");
    }
}

void DisplayHelp()
{
    printf("ls : To List out all files\n");
    printf("clear : To clear console\n");
    printf("open : To open the file\n");
    printf("close : To close the file\n");
    printf("closeall : To close all opened files\n");
    printf("create : To create new file\n");
    printf("read : To Read the contents from file\n");
    printf("write :To Write contents into file\n");
    printf("exit : To Terminate file system\n");
    printf("stat : To Display information of file using name\n");
    printf("fstat :To Display information of file using file descriptor\n");
    printf("truncate : To Remove all data from file\n");
    printf("rm : To Delet the file\n");
}

int GetFDFromName(char *name)
{
    int i = 0;
    int freeinode=SUPERBLOCKobj.FreeInode;

    while((i<MAXINODE)&&(freeinode>0))
    {
        if(UFDTArr[i].ptrfiletable != NULL)
        {
            if(strcmp((UFDTArr[i].ptrfiletable->ptrinode->FileName),name)==0)
            {
                break;
            }
            freeinode--;
        }
        i++;
    }

    if((i == MAXINODE)||(freeinode==0))
    {
        return -1;
    }
    else return i;
}

PINODE Get_Inode(char * name)
{
    PINODE temp = head;
    int i = 0;
    if(SUPERBLOCKobj.FreeInode==0)
    {
        return temp;
    }
    if(name == NULL)
    {
        return NULL;
    }

    while(temp!= NULL)
    {
        if(strcmp(name,temp->FileName) == 0)
        {
            break;
        }
        temp = temp->next;
    }
    return temp;
}

void CreateDILB()
{
    int i = 1;
    PINODE newn = NULL;
    PINODE temp = head;

    while(i<= MAXINODE)
    {
        newn = (PINODE)malloc(sizeof(INODE));

        newn->LinkCount =0;
        newn->ReferenceCount = 0;
        newn->FileType = 0;
        newn->FileSize = 0;

        newn->Buffer = NULL;
        newn->next = NULL;

        newn->InodeNumber = i;

        if(temp == NULL)
        {
            head = newn;
            temp = head;
        }
        else
        {
            temp->next = newn;
            temp = temp->next;
        }
        i++;
    }
    printf("DILB created successfully\n");
}

void InitialiseSuperBlock()
{
    int i = 0;
    while(i< MAXINODE)
    {
        UFDTArr[i].ptrfiletable = NULL;
        i++;
    }

    SUPERBLOCKobj.TotalInodes = MAXINODE;       // member of superblock set to max value i.e. 50
    SUPERBLOCKobj.FreeInode = MAXINODE;
}

int CreateFile(char *name,int permission)
{
    int i = 0;
    PINODE temp = head;

    if((name == NULL) || (permission == 0) || (permission > 3))
    {
        return -1;
    }

    if(SUPERBLOCKobj.FreeInode == 0)
    {
        return -2;
    }
    (SUPERBLOCKobj.FreeInode)--;

    if(Get_Inode(name) != NULL)
     {
         return -3;
     }

    while(temp!= NULL)
    {
        if(temp->FileType == 0)
        {
            break;
        }
        temp=temp->next;
    }

    while(i<MAXINODE)
    {
        if(UFDTArr[i].ptrfiletable == NULL)
        {
            break;
        }
        i++;
    }

    UFDTArr[i].ptrfiletable = (PFILETABLE)malloc(sizeof(FILETABLE));

    UFDTArr[i].ptrfiletable->count = 1;
    UFDTArr[i].ptrfiletable->mode = permission;
    UFDTArr[i].ptrfiletable->readoffset = 0;
    UFDTArr[i].ptrfiletable->writeoffset = 0;

    UFDTArr[i].ptrfiletable->ptrinode = temp;

    strcpy(UFDTArr[i].ptrfiletable->ptrinode->FileName,name);
    UFDTArr[i].ptrfiletable->ptrinode->FileType = REGULAR;
    UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount = 1;
    UFDTArr[i].ptrfiletable->ptrinode->LinkCount = 1;
    UFDTArr[i].ptrfiletable->ptrinode->FileSize = MAXFILESIZE;
    UFDTArr[i].ptrfiletable->ptrinode->FileActualSize = 0;
    UFDTArr[i].ptrfiletable->ptrinode->permission = permission;
    UFDTArr[i].ptrfiletable->ptrinode->Buffer = (char *)malloc(MAXFILESIZE);

    return i;
}

// rm_File("Demo.txt")
int rm_File(char *name)
{
    int fd = 0;

    fd = GetFDFromName(name);
    if(fd == -1)
    {
         return -1;
    }

    (UFDTArr[fd].ptrfiletable->ptrinode->LinkCount)--;

    if(UFDTArr[fd].ptrfiletable->ptrinode->LinkCount == 0)
    {
        UFDTArr[fd].ptrfiletable->ptrinode->FileType = 0;
        free(UFDTArr[fd].ptrfiletable->ptrinode->Buffer);
        strcpy(UFDTArr[fd].ptrfiletable->ptrinode->FileName,"");
        UFDTArr[fd].ptrfiletable->ptrinode->ReferenceCount=0;
        UFDTArr[fd].ptrfiletable->ptrinode->permission=0;
        UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize=0;
        free(UFDTArr[fd].ptrfiletable);
    }
    UFDTArr[fd].ptrfiletable = NULL;
    (SUPERBLOCKobj.FreeInode)++;
}

int ReadFile(int fd, char *arr, int isize)
{
    int read_size = 0;

    if(UFDTArr[fd].ptrfiletable == NULL)
    {
        return -1;
    }

    if(UFDTArr[fd].ptrfiletable->mode !=READ && UFDTArr[fd].ptrfiletable->mode !=READ+WRITE)
    {
        return -2;
    }
    if(UFDTArr[fd].ptrfiletable->ptrinode->permission != READ && UFDTArr[fd].ptrfiletable->ptrinode->permission != READ+WRITE)
    {
        return -2;
    }

    if(UFDTArr[fd].ptrfiletable->readoffset == UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize)
    {
        return -3;
    }

    if(UFDTArr[fd].ptrfiletable->ptrinode->FileType != REGULAR)
    {
        return -4;
    }

    read_size = (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) - (UFDTArr[fd].ptrfiletable->readoffset);
    if(read_size < isize)
    {
        strncpy(arr,(UFDTArr[fd].ptrfiletable->ptrinode->Buffer) + (UFDTArr[fd].ptrfiletable->readoffset),read_size);

        UFDTArr[fd].ptrfiletable->readoffset = UFDTArr[fd].ptrfiletable->readoffset + read_size;
    }
    else
    {
        strncpy(arr,(UFDTArr[fd].ptrfiletable->ptrinode->Buffer) + (UFDTArr[fd].ptrfiletable->readoffset),isize);

        (UFDTArr[fd].ptrfiletable->readoffset) = (UFDTArr[fd].ptrfiletable->readoffset) + isize;
    }

    return isize;
}

int WriteFile(int fd, char *arr, int isize)
{
    if(((UFDTArr[fd].ptrfiletable->mode) !=WRITE) && ((UFDTArr[fd].ptrfiletable->mode) !=READ+WRITE))
    {
        return -1;
    }

    if(((UFDTArr[fd].ptrfiletable->ptrinode->permission) !=WRITE) && ((UFDTArr[fd].ptrfiletable->ptrinode->permission) != READ+WRITE))
    {
        return -1;
    }

    if((UFDTArr[fd].ptrfiletable->writeoffset) == (UFDTArr[fd].ptrfiletable->ptrinode->FileSize))
    {
        //return -2;
        UFDTArr[fd].ptrfiletable->ptrinode->Buffer=(char *)realloc(UFDTArr[fd].ptrfiletable->ptrinode->Buffer,UFDTArr[fd].ptrfiletable->ptrinode->FileSize+(UFDTArr[fd].ptrfiletable->ptrinode->FileSize*2));
        UFDTArr[fd].ptrfiletable->ptrinode->FileSize=UFDTArr[fd].ptrfiletable->ptrinode->FileSize+(UFDTArr[fd].ptrfiletable->ptrinode->FileSize*2);
    }
    if((UFDTArr[fd].ptrfiletable->ptrinode->FileType) != REGULAR)
    {
        return -3;
    }
    if(((UFDTArr[fd].ptrfiletable->ptrinode->FileSize)-(UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize)<isize))
    {
        //return -4;
        UFDTArr[fd].ptrfiletable->ptrinode->Buffer=(char *)realloc(UFDTArr[fd].ptrfiletable->ptrinode->Buffer,UFDTArr[fd].ptrfiletable->ptrinode->FileSize+(UFDTArr[fd].ptrfiletable->ptrinode->FileSize*2));
        UFDTArr[fd].ptrfiletable->ptrinode->FileSize=UFDTArr[fd].ptrfiletable->ptrinode->FileSize+(UFDTArr[fd].ptrfiletable->ptrinode->FileSize*2);
    }
    strncpy((UFDTArr[fd].ptrfiletable->ptrinode->Buffer) + (UFDTArr[fd].ptrfiletable->writeoffset),arr,isize);

    (UFDTArr[fd].ptrfiletable->writeoffset) = (UFDTArr[fd].ptrfiletable->writeoffset )+ isize;

    (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) = (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) + isize;

    return isize;
}

int OpenFile(char *name, int mode)
{
    int i = 0;
    PINODE temp = NULL;

    if(name == NULL || mode <= 0)
    {
        return -1;
    }

    temp = Get_Inode(name);
    if(temp == NULL)
    {
        return -2;
    }

    if(temp->permission < mode)
    {
        return -3;
    }

    while(i<MAXINODE)
    {
        if(UFDTArr[i].ptrfiletable == NULL)
        {
            break;
        }
        i++;
    }

    UFDTArr[i].ptrfiletable = (PFILETABLE)malloc(sizeof(FILETABLE));
    if(UFDTArr[i].ptrfiletable == NULL) return -1;
    UFDTArr[i].ptrfiletable->count = 1;
    UFDTArr[i].ptrfiletable->mode = mode;
    if(mode == READ + WRITE)
    {
        UFDTArr[i].ptrfiletable->readoffset = 0;
        UFDTArr[i].ptrfiletable->writeoffset = 0;
    }
    else if(mode == READ)
    {
        UFDTArr[i].ptrfiletable->readoffset = 0;
    }
    else if(mode == WRITE)
    {
        UFDTArr[i].ptrfiletable->writeoffset = 0;
    }
        UFDTArr[i].ptrfiletable->ptrinode = temp;
        (UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount)++;

    return i;
}

void CloseFileByfd(int fd)
{
    UFDTArr[fd].ptrfiletable->readoffset = 0;
    UFDTArr[fd].ptrfiletable->writeoffset = 0;
    (UFDTArr[fd].ptrfiletable->ptrinode->ReferenceCount)--;
}

int CloseFileByName(char *name)
{
    int i = 0;
    i = GetFDFromName(name);
    if(i == -1)
    {
        return -1;
    }

    UFDTArr[i].ptrfiletable->readoffset = 0;
    UFDTArr[i].ptrfiletable->writeoffset = 0;
    (UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount)--;

    return 0;
}

void CloseAllFile()
{
    int i = 0;
    while(i<MAXINODE)
    {
        if(UFDTArr[i].ptrfiletable != NULL)
        {
            UFDTArr[i].ptrfiletable->readoffset = 0;
            UFDTArr[i].ptrfiletable->writeoffset = 0;
            (UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount)=0;
        }
        i++;
    }
}

int LseekFile(int fd, int size, int from)
{
    if((fd<0) || (from > 2))
    {
        return -1;
    }
    if(UFDTArr[fd].ptrfiletable == NULL)
    {
        return -1;
    }

    if((UFDTArr[fd].ptrfiletable->mode == READ) || (UFDTArr[fd].ptrfiletable->mode ==READ+WRITE))
    {
        if(from == CURRENT)
        {
            if(((UFDTArr[fd].ptrfiletable->readoffset) + size) > UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize)
            {
                return -1;
            }
            if(((UFDTArr[fd].ptrfiletable->readoffset) + size) < 0)
            {
                return -1;
            }
            (UFDTArr[fd].ptrfiletable->readoffset) = (UFDTArr[fd].ptrfiletable->readoffset) +size;
        }
        else if(from == START)
        {
            if(size > (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize))
            {
                return -1;
            }
            if(size < 0)
            {
                return -1;
            }
            (UFDTArr[fd].ptrfiletable->readoffset) = size;
        }
        else if(from == END)
        {
            if((UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) + size > UFDTArr[fd].ptrfiletable->ptrinode->FileSize)
            {
                return -1;
            }
            if(((UFDTArr[fd].ptrfiletable->readoffset) + size) < 0)
            {
                return -1;
            }
            (UFDTArr[fd].ptrfiletable->readoffset) = (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) + size;
        }
    }
    else if(UFDTArr[fd].ptrfiletable->mode == WRITE)
    {
        if(from == CURRENT)
        {
            if(((UFDTArr[fd].ptrfiletable->writeoffset) + size) > UFDTArr[fd].ptrfiletable->ptrinode->FileSize)
            {
                //return -1;
                UFDTArr[fd].ptrfiletable->ptrinode->Buffer=(char *)realloc(UFDTArr[fd].ptrfiletable->ptrinode->Buffer,UFDTArr[fd].ptrfiletable->ptrinode->FileSize+(MAXFILESIZE*2));
                UFDTArr[fd].ptrfiletable->ptrinode->FileSize=UFDTArr[fd].ptrfiletable->ptrinode->FileSize+(MAXFILESIZE*2);
            }
            if(((UFDTArr[fd].ptrfiletable->writeoffset) + size) < 0)
            {
                return -1;
            }
            if(((UFDTArr[fd].ptrfiletable->writeoffset) + size) > (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize))
            (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) =(UFDTArr[fd].ptrfiletable->writeoffset) + size;
            (UFDTArr[fd].ptrfiletable->writeoffset) = (UFDTArr[fd].ptrfiletable->writeoffset) +size;
        }
        else if(from == START)
        {
            if(size < 0)
            {
                return -1;
            }
            if(size > MAXFILESIZE)
            {
                //return -1;
                UFDTArr[fd].ptrfiletable->ptrinode->Buffer=(char *)realloc(UFDTArr[fd].ptrfiletable->ptrinode->Buffer,UFDTArr[fd].ptrfiletable->ptrinode->FileSize+(MAXFILESIZE*2));
                UFDTArr[fd].ptrfiletable->ptrinode->FileSize=UFDTArr[fd].ptrfiletable->ptrinode->FileSize+(MAXFILESIZE*2);
            }

            if(size > (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize))
            {
                (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) = size;
                (UFDTArr[fd].ptrfiletable->writeoffset) = size;
            }
        }
        else if(from == END)
        {
            if((UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) + size > MAXFILESIZE)
            {
               // return -1;
               UFDTArr[fd].ptrfiletable->ptrinode->Buffer=(char *)realloc(UFDTArr[fd].ptrfiletable->ptrinode->Buffer,UFDTArr[fd].ptrfiletable->ptrinode->FileSize+(MAXFILESIZE*2));
                UFDTArr[fd].ptrfiletable->ptrinode->FileSize=UFDTArr[fd].ptrfiletable->ptrinode->FileSize+(MAXFILESIZE*2);
            }
            if(((UFDTArr[fd].ptrfiletable->writeoffset) + size) < 0)
            {
                return -1;
            }
            (UFDTArr[fd].ptrfiletable->writeoffset) = (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) + size;
        }
    }
}

void ls_file()
{
    int i = 0;
    PINODE temp = head;

    if(SUPERBLOCKobj.FreeInode == MAXINODE)
    {
        printf("Error : There are no files\n");
        return;
    }
    //printf("%d\t",head);
    printf("\nFile Name\tInode number\tFile size\tLink count\n");
    printf("---------------------------------------------------------------\n");
    while(temp != NULL)
    {
        if(temp->FileType != 0)
        {
            printf("%s\t\t%d\t\t%d\t\t%d\n",temp->FileName,temp->InodeNumber,temp->FileActualSize,temp->LinkCount);
        }
        temp = temp->next;
    }
    printf("---------------------------------------------------------------\n");
}

int fstat_file(int fd)
{
    PINODE temp = head;
    int i = 0;

    if(fd < 0)
    {
        return -1;
    }

    if(UFDTArr[fd].ptrfiletable == NULL)
    {
        return -2;
    }

    temp = UFDTArr[fd].ptrfiletable->ptrinode;
    if(temp->FileType==0)
    {
        return -2;        // modified filter
    }
    printf("\n---------Statistical Information about file----------\n");
    printf("File name : %s\n",temp->FileName);
    printf("Inode Number %d\n",temp->InodeNumber);
    printf("File size : %d\n",temp->FileSize);
    printf("Actual File size : %d\n",temp->FileActualSize);
    printf("Link count : %d\n",temp->LinkCount);
    printf("Reference count : %d\n",temp->ReferenceCount);

    if(temp->permission == 1)
    {
        printf("File Permission : Read only\n");
    }
    else if(temp->permission == 2)
    {
        printf("File Permission : Write\n");
    }
    else if(temp->permission == 3)
    {
        printf("File Permission : Read & Write\n");
    }
    printf("------------------------------------------------------\n\n");

    return 0;
}

int stat_file(char *name)
{
    PINODE temp = head;
    int i = 0;

    if(name == NULL)
    {
        return -1;
    }

    while(temp!= NULL)
    {
        if(strcmp(name,temp->FileName) == 0)
        {
            break;
        }
        temp = temp->next;
    }

    if(temp == NULL) return -2;

    printf("\n---------Statistical Information about file----------\n");
    printf("File name : %s\n",temp->FileName);
    printf("Inode Number %d\n",temp->InodeNumber);
    printf("File size : %d\n",temp->FileSize);
    printf("Actual File size : %d\n",temp->FileActualSize);
    printf("Link count : %d\n",temp->LinkCount);
    printf("Reference count : %d\n",temp->ReferenceCount);

    if(temp->permission == 1)
    {
        printf("File Permission : Read only\n");
    }
    else if(temp->permission == 2)
    {
        printf("File Permission : Write\n");
    }
    else if(temp->permission == 3)
    {
        printf("File Permission : Read & Write\n");
    }
    printf("------------------------------------------------------\n\n");

    return 0;
}

int truncate_File(char *name)
{
    int fd = GetFDFromName(name);
    if(fd == -1)
    {
        return -1;
    }

    memset(UFDTArr[fd].ptrfiletable->ptrinode->Buffer,0,UFDTArr[fd].ptrfiletable->ptrinode->FileSize);
    UFDTArr[fd].ptrfiletable->readoffset = 0;
    UFDTArr[fd].ptrfiletable->writeoffset = 0;
    UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize = 0;
}

int cat(int fd)
{
    int i=0;
    if(UFDTArr[fd].ptrfiletable == NULL)
    {
        return -1;
    }

    if(UFDTArr[fd].ptrfiletable->mode !=READ && UFDTArr[fd].ptrfiletable->mode !=READ+WRITE)
    {
        return -2;
    }
    if(UFDTArr[fd].ptrfiletable->ptrinode->permission != READ && UFDTArr[fd].ptrfiletable->ptrinode->permission != READ+WRITE)
    {
        return -2;
    }
    if(UFDTArr[fd].ptrfiletable->ptrinode->FileType != REGULAR)
    {
        return -4;
    }
    char *arr=UFDTArr[fd].ptrfiletable->ptrinode->Buffer;
    for(i=0;i<UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize;i++)
    {
        printf("%c",*arr);
        arr++;
    }
}

void cleaner()
{
    PINODE temp=head;
    int i=0;
    for(i=0;i<MAXINODE;i++)
    {
        if(UFDTArr[i].ptrfiletable!=NULL)
        {
            free(UFDTArr[i].ptrfiletable);
        }
        head=head->next;
        if(temp->Buffer!=NULL)
        {
            free(temp->Buffer);
        }
        free(temp);
        temp=head;
    }
    printf("Files cleared successfully......\n");
}

void backup()
{
    int i = 0,j=0,fd=-1,ret=-1;
    int freeinode=SUPERBLOCKobj.FreeInode;
    char *arr=NULL,temp=NULL;
    MYFILE obj;
    fd=open("backup.txt",O_RDWR|O_CREAT|O_TRUNC);
    if(fd==-1)
    {
        printf("Unable to create backup file");
        return;
    }
    arr=(char *)malloc(MAXFILESIZE);
    while((i<MAXINODE)&&(freeinode>0))
    {
        if(UFDTArr[i].ptrfiletable != NULL)
        {
            strcpy(obj.FileName,UFDTArr[i].ptrfiletable->ptrinode->FileName);
            obj.FileActualSize=UFDTArr[i].ptrfiletable->ptrinode->FileActualSize;
            obj.FileSize=UFDTArr[i].ptrfiletable->ptrinode->FileSize;
            obj.FileType=UFDTArr[i].ptrfiletable->ptrinode->FileType;
            obj.permission=UFDTArr[i].ptrfiletable->ptrinode->permission;
            arr=UFDTArr[i].ptrfiletable->ptrinode->Buffer;
            ret=write(fd,&obj,sizeof(obj));
            if(ret==-1)
            {
                printf("Error : Unable to write data on backup file");
                break;
            }
            for(j=0;j<UFDTArr[i].ptrfiletable->ptrinode->FileActualSize;j++)
            {
                //arr[j]=(UFDTArr[i].ptrfiletable->ptrinode->Buffer)[j];
                printf("%c",arr[j]);
            }
            ret=write(fd,arr,UFDTArr[i].ptrfiletable->ptrinode->FileActualSize);
            memset(arr,0,MAXFILESIZE);
            freeinode--;
        }
        i++;
    }
    printf("Backing up files.......\nDone\n");
}

void Restore()
{
    int fd=-1,fd2=0,ret=-1,i=0,j=0;
    char *ch=(char *)malloc(BLOCKSIZE);
    MYFILE obj;

    fd=open("backup.txt",O_RDONLY);
    if(fd==-1)
    {
        printf("Unable to restore files");
		return;
    }
    //ret=read(fd,&obj,sizeof(MYFILE));
    while((ret=read(fd,&obj,sizeof(MYFILE)))>0)
    {
        memset(ch,0,BLOCKSIZE);
        ret=read(fd,ch,obj.FileActualSize);
        CreateFile(obj.FileName,obj.permission);
        fd2=GetFDFromName(obj.FileName);
        WriteFile(fd2,ch,obj.FileActualSize);

    }
    printf("Restoring files....\nDone\n");
}

void clone(int fd,char *newfile)
{
    int ret=CreateFile(newfile,UFDTArr[fd].ptrfiletable->ptrinode->permission);
    if(ret >= 0)
    {
        printf("Shortcut is successfully created with file descriptor : %d\n",ret);
    }
    if(ret == -1)
    {
        printf("ERROR : Incorrect parameters\n");
    }
    if(ret == -2)
    {
        printf("ERROR : There is no inodes\n");
    }
    if(ret == -3)
    {
        printf("ERROR : File already exists\n");
    }
    if(ret == -4)
    {
        printf("ERROR : Memory allocation failure\n");
    }
    ret=WriteFile(ret,UFDTArr[fd].ptrfiletable->ptrinode->Buffer,UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize);
    {
        printf("Shortcut File successfully written");
    }
    if(ret == -1)
    {
        printf("ERROR : Permission denied\n");
    }
    if(ret == -2)
    {
        printf("ERROR : There is no sufficient memory to write\n");
    }
    if(ret == -3)
    {
        printf("ERROR : It is not regular file\n");
    }
    if(ret == -4)
    {
        printf("ERROR : There no sufficient memory available");
    }
}

int main()
{
    char *ptr = NULL;
    int ret = 0, fd = 0, count = 0;
    char command[4][80], str[80], arr[MAXFILESIZE];

    InitialiseSuperBlock();
    CreateDILB();
    Restore();
    while(1)
    {
        fflush(stdin);
        strcpy(str,"");

        printf("\nMarvellous VFS : > ");

        fgets(str,80,stdin);// scanf("%[^'\n']s",str);

        count = sscanf(str,"%s %s %s %s",command[0],command[1],command[2],command[3]);

        if(count == 1)
        {
            if(strcmp(command[0],"ls") == 0)
            {
                ls_file();
            }
            else if(strcmp(command[0],"closeall") == 0)
            {
                CloseAllFile();
                printf("All files closed successfully\n");
                continue;
            }
            else if(strcmp(command[0],"clear") == 0)
            {
                system("cls");
                continue;
            }
            else if(strcmp(command[0],"help") == 0)
            {
                DisplayHelp();
                continue;
            }
            else if(strcmp(command[0],"exit") == 0)
            {
                backup();
                cleaner();
                printf("Terminating the Marvellous Virtual File System\n");
                break;
            }
            else
            {
                printf("\nERROR : Command not found !!!\n");
                continue;
            }
        }
        else if(count == 2)
        {
            if(strcmp(command[0],"stat") == 0)                      // stat Demo.txt
            {
                ret = stat_file(command[1]);
                if(ret == -1)
                {
                    printf("ERROR : Incorrect parameters\n");
                }
                if(ret == -2)
                {
                    printf("ERROR : There is no such file\n");
                }
                continue;
            }
            else if(strcmp(command[0],"fstat") == 0)                //fstat 0
            {
                ret = fstat_file(atoi(command[1]));             //ascii to integer conversion
                if(ret == -1)
                {
                    printf("ERROR : Incorrect parameters\n");
                }
                if(ret == -2)
                {
                    printf("ERROR : There is no such file\n");
                }
                continue;
            }
            else if(strcmp(command[0],"close") == 0)
            {
                ret = CloseFileByName(command[1]);         // call close 0
                if(ret == -1)
                {
                    printf("ERROR : There is no such file\n");
                }
                continue;
            }
            else if(strcmp(command[0],"rm") == 0)
            {
                ret = rm_File(command[1]);
                if(ret == -1)
                {
                    printf("ERROR : There is no such file\n");
                }
                continue;
            }
            else if(strcmp(command[0],"man") == 0)
            {
                man(command[1]);
            }
            else if(strcmp(command[0],"write") == 0)
            {
                fd = GetFDFromName(command[1]);
                if(fd == -1)
                {
                    printf("Error : Incorrect parameter\n");
                    continue;
                }
                printf("Enter the data : \n");
                fflush(stdin);
                scanf("%[^\n]",arr);        //"abcd"
                ret = strlen(arr);
                if(ret == 0)
                {
                    printf("Error : Incorrect parameter\n");
                    continue;
                }
                ret = WriteFile(fd,arr,ret);
                if(ret >0)
                {
                    printf("File successfully written");
                }
                if(ret == -1)
                {
                    printf("ERROR : Permission denied\n");
                }
                if(ret == -2)
                {
                    printf("ERROR : There is no sufficient memory to write\n");
                }
                if(ret == -3)
                {
                    printf("ERROR : It is not regular file\n");
                }
                if(ret == -4)
                {
                    printf("ERROR : There no sufficient memory available");
                }
            }
            else if(strcmp(command[0],"truncate") == 0)
            {
                ret = truncate_File(command[1]);
                if(ret == -1)
                {
                    printf("Error : Incorrect parameter\n");
                }
                continue;
            }
            else if(strcmp(command[0],"cat")==0)
            {
                int ret=0;
                fd = GetFDFromName(command[1]);
                if(fd == -1)
                {
                    printf("Error : Incorrect parameter\n");
                    continue;
                }
                ret=cat(fd);
                if(ret == -1)
                {
                    printf("ERROR : File not existing\n");
                }
                if(ret == -2)
                {
                    printf("ERROR : Permission denied\n");
                }
                if(ret == -4)
                {
                    printf("ERROR : It is not regular file\n");
                }
                continue;
            }
            else
            {
                printf("\nERROR : Command not found !!!\n");
                continue;
            }
        }
        else if(count == 3)
        {
            if(strcmp(command[0],"create") == 0)
            {
                ret = CreateFile(command[1],atoi(command[2]));
                if(ret >= 0)
                {
                    printf("File is successfully created with file descriptor : %d\n",ret);
                }
                if(ret == -1)
                {
                    printf("ERROR : Incorrect parameters\n");
                }
                if(ret == -2)
                {
                    printf("ERROR : There is no inodes\n");
                }
                if(ret == -3)
                {
                    printf("ERROR : File already exists\n");
                }
                if(ret == -4)
                {
                    printf("ERROR : Memory allocation failure\n");
                }
                continue;
            }
            else if(strcmp(command[0],"open") == 0)
            {
                ret = OpenFile(command[1],atoi(command[2]));
                if(ret >= 0)
                {
                    printf("File is successfully opened with file descriptor : %d\n",ret);
                }
                if(ret == -1)
                {
                    printf("ERROR : Incorrect parameters\n");
                }
                if(ret == -2)
                {
                    printf("ERROR : File not present\n");
                }
                if(ret == -3)
                {
                    printf("ERROR : Permission denied\n");
                }
                continue;
            }
            else if(strcmp(command[0],"read") == 0)
            {
                fd = GetFDFromName(command[1]);
                if(fd == -1)
                {
                    printf("Error : Incorrect parameter\n");
                    continue;
                }
                ptr = (char *)malloc(sizeof(atoi(command[2]))+1);
                if(ptr == NULL)
                {
                    printf("Error : Memory allocation failure\n");
                    continue;
                }
                ret = ReadFile(fd,ptr,atoi(command[2]));
                if(ret == -1)
                {
                    printf("ERROR : File not existing\n");
                }
                if(ret == -2)
                {
                    printf("ERROR : Permission denied\n");
                }
                if(ret == -3)
                {
                    printf("ERROR : Reached at end of file\n");
                }
                if(ret == -4)
                {
                    printf("ERROR : It is not regular file\n");
                }
                if(ret == 0)
                {
                    printf("ERROR : File empty\n");
                }
                if(ret > 0)
                {
                    write(1,ptr,ret);// its like printf()
                }
                continue;
            }
            else if(strcmp(command[0],"shortcut")==0)
            {
                int fd=GetFDFromName(command[1]);

                if(fd == -1)
                {
                    printf("Error : Incorrect parameter\n");
                    continue;
                }
                clone(fd,command[2]);
            }
            else
            {
                printf("\nERROR : Command not found !!!\n");
                continue;
            }
        }
        else if(count == 4)
        {
            if(strcmp(command[0],"lseek") == 0)
            {
                fd = GetFDFromName(command[1]);
                if(fd == -1)
                {
                    printf("Error : Incorrect parameter\n");
                    continue;
                }
                ret = LseekFile(fd,atoi(command[2]),atoi(command[3]));
                if(ret == -1)
                {
                    printf("ERROR : Unable to perform lseek\n");
                }
            }
            else
            {
                printf("\nERROR : Command not found !!!\n");
                continue;
            }
        }
        else
        {
            printf("\nERROR : Command not found !!!\n");
            continue;
        }
    }
    return 0;
}
