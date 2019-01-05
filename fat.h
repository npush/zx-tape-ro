//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//�������� � FAT
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//BOOT-������ � ��������� BPB
#define BS_jmpBoot			0
#define BS_OEMName		3
#define BPB_BytsPerSec		11
#define BPB_SecPerClus		13
#define BPB_ResvdSecCnt		14
#define BPB_NumFATs		16
#define BPB_RootEntCnt		17
#define BPB_TotSec16		19
#define BPB_Media			21
#define BPB_FATSz16		22
#define BPB_SecPerTrk		24
#define BPB_NumHeads		26
#define BPB_HiddSec		28
#define BPB_TotSec32		32
#define BS_DrvNum			36
#define BS_Reserved1		37
#define BS_BootSig			38
#define BS_VolID			39
#define BS_VolLab			43
#define BS_FilSysType		54
#define BPB_FATSz32		36
#define BPB_ExtFlags		40
#define BPB_FSVer			42
#define BPB_RootClus		44
#define BPB_FSInfo			48
#define BPB_BkBootSec		50
#define BPB_Reserved		52

//��� �������� �������
#define FAT12 0
#define FAT16 1
#define FAT32 2

//��������� �������
#define FAT16_EOC 0xFFF8UL

//�������� �����
#define ATTR_READ_ONLY		0x01
#define ATTR_HIDDEN 			0x02
#define ATTR_SYSTEM 			0x04
#define ATTR_VOLUME_ID 		0x08
#define ATTR_DIRECTORY			0x10
#define ATTR_ARCHIVE  			0x20
#define ATTR_LONG_NAME 		(ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)


//----------------------------------------------------------------------------------------------------
//���������� ����������
//----------------------------------------------------------------------------------------------------
unsigned char Sector[512];//������ ��� �������
unsigned long LastReadSector=0xffffffffUL;//��������� ��������� ������
unsigned long FATOffset=0;//�������� FAT

//��������� ���� ������ ������ ������ ��������
struct SFATRecordPointer
{
    unsigned long BeginFolderAddr;//��������� ����� ��� ������ ������ ����������
    unsigned long CurrentFolderAddr;//������� ����� ��� ������ ������ ����������
    unsigned long BeginFolderCluster;//��������� ������� ����� ����� ������ ����������
    unsigned long CurrentFolderCluster;//������� ������� ����� ����� ������ ����������
    unsigned long BeginFolderClusterAddr;//��������� ����� �������� ��������
    unsigned long EndFolderClusterAddr;//�������� ����� ��� ������ ������ ���������� (��� ��������)
} sFATRecordPointer;

unsigned long FirstRootFolderAddr;//��������� ����� �������� ����������

unsigned long SecPerClus;//���������� �������� � ��������
unsigned long BytsPerSec;//���������� ���� � �������
unsigned long ResvdSecCnt;//������ ��������� �������

unsigned long FATSz;//������ ������� FAT
unsigned long DataSec;//���������� �������� � ������� ������ �����
unsigned long RootDirSectors;//���������� ��������, ������� �������� �����������
unsigned long CountofClusters;//���������� ��������� ��� ������ (������� ���������� � ������ 2! ��� ����������, � �� ����� ���������� ��������)
unsigned long FirstDataSector;//������ ������ ������
unsigned long FirstRootFolderSecNum;//������ �������� ���������� (��� FAT16 - ��� ������ � ��������� �������, ��� FAT32 - ��� ���� � ������� ������ � ��������� BPB_RootClus)
unsigned long ClusterSize;//������ �������� � ������

unsigned char FATType=FAT12;//��� �������� �������

const char Text_FAT_Type[] PROGMEM =  "��� �. �������  \0";
const char Text_FAT32[] PROGMEM =     "FAT32- ������!  \0";
const char Text_FAT16[] PROGMEM =     "FAT16- ��.      \0";
const char Text_FAT12[] PROGMEM =     "FAT12- ������!  \0";

//----------------------------------------------------------------------------------------------------
//��������� �������
//----------------------------------------------------------------------------------------------------
void FAT_Init(void);//������������� FAT
unsigned long GetByte(unsigned long offset);//������� ����
unsigned long GetShort(unsigned long offset);//������� ��� �����
unsigned long GetLong(unsigned long offset);//������� 4 �����

