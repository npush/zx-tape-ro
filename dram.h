//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//��������� ������������ ������
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define DRAM_RAS_PORT PORTD
#define DRAM_RAS_DDR  DDRD
#define DRAM_RAS      6

#define DRAM_CAS_PORT PORTD
#define DRAM_CAS_DDR  DDRD
#define DRAM_CAS      7

#define DRAM_WE_PORT  PORTD
#define DRAM_WE_DDR   DDRD
#define DRAM_WE       5

#define DRAM_OE_PORT  PORTC
#define DRAM_OE_DDR   DDRC
#define DRAM_OE       0

#define DRAM_A0_A7_PORT  PORTA
#define DRAM_A0_A7_DDR   DDRA

#define DRAM_A8_PORT  PORTC
#define DRAM_A8_DDR   DDRC
#define DRAM_A8       1

#define DRAM_D1_D4_PORT PORTB
#define DRAM_D1_D4_PIN  PINB
#define DRAM_D1_D4_DDR  DDRB
#define DRAM_D1_D4_MASK ((1<<0)|(1<<1)|(1<<2)|(1<<3))

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//��������� �������
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void DRAM_Init(void);//������������� ������
void DRAM_Refresh(void);//���������� ���� ������� �����������
unsigned char DRAM_ReadNibble(unsigned long addr,bool nibble_one);//������� �����
void DRAM_WriteNibble(unsigned long addr,unsigned char nibble,bool nibble_one);//�������� �����
unsigned char DRAM_ReadByte(unsigned long addr);//������� ����
void DRAM_WriteByte(unsigned long addr,unsigned char byte);//�������� ����
//----------------------------------------------------------------------------------------------------
//������������� �������
//----------------------------------------------------------------------------------------------------
void DRAM_Init(void)
{
//�������� �����
    DRAM_RAS_DDR|=(1<<DRAM_RAS);
    DRAM_CAS_DDR|=(1<<DRAM_CAS);
    DRAM_WE_DDR|=(1<<DRAM_WE);
    DRAM_OE_DDR|=(1<<DRAM_OE);

    DRAM_A0_A7_DDR=0xff;
    DRAM_A8_DDR|=(1<<DRAM_A8);

    DRAM_D1_D4_DDR&=0xff^(DRAM_D1_D4_MASK);
//��������� ��� ������� ���������� ������� � ���������� ���������
    DRAM_RAS_PORT|=(1<<DRAM_RAS);
    DRAM_CAS_PORT|=(1<<DRAM_CAS);
    DRAM_WE_PORT|=(1<<DRAM_WE);
    DRAM_OE_PORT|=(1<<DRAM_OE);
}
//----------------------------------------------------------------------------------------------------
//���������� ���� ������� �����������
//----------------------------------------------------------------------------------------------------
void DRAM_Refresh(void)
{
//��� ������ �����������
    DRAM_CAS_PORT&=0xff^(1<<DRAM_CAS);
    asm volatile ("nop"::);
    asm volatile ("nop"::);
    DRAM_RAS_PORT&=0xff^(1<<DRAM_RAS);
    asm volatile ("nop"::);
    asm volatile ("nop"::);
    asm volatile ("nop"::);
    asm volatile ("nop"::);
    DRAM_CAS_PORT|=(1<<DRAM_CAS);
    asm volatile ("nop"::);
    asm volatile ("nop"::);
    DRAM_RAS_PORT|=(1<<DRAM_RAS);
}
//----------------------------------------------------------------------------------------------------
//������� �����
//----------------------------------------------------------------------------------------------------
unsigned char DRAM_ReadNibble(unsigned long addr,bool nibble_one)
{
//���� ������ - �� ������
    DRAM_D1_D4_DDR&=0xff^(DRAM_D1_D4_MASK);
    DRAM_OE_PORT|=1<<DRAM_OE;
//���������� ������� ����� ������
    DRAM_A0_A7_PORT=(addr&0xff);
    DRAM_A8_PORT&=0xff^(1<<DRAM_A8);
    DRAM_A8_PORT|=(((addr>>16)&0x01)<<DRAM_A8);
//��� ������ RAS
    DRAM_RAS_PORT&=0xff^(1<<DRAM_RAS);
//���������� ������� ����� ������
    DRAM_A0_A7_PORT=(addr>>8)&0xff;
    DRAM_A8_PORT&=0xff^(1<<DRAM_A8);
    if (nibble_one==false) DRAM_A8_PORT|=1<<DRAM_A8;
//��� ������ CAS
    DRAM_CAS_PORT&=0xff^(1<<DRAM_CAS);
    asm volatile ("nop"::);
    asm volatile ("nop"::);
//��������� ������
    DRAM_OE_PORT&=0xff^(1<<DRAM_OE);
    asm volatile ("nop"::);
    asm volatile ("nop"::);

    unsigned char byte=(DRAM_D1_D4_PIN&0x0f);
    DRAM_OE_PORT|=1<<DRAM_OE;
//������� ������ CAS
    DRAM_CAS_PORT|=(1<<DRAM_CAS);
//������� ������ CAS
    DRAM_CAS_PORT|=(1<<DRAM_CAS);
//������� ������ RAS
    DRAM_RAS_PORT|=(1<<DRAM_RAS);
    return(byte);
}
//----------------------------------------------------------------------------------------------------
//�������� �����
//----------------------------------------------------------------------------------------------------
void DRAM_WriteNibble(unsigned long addr,unsigned char nibble,bool nibble_one)
{
    DRAM_OE_PORT|=1<<DRAM_OE;
//���� ������ - �� ������
    DRAM_D1_D4_DDR|=DRAM_D1_D4_MASK;
//���������� ������� ����� ������
    DRAM_A0_A7_PORT=(addr&0xff);
    DRAM_A8_PORT&=0xff^(1<<DRAM_A8);
    DRAM_A8_PORT|=(((addr>>16)&0x01)<<DRAM_A8);
//��� ������ RAS
    DRAM_RAS_PORT&=0xff^(1<<DRAM_RAS);
    asm volatile ("nop"::);
//�������� ������ ������
    DRAM_WE_PORT&=0xff^(1<<DRAM_WE);
    asm volatile ("nop"::);
//���������� ������� ����� ������
    DRAM_A0_A7_PORT=(addr>>8)&0xff;
    DRAM_A8_PORT&=0xff^(1<<DRAM_A8);
    if (nibble_one==false) DRAM_A8_PORT|=1<<DRAM_A8;
//����� ������
    DRAM_D1_D4_PORT&=0xff^(DRAM_D1_D4_MASK);
    DRAM_D1_D4_PORT|=nibble&0x0f;
//��� ������ CAS
    DRAM_CAS_PORT&=0xff^(1<<DRAM_CAS);
    asm volatile ("nop"::);
    asm volatile ("nop"::);
//������� ������ ������
    DRAM_WE_PORT|=(1<<DRAM_WE);
//������� ������ CAS
    DRAM_CAS_PORT|=(1<<DRAM_CAS);
//������� ������ RAS
    DRAM_RAS_PORT|=(1<<DRAM_RAS);
}

//----------------------------------------------------------------------------------------------------
//������� ����
//----------------------------------------------------------------------------------------------------
unsigned char DRAM_ReadByte(unsigned long addr)
{
    unsigned char byte=DRAM_ReadNibble(addr,false);
    byte<<=4;
    byte|=DRAM_ReadNibble(addr,true);
    return(byte);
}
//----------------------------------------------------------------------------------------------------
//�������� ����
//----------------------------------------------------------------------------------------------------
void DRAM_WriteByte(unsigned long addr,unsigned char byte)
{
    DRAM_WriteNibble(addr,byte>>4,false);
    DRAM_WriteNibble(addr,byte&0x0f,true);
}
