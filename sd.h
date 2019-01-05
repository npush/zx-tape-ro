//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//��������� SD-�����
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define SD_CS_DDR   DDRB
#define SD_CS_PORT  PORTB
#define SD_CS       4

#define SD_DI_DDR   DDRB
#define SD_DI_PORT  PORTB
#define SD_DI       6

#define SD_DO_DDR   DDRB
#define SD_DO_PORT  PORTB
#define SD_DO       7

#define SD_SCK_DDR  DDRB
#define SD_SCK_PORT PORTB
#define SD_SCK      5


//���� ������
#define CMD0 0x40
#define CMD1 (CMD0+1)
#define CMD8 (CMD0+8)
#define CMD9 (CMD0+9)
#define CMD16 (CMD0+16)
#define CMD17 (CMD0+17)
#define CMD55 (CMD0+55)
#define CMD58 (CMD0+58)


//������ ������
#define ANSWER_R1_SIZE 1
#define ANSWER_R3_SIZE 5

//���� SD-����
typedef enum
{
    SD_TYPE_NONE=0,
    SD_TYPE_MMC_V3=1,
    SD_TYPE_SD_V1=2,
    SD_TYPE_SD_V2=3,
    SD_TYPE_SD_V2_HC=4
} SD_TYPE;

//----------------------------------------------------------------------------------------------------
//���������� ����������
//----------------------------------------------------------------------------------------------------
unsigned short BlockByteCounter=512;//��������� ���� �����
SD_TYPE SDType=SD_TYPE_NONE;//��� ����� ������

const char Text_SD_No_SPI_Up[] PROGMEM =       "����� ������ �� \0";
const char Text_SD_No_SPI_Down[] PROGMEM =     "������������ SPI\0";
const char Text_SD_No_Response[] PROGMEM =     "����� ������!   \0";
const char Text_SD_Size_Error_Up[] PROGMEM =   "����� SD-�����  \0";
const char Text_SD_Size_Error_Down[] PROGMEM = "�� ���������!   \0";
const char Text_SD_Size[] PROGMEM =            "����� SD-�����  \0";
//----------------------------------------------------------------------------------------------------
//��������� �������
//----------------------------------------------------------------------------------------------------
void SD_Init(void);//������������� ����� ������

