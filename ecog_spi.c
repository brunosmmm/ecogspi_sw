#include "ecog_spi.h"

//estrutura global contendo os dados
static ECOGSPI EcoGSPIData;


//protótipos de funções locais para comunicação
static void FT2232_PGA280_WriteData(unsigned char * sendbuf, unsigned char buflen); //escreve em um registrador do PGA280 via SPI
static void FT2232_PGA280_ReadData(unsigned char * sendbuf , unsigned char buflen, unsigned char * readbuf, unsigned char readlen); //lê um registrador do PGA280 via SPI

static void FT2232_ADS1259_WriteReadData(unsigned char * writeBuf, unsigned char writeLen, unsigned char * readBuf, unsigned char readLen); //envia e recebe dados do ADS1259 através do PGA280

int ECOGSPI_Init(void)
{
    int initStatus = 0x00;

    //inicialização FTDI2232
    EcoGSPIData.ft2232spi = FT2232SPI_INIT(0,0,FT2232SPI_CPHA1,TRUE,12);

    initStatus = FT2232SPI_HWINIT(EcoGSPIData.ft2232spi,FTDI_VID,FTDI_FT2232H_PID,INTERFACE_A);

    //inicialização PGA280
    EcoGSPIData.pga280 = PGA280_INIT(FT2232_PGA280_WriteData,FT2232_PGA280_ReadData);

    return initStatus;
}

static void FT2232_PGA280_WriteData(unsigned char * sendbuf , unsigned char buflen)
{
    //escreve dados: escreve o comando de escrita
    FT2232SPI_SendRecvData(EcoGSPIData.ft2232spi, buflen, 0, sendbuf, NULL,FT2232SPI_RW_ASSERTCS);

}

static void FT2232_PGA280_ReadData(unsigned char * sendbuf , unsigned char buflen, unsigned char * readbuf, unsigned char readlen)
{
    //lê dados: primeiro escreve o comando de leitura e depois recebe os dados
    FT2232SPI_SendRecvData(EcoGSPIData.ft2232spi, buflen, readlen, sendbuf, readbuf,FT2232SPI_RW_ASSERTCS);

}


static void FT2232_ADS1259_WriteReadData(unsigned char * writeBuf, unsigned char writeLen, unsigned char * readBuf, unsigned char readLen)
{
    unsigned char myBuf[writeLen + 1];

    myBuf[0] = 0xC0; //comando para utilizar o modo ECS (GPIO0) do PGA280 para enviar dados

    memcpy(&myBuf[1],writeBuf,writeLen); //copia buffer de escrita para variável local

    //escreve e lê
    FT2232SPI_SendRecvData(EcoGSPIData.ft2232spi, writeLen+1, readLen, writeBuf, readBuf, FT2232SPI_RW_ASSERTCS);

}