bool FAT_RecordPointerStepForward(struct SFATRecordPointer *sFATRecordPointerPtr);//������������� �� ������ �����
bool FAT_RecordPointerStepReverse(struct SFATRecordPointer *sFATRecordPointerPtr);//������������� �� ������ �����

bool FAT_BeginFileSearch(void);//������ ����� ����� � �������
bool FAT_PrevFileSearch(void);//��������� � ����������� ����� � ��������
bool FAT_NextFileSearch(void);//���������� ����� ����� � ��������
bool FAT_GetFileSearch(char *filename,unsigned long *FirstCluster,unsigned long *Size,signed char *directory);//�������� ��������� �������� ���������� ����� � ��������
bool FAT_EnterDirectory(unsigned long FirstCluster);//����� � ���������� � ����� ������ ����
bool FAT_WriteBlock(unsigned short *BlockSize,unsigned short Block);//�������� � ��� ���� �����
//----------------------------------------------------------------------------------------------------
//������������� FAT
//----------------------------------------------------------------------------------------------------
void FAT_Init(void)
{
    WH1602_SetTextUpLine("");
    WH1602_SetTextDownLine("");

    LastReadSector=0xffffffffUL;
//���� FAT
    FATOffset=0;
    /*
    unsigned short a=GetByte(1024UL);
    LastReadSector=0xffffffffUL;
    a=GetByte(0UL);
    unsigned short b=GetByte(510UL);
    unsigned short c=GetByte(511UL);
    char str[16];
    sprintf(str,"%x %x %x",a,b,c);
    WH1602_SetTextDownLine(str);
    _delay_ms(10000);*/

    for(unsigned long fo=0; fo<33554432UL; fo++)
    {
        unsigned char b=GetByte(fo);
        if (b==0xE9 || b==0xEB)
        {
            b=GetByte(fo+511UL);
            if (b==0xAA)
            {
                b=GetByte(fo+510UL);
                if (b==0x55)
                {
                    FATOffset=fo;
                    break;
                }
            }
        }
    }
    LastReadSector=0xffffffffUL;

    SecPerClus=GetByte(BPB_SecPerClus);//���������� �������� � ��������
    BytsPerSec=GetShort(BPB_BytsPerSec);//���������� ���� � �������
    ResvdSecCnt=GetShort(BPB_ResvdSecCnt);//������ ��������� �������

//���������� ���������� ��������, ������� �������� �����������
    RootDirSectors=(unsigned long)(ceil((GetShort(BPB_RootEntCnt)*32UL+(BytsPerSec-1UL))/BytsPerSec));
//���������� ������ ������� FAT
    FATSz=GetShort(BPB_FATSz16);//������ ����� ������� FAT � ��������
    if (FATSz==0) FATSz=GetLong(BPB_FATSz32);
//���������� ���������� �������� � ������� ������ �����
    unsigned long TotSec=GetShort(BPB_TotSec16);//����� ���������� �������� �� �����
    if (TotSec==0) TotSec=GetLong(BPB_TotSec32);
    DataSec=TotSec-(ResvdSecCnt+GetByte(BPB_NumFATs)*FATSz+RootDirSectors);
//���������� ���������� ��������� ��� ������ (������� ���������� � ������ 2! ��� ����������, � �� ����� ���������� ��������)
    CountofClusters=(unsigned long)floor(DataSec/SecPerClus);
//���������� ������ ������ ������
    FirstDataSector=ResvdSecCnt+(GetByte(BPB_NumFATs)*FATSz)+RootDirSectors;
//��������� ��� �������� �������

    FATType=FAT12;
    WH1602_SetTextProgmemUpLine(Text_FAT_Type);
    if (CountofClusters<4085UL)
    {
        WH1602_SetTextProgmemDownLine(Text_FAT12);
        while(1);
    }
    else
    {
        if (CountofClusters<65525UL)
        {
            WH1602_SetTextProgmemDownLine(Text_FAT16);
            _delay_ms(2000);
            FATType=FAT16;
        }
        else
        {
            WH1602_SetTextProgmemDownLine(Text_FAT32);
            FATType=FAT32;
            while(1);
        }
    }
    if (FATType==FAT12) return;//�� ������������
    if (FATType==FAT32) return;//�� ������������
//���������� ������ �������� ���������� (��� FAT16 - ��� ������ � ��������� �������, ��� FAT32 - ��� ���� � ������� ������ � ��������� BPB_RootClus)
    FirstRootFolderSecNum=ResvdSecCnt+(GetByte(BPB_NumFATs)*FATSz);
    ClusterSize=SecPerClus*BytsPerSec;//������ �������� � ������

//������ �������� ����������
    FirstRootFolderAddr=FirstRootFolderSecNum*BytsPerSec;//��������� ����� �������� ����������
//����������� ��������� ��� ������ ������ ����������
    sFATRecordPointer.BeginFolderAddr=FirstRootFolderAddr;//��������� ����� ��� ������ ������ ����������
    sFATRecordPointer.CurrentFolderAddr=sFATRecordPointer.BeginFolderAddr;//������� ����� ��� ������ ������ ����������
    sFATRecordPointer.BeginFolderCluster=0;//��������� ������� ����� ����� ������ ����������
    sFATRecordPointer.CurrentFolderCluster=0;//������� ������� ����� ����� ������ ����������
    sFATRecordPointer.EndFolderClusterAddr=sFATRecordPointer.BeginFolderAddr+(RootDirSectors*BytsPerSec);//�������� ����� ��� ������ ������ ���������� (��� ��������)
    sFATRecordPointer.BeginFolderClusterAddr=sFATRecordPointer.CurrentFolderAddr;//����� ���������� �������� ����������
}
//----------------------------------------------------------------------------------------------------
//������������� �� ������ �����
//----------------------------------------------------------------------------------------------------
bool FAT_RecordPointerStepForward(struct SFATRecordPointer *sFATRecordPointerPtr)
{
    sFATRecordPointerPtr->CurrentFolderAddr+=32UL;//��������� � ��������� ������
    if (sFATRecordPointerPtr->CurrentFolderAddr>=sFATRecordPointerPtr->EndFolderClusterAddr)//����� �� ������� �������� ��� ����������
    {
        if (sFATRecordPointerPtr->BeginFolderAddr==FirstRootFolderAddr)//���� � ��� ����������� �������� ����������
        {
            return(false);
        }
        else//��� �� �������� ���������� ����� ����� ����� ��������
        {
            unsigned long FATClusterOffset=0;//�������� �� ������� FAT � ������ (� FAT32 ��� 4-� �������, � � FAT16 - �����������)
            if (FATType==FAT16) FATClusterOffset=sFATRecordPointerPtr->CurrentFolderCluster*2UL;//����� �������� � ������� FAT
            unsigned long NextClusterAddr=ResvdSecCnt*BytsPerSec+FATClusterOffset;//����� ���������� ��������
            //��������� ����� ���������� �������� �����
            unsigned long NextCluster=0;
            if (FATType==FAT16) NextCluster=GetShort(NextClusterAddr);
            if (NextCluster==0 || NextCluster>=CountofClusters+2UL || NextCluster>=FAT16_EOC)//������ �������� ���
            {
                return(false);
            }
            sFATRecordPointerPtr->CurrentFolderCluster=NextCluster;//��������� � ���������� ��������
            unsigned long FirstSectorofCluster=((sFATRecordPointerPtr->CurrentFolderCluster-2UL)*SecPerClus)+FirstDataSector;
            sFATRecordPointerPtr->CurrentFolderAddr=FirstSectorofCluster*BytsPerSec;
            sFATRecordPointerPtr->BeginFolderClusterAddr=sFATRecordPointerPtr->CurrentFolderAddr;
            sFATRecordPointerPtr->EndFolderClusterAddr=sFATRecordPointerPtr->CurrentFolderAddr+SecPerClus*BytsPerSec;
        }
    }
    return(true);
}
//----------------------------------------------------------------------------------------------------
//������������� �� ������ �����
//----------------------------------------------------------------------------------------------------
bool FAT_RecordPointerStepReverse(struct SFATRecordPointer *sFATRecordPointerPtr)
{
    sFATRecordPointerPtr->CurrentFolderAddr-=32UL;//������������ �� ������ �����
    if (sFATRecordPointerPtr->CurrentFolderAddr<sFATRecordPointerPtr->BeginFolderClusterAddr)//����� �� ������ ������� ��������
    {
        if (sFATRecordPointerPtr->BeginFolderAddr==FirstRootFolderAddr)//���� � ��� �������� ����������
        {
            return(false);//����� �� ������� ����������
        }
        else//��� �� �������� ���������� ����� ����� �����
        {
            unsigned long PrevCluster=sFATRecordPointerPtr->BeginFolderCluster;//���������� �������
            while(1)
            {
                unsigned long FATClusterOffset=0;//�������� �� ������� FAT � ������ (� FAT32 ��� 4-� �������, � � FAT16 - �����������)
                if (FATType==FAT16) FATClusterOffset=PrevCluster*2UL;
                unsigned long ClusterAddr=ResvdSecCnt*BytsPerSec+FATClusterOffset;//����� ����������� ��������
                unsigned long cluster=GetShort(ClusterAddr);
                if (cluster<=2 || cluster>=FAT16_EOC)//������ �������� ���
                {
                    return(false);//����� �� ������� ����������
                }
                if (cluster==sFATRecordPointerPtr->CurrentFolderCluster) break;//�� ����� �������������� �������
                PrevCluster=cluster;
            }
            if (PrevCluster<=2 || PrevCluster>=FAT16_EOC)//������ �������� ���
            {
                return(false);//����� �� ������� ����������
            }
            sFATRecordPointerPtr->CurrentFolderCluster=PrevCluster;//��������� � ����������� ��������
            unsigned long FirstSectorofCluster=((sFATRecordPointerPtr->CurrentFolderCluster-2UL)*SecPerClus)+FirstDataSector;
            sFATRecordPointerPtr->BeginFolderClusterAddr=FirstSectorofCluster*BytsPerSec;
            sFATRecordPointerPtr->EndFolderClusterAddr=sFATRecordPointerPtr->BeginFolderClusterAddr+SecPerClus*BytsPerSec;
            sFATRecordPointerPtr->CurrentFolderAddr=sFATRecordPointerPtr->EndFolderClusterAddr-32UL;//�� ������ �����
        }
    }
    return(true);
}
//----------------------------------------------------------------------------------------------------
//������ ����� ����� � �������
//----------------------------------------------------------------------------------------------------
bool FAT_BeginFileSearch(void)
{
    unsigned long FirstCluster;//������ ������� �����
    unsigned long Size;//������ �����
    signed char Directory;//�� ���������� �� ����

    sFATRecordPointer.CurrentFolderAddr=sFATRecordPointer.BeginFolderAddr;
    sFATRecordPointer.CurrentFolderCluster=sFATRecordPointer.BeginFolderCluster;
    sFATRecordPointer.BeginFolderClusterAddr=sFATRecordPointer.CurrentFolderAddr;
    if (sFATRecordPointer.BeginFolderAddr!=FirstRootFolderAddr)//��� �� �������� ����������
    {
        sFATRecordPointer.EndFolderClusterAddr=sFATRecordPointer.BeginFolderAddr+SecPerClus*BytsPerSec;
    }
    else sFATRecordPointer.EndFolderClusterAddr=sFATRecordPointer.BeginFolderAddr+(RootDirSectors*BytsPerSec);//�������� ����� ��� ������ ������ ���������� (��� ��������)
//��������� � ������� ������� ��� �����
    while(1)
    {
        if (FAT_GetFileSearch(NULL,&FirstCluster,&Size,&Directory)==false)
        {
            if (FAT_NextFileSearch()==false) return(false);
        }
        else return(true);
    }
    return(false);
}
//----------------------------------------------------------------------------------------------------
//������� � ����������� ����� � ��������
//----------------------------------------------------------------------------------------------------
bool FAT_PrevFileSearch(void)
{
    struct SFATRecordPointer sFATRecordPointer_Copy=sFATRecordPointer;
    while(1)
    {
        if (FAT_RecordPointerStepReverse(&sFATRecordPointer_Copy)==false) return(false);
        //����������� ��� �����
        unsigned char n;
        bool res=true;
        for(n=0; n<11; n++)
        {
            unsigned char b=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+(unsigned long)(n));
            if (n==0)
            {
                if (b==0x20 || b==0xE5)
                {
                    res=false;
                    break;
                }
            }
            if (b<0x20)
            {
                res=false;
                break;
            }
            if (n==1)
            {
                unsigned char a=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr);
                unsigned char b=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+1UL);
                if (a==(unsigned char)'.' && b!=(unsigned char)'.')
                {
                    res=false;
                    break;
                }
            }
        }
        //������� ����������
        if (res==true)
        {
            unsigned char type=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+11UL);
            if (type&ATTR_VOLUME_ID) continue;//���� ���� - ��� �����
            if ((type&ATTR_DIRECTORY)==0)//��� ����
            {
                unsigned char a=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+10UL);
                unsigned char b=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+9UL);
                unsigned char c=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+8UL);
                if (!(a=='P' && b=='A' && c=='T')) continue;//�������� ����������
            }
            sFATRecordPointer=sFATRecordPointer_Copy;
            return(true);
        }
    }
    return(false);
}
//----------------------------------------------------------------------------------------------------
//������� � ���������� ����� � ��������
//----------------------------------------------------------------------------------------------------
bool FAT_NextFileSearch(void)
{
    struct SFATRecordPointer sFATRecordPointer_Copy=sFATRecordPointer;
    while(1)
    {
        if (FAT_RecordPointerStepForward(&sFATRecordPointer_Copy)==false) return(false);
        unsigned char n;
        bool res=true;
        for(n=0; n<11; n++)
        {
            unsigned char b=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+(unsigned long)(n));
            if (n==0)
            {
                if (b==0x20 || b==0xE5)
                {
                    res=false;
                    break;
                }
            }
            if (b<0x20)
            {
                res=false;
                break;
            }
            if (n==1)
            {
                unsigned char a=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr);
                unsigned char b=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+1UL);
                if (a==(unsigned char)'.' && b!=(unsigned char)'.')
                {
                    res=false;
                    break;
                }
            }
        }
        if (res==true)
        {
            unsigned char type=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+11UL);
            if (type&ATTR_VOLUME_ID) continue;//���� ���� - ��� �����
            if ((type&ATTR_DIRECTORY)==0)//��� ����
            {
                unsigned char a=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+10UL);
                unsigned char b=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+9UL);
                unsigned char c=GetByte(sFATRecordPointer_Copy.CurrentFolderAddr+8UL);
                if (!(a=='P' && b=='A' && c=='T')) continue;//�������� ����������
            }
            sFATRecordPointer=sFATRecordPointer_Copy;
            return(true);
        }
    }
    return(false);
}
//----------------------------------------------------------------------------------------------------
//�������� ��������� �������� ���������� ����� � ��������
//----------------------------------------------------------------------------------------------------
bool FAT_GetFileSearch(char *filename,unsigned long *FirstCluster,unsigned long *Size,signed char *directory)
{
    unsigned char n;
    bool res=true;
    *directory=0;
    if (filename!=NULL)
    {
        for(n=0; n<11; n++) filename[n]=32;
    }
    for(n=0; n<11; n++)
    {
        unsigned char b=GetByte(sFATRecordPointer.CurrentFolderAddr+(unsigned long)(n));
        if (n==0)
        {
            if (b==0x20 || b==0xE5)
            {
                res=false;
                break;
            }
        }
        if (b<0x20)
        {
            res=false;
            break;
        }
        if (filename!=NULL)
        {
            if (n<8) filename[n]=b;
            else filename[n+1]=b;
        }
        if (n==1)
        {
            unsigned char a=GetByte(sFATRecordPointer.CurrentFolderAddr);
            unsigned char b=GetByte(sFATRecordPointer.CurrentFolderAddr+1UL);
            if (a==(unsigned char)'.' && b!=(unsigned char)'.')
            {
                res=false;
                break;
            }
        }
    }
    if (res==true)
    {
        unsigned char type=GetByte(sFATRecordPointer.CurrentFolderAddr+11UL);
        if (type&ATTR_VOLUME_ID) return(false);//���� ���� - ��� �����
        if ((type&ATTR_DIRECTORY)==0)//��� ����
        {
            unsigned char a=GetByte(sFATRecordPointer.CurrentFolderAddr+10UL);
            unsigned char b=GetByte(sFATRecordPointer.CurrentFolderAddr+9UL);
            unsigned char c=GetByte(sFATRecordPointer.CurrentFolderAddr+8UL);
            if (!(a=='P' && b=='A' && c=='T')) return(false);//�������� ����������
        }
        else//���� ��� ����������
        {
            unsigned char a=GetByte(sFATRecordPointer.CurrentFolderAddr);
            unsigned char b=GetByte(sFATRecordPointer.CurrentFolderAddr+1UL);
            if (a==(unsigned char)'.' && b==(unsigned char)'.') *directory=-1;//�� ���������� ����
            else *directory=1;//�� ���������� ����
        }
        //������ ������� �����
        *FirstCluster=(GetShort(sFATRecordPointer.CurrentFolderAddr+20UL)<<16)|GetShort(sFATRecordPointer.CurrentFolderAddr+26UL);
        //����� ������ ����� � ������
        *Size=GetLong(sFATRecordPointer.CurrentFolderAddr+28UL);
        if (filename!=NULL)
        {
            if ((type&ATTR_DIRECTORY)==0) filename[8]='.';//����� ��������� �����
            filename[12]=0;
            //������ ������� ��� �����
            struct SFATRecordPointer sFATRecordPointer_Local=sFATRecordPointer;
            unsigned char long_name_length=0;
            while(1)
            {
                if (FAT_RecordPointerStepReverse(&sFATRecordPointer_Local)==false) break;
                unsigned char attr=GetByte(sFATRecordPointer_Local.CurrentFolderAddr+11UL);
                if (attr&ATTR_LONG_NAME)//��� ������� ���
                {
                    //�������� ������ ���
                    unsigned char name_index=GetByte(sFATRecordPointer_Local.CurrentFolderAddr);
                    for(n=0; n<10 && long_name_length<=16; n+=2,long_name_length++) filename[long_name_length]=GetByte(sFATRecordPointer_Local.CurrentFolderAddr+n+1UL);
                    for(n=0; n<12 && long_name_length<=16; n+=2,long_name_length++) filename[long_name_length]=GetByte(sFATRecordPointer_Local.CurrentFolderAddr+n+14UL);
                    for(n=0; n<4 && long_name_length<=16; n+=2,long_name_length++) filename[long_name_length]=GetByte(sFATRecordPointer_Local.CurrentFolderAddr+n+28UL);
                    if (long_name_length>16) break;
                    if (name_index&0x40) break;//��������� ����� �����
                }
                else break;//��� �� ������� ���
            }
            if (long_name_length>16) long_name_length=16;
            if (long_name_length>0) filename[long_name_length]=0;
        }
        return(true);
    }
    return(false);
}
//----------------------------------------------------------------------------------------------------
//����� � ���������� � ����� ������ ����
//----------------------------------------------------------------------------------------------------
bool FAT_EnterDirectory(unsigned long FirstCluster)
{
    if (FirstCluster==0UL)//��� �������� ���������� (����� ������� ��������, ��������������� ����������)
    {
        sFATRecordPointer.BeginFolderAddr=FirstRootFolderAddr;
        sFATRecordPointer.EndFolderClusterAddr=sFATRecordPointer.BeginFolderAddr+(RootDirSectors*BytsPerSec);//�������� ����� ��� ������ ������ ���������� (��� ��������)
    }
    else
    {
        unsigned long FirstSectorofCluster=((FirstCluster-2UL)*SecPerClus)+FirstDataSector;
        sFATRecordPointer.BeginFolderAddr=FirstSectorofCluster*BytsPerSec;//��������� ����� ��� ������ ������ ����������
        sFATRecordPointer.EndFolderClusterAddr=sFATRecordPointer.BeginFolderAddr+SecPerClus*BytsPerSec;
    }
    sFATRecordPointer.BeginFolderCluster=FirstCluster;//��������� ������� ����� ����� ������ ����������
    sFATRecordPointer.CurrentFolderCluster=sFATRecordPointer.BeginFolderCluster;//������� ������� ����� ����� ������ ����������
    sFATRecordPointer.CurrentFolderAddr=sFATRecordPointer.BeginFolderAddr;//������� ����� ��� ������ ������ ����������
    sFATRecordPointer.BeginFolderClusterAddr=sFATRecordPointer.BeginFolderAddr;
    return(FAT_BeginFileSearch());
}
//----------------------------------------------------------------------------------------------------
//�������� � ��� ���� �����
//----------------------------------------------------------------------------------------------------
bool FAT_WriteBlock(unsigned short *BlockSize,unsigned short Block)
{
    unsigned long CurrentCluster;
    unsigned long Size;

    unsigned long i=0;//����� ������������ ����� �����
    unsigned short dram_addr=0;//����� � ������������ ������
    unsigned short current_block=0;//������� ����� �����
    unsigned short block_size=0;//������ �����
    signed char Directory;//�� ���������� �� ����
    *BlockSize=0;
    if (FAT_GetFileSearch(string,&CurrentCluster,&Size,&Directory)==false) return(false);
    unsigned char mode=0;
    while(i<Size)
    {
        DRAM_Refresh();//���������� ����������� ������
        //��������� ������
        unsigned long length=ClusterSize;
        if (length+i>=Size) length=Size-i;
        //�������� ������ ������ ��������
        unsigned long FirstSectorofCluster=((CurrentCluster-2UL)*SecPerClus)+FirstDataSector;
        unsigned long addr=FirstSectorofCluster*BytsPerSec;
        for(unsigned long m=0; m<length; m++,i++)
        {
            DRAM_Refresh();//���������� ����������� ������
            unsigned char b=GetByte(addr+m);
            if (mode==0)//������ �������� ����� �����
            {
                block_size=b;
                mode=1;
                continue;
            }
            if (mode==1)//������ �������� ����� �����
            {
                block_size|=((unsigned short)b)<<8;
                mode=2;
                dram_addr=0;
                continue;
            }
            if (mode==2)//������ ������
            {
                if (current_block==Block) DRAM_WriteByte(dram_addr,b);//��� ��������� ����
                dram_addr++;
                if (dram_addr>=block_size)//���� ��������
                {
                    if (current_block==Block)//���������� ��������� ����
                    {
                        *BlockSize=block_size;
                        return(true);
                    }
                    //��������� � ���������� �����
                    block_size=0;
                    current_block++;
                    mode=0;
                }
            }
        }
        //��������� � ���������� �������� �����
        unsigned long FATClusterOffset=0;//�������� �� ������� FAT � ������ (� FAT32 ��� 4-� �������, � � FAT16 - �����������)
        if (FATType==FAT16) FATClusterOffset=CurrentCluster*2UL;
        unsigned long NextClusterAddr=ResvdSecCnt*BytsPerSec+FATClusterOffset;//����� ���������� ��������
        //��������� ����� ���������� �������� �����
        unsigned long NextCluster=0;
        if (FATType==FAT16) NextCluster=GetShort(NextClusterAddr);
        if (NextCluster==0) break;//�������������� �������
        if (NextCluster>=CountofClusters+2UL) break;//����� ������ ����������� ���������� ������ �������� - ����� ����� ��� ����
        CurrentCluster=NextCluster;
    }
//����� �����
    return(false);
}
//----------------------------------------------------------------------------------------------------
//������� ����
//----------------------------------------------------------------------------------------------------
unsigned long GetByte(unsigned long offset)
{
    offset+=FATOffset;
    unsigned long s=offset>>9UL;//����� �� 512
    if (s!=LastReadSector)
    {
        LastReadSector=s;
        SD_ReadBlock(s,Sector);
        //������ �� ���������, �� ����� ������ ������� �� ������ - ���� ��������, ���� ���
    }
    return(Sector[offset&0x1FFUL]);
}
//----------------------------------------------------------------------------------------------------
//������� ��� �����
//----------------------------------------------------------------------------------------------------
unsigned long GetShort(unsigned long offset)
{
    unsigned long v=GetByte(offset+1UL);
    v<<=8UL;
    v|=GetByte(offset);
    return(v);
}
//----------------------------------------------------------------------------------------------------
//������� 4 �����
//----------------------------------------------------------------------------------------------------
unsigned long GetLong(unsigned long offset)
{
    unsigned long v=GetByte(offset+3UL);
    v<<=8UL;
    v|=GetByte(offset+2UL);
    v<<=8UL;
    v|=GetByte(offset+1UL);
    v<<=8UL;
    v|=GetByte(offset);
    return(v);
}