unsigned char SD_TransmitData(unsigned char data);//������� ������ SD-����� � ������� �����
bool SD_SendCommand(unsigned char cmd,unsigned char b0,unsigned char b1,unsigned char b2,unsigned char b3,unsigned char answer_size,unsigned char *answer);//������� ������� � �������� ����� �� SD-�����
bool SD_GetSize(unsigned long *size);//�������� ����� SD-����� � ������
unsigned short GetBits(unsigned char *data,unsigned char begin,unsigned char end);//�������� ���� � begin �� end ������������
bool SD_BeginReadBlock(unsigned long BlockAddr);//������ ������ �����
bool SD_ReadBlockByte(unsigned char *byte);//������� ���� �����
bool SD_ReadBlock(unsigned long BlockAddr,unsigned char *Addr);//������� ���� � 512 ���� � ������
//----------------------------------------------------------------------------------------------------
//������������� ����� ������
//----------------------------------------------------------------------------------------------------
void SD_Init(void)
{
    WH1602_SetTextUpLine("");
    WH1602_SetTextDownLine("");
    SD_CS_DDR|=(1<<SD_CS);
    SD_DI_DDR|=(1<<SD_DI);
    SD_SCK_DDR|=(1<<SD_SCK);
    SD_DO_DDR&=0xff^(1<<SD_DO);
//����� SPI SS � ������ MASTER ��������������� ��� ����� � �� SPI �� ������
    _delay_ms(1000);//�����, ���� ����� �� ���������
    unsigned char n;
//��� �� ����� 74 ��������� ������������� ��� ������� ������ �� CS � DI
    SD_CS_PORT|=(1<<SD_CS);
    _delay_ms(500);
    SD_DI_PORT|=(1<<SD_DI);
    for(n=0; n<250; n++)
    {
        SD_SCK_PORT|=(1<<SD_SCK);
        _delay_ms(1);
        SD_SCK_PORT&=0xff^(1<<SD_SCK);
        _delay_ms(1);
    }
    SD_CS_PORT&=0xff^(1<<SD_CS);
//����������� SP
    SPCR=(0<<SPIE)|(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)|(1<<SPR1)|(1<<SPR0);
    SPSR=(0<<SPI2X);//��������� �������� SPI
    _delay_ms(100);

    unsigned char answer[ANSWER_R3_SIZE];//����� �� �����
    bool res;
//��� CMD0
    res=SD_SendCommand(CMD0,0x00,0x00,0x00,0x00,ANSWER_R1_SIZE,answer);
    if (res==false || answer[0]!=1)//������
    {
        WH1602_SetTextProgmemUpLine(Text_SD_No_SPI_Up);
        WH1602_SetTextProgmemDownLine(Text_SD_No_SPI_Down);
        while(1);
        return;
    }

//���������� ��� �����
    SDType=SD_TYPE_NONE;
//��� CMD8 (����� ����� R3 � ������ SPI)
    res=SD_SendCommand(CMD8,0xAA,0x01,0x00,0x00,ANSWER_R3_SIZE,answer);
    if (res==true && answer[0]==0x01)
    {
        if (!((answer[ANSWER_R3_SIZE-2]&0x0F)==0x01 && answer[ANSWER_R3_SIZE-1]==0xAA))
        {
            WH1602_SetTextProgmemUpLine(Text_SD_No_Response);
            while(1);
            return;
        }
        //��� ACMD41
        for(n=0; n<65535; n++)
        {
            //��� ACMD41
            res=SD_SendCommand(CMD55,0x00,0x00,0x00,0x00,ANSWER_R1_SIZE,answer);
            if (res==false || (answer[0]!=0x00 && answer[0]!=0x01))
            {
                WH1602_SetTextProgmemUpLine(Text_SD_No_Response);
                while(1);
                return;
            }
            res=SD_SendCommand(CMD1,0x00,0x00,0x00,0x40,ANSWER_R1_SIZE,answer);
            if (res==true && answer[0]==0x00) break;
        }
        if (n==65535)
        {
            WH1602_SetTextProgmemUpLine(Text_SD_No_Response);
            while(1);
            return;
        }
        //��� CMD58
        res=SD_SendCommand(CMD58,0x00,0x00,0x00,0x00,ANSWER_R3_SIZE,answer);
        if (res==false)
        {
            WH1602_SetTextProgmemUpLine(Text_SD_No_Response);
            while(1);
            return;
        }
        if (answer[1]&0x40) SDType=SD_TYPE_SD_V2_HC;
        else SDType=SD_TYPE_SD_V2;
    }
    else//����� �� �������� �� CMD8
    {
        //��������� ����������� ����� ACMD41, �� ������ ������ ����� �� 16 �� ���������� � ��� ��������, ��� ���� ������ ��� �� ���������.
        //����� ����� ����� ���������������� ������ ����� CMD1, � �� ����� ACMD41
        //��� ACMD41
        for(n=0; n<65535; n++)
        {
            /*
            //��� ACMD41, ���� ������ ��� �� MMC V3
            if (SDType!=SD_TYPE_MMC_V3)
            {
            res=SD_SendCommand(CMD55,0x00,0x00,0x00,0x00,ANSWER_R1_SIZE,answer);
             if (res==false || (answer[0]!=0x00 && answer[0]!=0x01)) SDType=SD_TYPE_MMC_V3;//����� �� ����� ACM41, ��� ��� ������ ��� ������ CMD1
            }
            */
            res=SD_SendCommand(CMD1,0x00,0x00,0x00,0x00,ANSWER_R1_SIZE,answer);
            if (res==true && answer[0]==0x00) break;
        }
        if (n==65535)
        {
            WH1602_SetTextProgmemUpLine(Text_SD_No_Response);
            while(1);
        }
        if (SDType!=SD_TYPE_MMC_V3) SDType=SD_TYPE_SD_V1;
    }
//����� ������ ����� 512 ����
    res=SD_SendCommand(CMD16,0x00,0x00,0x02,0x00,ANSWER_R1_SIZE,answer);

//������������� ������� �������
    if (SDType!=SD_TYPE_SD_V2_HC)//������ ����� ����� ������
    {
        //������ ����� ����� ������
        unsigned long SD_Size=0;
        if (SD_GetSize(&SD_Size)==false)//������
        {
            WH1602_SetTextProgmemUpLine(Text_SD_Size_Error_Up);
            WH1602_SetTextProgmemDownLine(Text_SD_Size_Error_Down);
            while(1);
            return;
        }
        unsigned short size=(unsigned short)(SD_Size>>20);
        sprintf(string,"%i ��",size);
        WH1602_SetTextProgmemUpLine(Text_SD_Size);
        WH1602_SetTextDownLine(string);
        _delay_ms(1000);
    }
    for(unsigned short m=0; m<1024; m++) SD_TransmitData(0xff);
}

//----------------------------------------------------------------------------------------------------
//������� ������ SD-����� � ������� �����
//----------------------------------------------------------------------------------------------------
inline unsigned char SD_TransmitData(unsigned char data)
{
    SPDR=data;//�������
    while(!(SPSR&(1<<SPIF)));//��� ���������� �������� � ��������� ������
    unsigned char res=SPDR;
    return(res);
}

//----------------------------------------------------------------------------------------------------
//������� ������� � �������� �����
//----------------------------------------------------------------------------------------------------
bool SD_SendCommand(unsigned char cmd,unsigned char b0,unsigned char b1,unsigned char b2,unsigned char b3,unsigned char answer_size,unsigned char *answer)
{
//���������� ������� � ������� � CRC7
    unsigned char crc7=0;
    unsigned char cmd_buf[5]= {cmd,b3,b2,b1,b0};
    unsigned short n;
    for(n=0; n<5; n++)
    {
        SD_TransmitData(cmd_buf[n]);

        unsigned char b=cmd_buf[n];
        for (unsigned char i=0; i<8; i++)
        {
            crc7<<=1;
            if ((b&0x80)^(crc7&0x80)) crc7^=0x09;
            b<<=1;
        }
    }
    crc7=crc7<<1;
    crc7|=1;
    SD_TransmitData(crc7);//CRC
//����� ����� �������� �� �����
//��������� ����� R1 (������� ��� ������ 0)
    for(n=0; n<65535; n++)
    {
        unsigned char res=SD_TransmitData(0xff);
        if ((res&128)==0)
        {
            answer[0]=res;
            break;
        }
        _delay_us(10);
    }
    if (n==65535) return(false);
    for(n=1; n<answer_size; n++)
    {
        answer[n]=SD_TransmitData(0xff);
    }
    SD_TransmitData(0xff);
    return(true);//����� ������
}
//----------------------------------------------------------------------------------------------------
//�������� ����� SD-����� � ������
//----------------------------------------------------------------------------------------------------
bool SD_GetSize(unsigned long *size)
{
    unsigned short n;
    unsigned char answer[ANSWER_R1_SIZE];
    if (SD_SendCommand(CMD9,0x00,0x00,0x00,0x00,ANSWER_R1_SIZE,answer)==false) return(false);//����� �� ������
//��������� 16 ���� ������ ������ R1
    unsigned char byte=0;
    for(n=0; n<65535; n++)
    {
        byte=SD_TransmitData(0xff);
        if (byte!=0xff) break;
    }
    if (n==65535) return(false);//����� �� ������
    unsigned char b[16];
    n=0;
    if (byte!=0xfe)//�������� ���� �� ��� ������ �������� ������ ������
    {
        b[0]=byte;
        n++;
    }
    for(; n<16; n++) b[n]=SD_TransmitData(0xff);
//������� ������ ����� ������
    unsigned long blocks=0;
    if (SDType==SD_TYPE_SD_V2_HC)//������� CSD ������
    {
        blocks=GetBits(b,69,48);
        unsigned long read_bl_len=GetBits(b,83,80);
        unsigned long block_size=(1UL<<read_bl_len);
        //���� ��� ����������, ��� �� ���� ���� �������� ����� ����� ������


    }
    else//������� SD-�����
    {
        //���������, �������� SDCardManual
        unsigned long read_bl_len=GetBits(b,83,80);
        unsigned long c_size=GetBits(b,73,62);
        unsigned long c_size_mult=GetBits(b,49,47);
        blocks=(c_size+1UL)*(1UL<<(c_size_mult+2UL));
        blocks*=(1UL<<read_bl_len);
    }
    *size=blocks;
    return(true);
}
//----------------------------------------------------------------------------------------------------
//�������� ���� � begin �� end ������������
//----------------------------------------------------------------------------------------------------
unsigned short GetBits(unsigned char *data,unsigned char begin,unsigned char end)
{
    unsigned short bits=0;
    unsigned char size=1+begin-end;
    for(unsigned char i=0; i<size; i++)
    {
        unsigned char position=end+i;
        unsigned short byte=15-(position>>3);
        unsigned short bit=position&0x7;
        unsigned short value=(data[byte]>>bit)&1;
        bits|=value<<i;
    }
    return(bits);
}
//----------------------------------------------------------------------------------------------------
//������ ������ �����
//----------------------------------------------------------------------------------------------------
bool SD_BeginReadBlock(unsigned long BlockAddr)
{
    if (SDType!=SD_TYPE_SD_V2_HC) BlockAddr<<=9;//�������� �� 512 ��� ������ ���� ������
//��� ������� ������ �����
    unsigned char a1=(unsigned char)((BlockAddr>>24)&0xff);
    unsigned char a2=(unsigned char)((BlockAddr>>16)&0xff);
    unsigned char a3=(unsigned char)((BlockAddr>>8)&0xff);
    unsigned char a4=(unsigned char)(BlockAddr&0xff);
    unsigned char answer[ANSWER_R1_SIZE];
    bool ret=SD_SendCommand(CMD17,a4,a3,a2,a1,ANSWER_R1_SIZE,answer);//�������� CMD17
    if (ret==false || answer[0]!=0) return(false);//������ �������
    SD_TransmitData(0xff);//�������� ����������
//��� ������ ����������� ������
    unsigned short n;
    for(n=0; n<65535; n++)
    {
        unsigned char res=SD_TransmitData(0xff);
        if (res==0xfe) break;//������ �������
        _delay_us(10);
    }
    if (n==65535) return(false);//������ ������ ������ �� �������
    BlockByteCounter=0;
    return(true);
}
//----------------------------------------------------------------------------------------------------
//������� ���� �����
//----------------------------------------------------------------------------------------------------
bool SD_ReadBlockByte(unsigned char *byte)
{
    if (BlockByteCounter>=512) return(false);
    *byte=SD_TransmitData(0xff);//������ ���� � SD-�����
    BlockByteCounter++;
    if (BlockByteCounter==512)
    {
        //��������� CRC
        SD_TransmitData(0xff);
        SD_TransmitData(0xff);
    }
    return(true);
}
//----------------------------------------------------------------------------------------------------
//������� ���� � 512 ���� � ������
//----------------------------------------------------------------------------------------------------
bool SD_ReadBlock(unsigned long BlockAddr,unsigned char *Addr)
{
    if (SDType!=SD_TYPE_SD_V2_HC) BlockAddr<<=9;//�������� �� 512 ��� ������ ���� ������
//��� ������� ������ �����
    unsigned char a1=(unsigned char)((BlockAddr>>24)&0xff);
    unsigned char a2=(unsigned char)((BlockAddr>>16)&0xff);
    unsigned char a3=(unsigned char)((BlockAddr>>8)&0xff);
    unsigned char a4=(unsigned char)(BlockAddr&0xff);
    unsigned char answer[ANSWER_R1_SIZE];
    bool ret=SD_SendCommand(CMD17,a4,a3,a2,a1,ANSWER_R1_SIZE,answer);//�������� CMD17
    if (ret==false || answer[0]!=0) return(false);//������ �������
    SD_TransmitData(0xff);//�������� ����������
//��� ������ ����������� ������
    unsigned short n;
    for(n=0; n<65535; n++)
    {
        unsigned char res=SD_TransmitData(0xff);
        if (res==0xfe) break;//������ �������
        _delay_us(10);
    }
    if (n==65535) return(false);//������ ������ ������ �� �������
    for(n=0; n<512; n++,Addr++)
    {
        *Addr=SD_TransmitData(0xff);//������ ���� � SD-�����
    }
//��������� CRC
    SD_TransmitData(0xff);
    SD_TransmitData(0xff);
    return(true);
}
