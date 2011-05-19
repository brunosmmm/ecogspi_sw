#include "ecog_spi.h"

//estrutura global contendo os dados
static ECOGSPI EcoGSPIData;


//protótipos de funções locais para comunicação
static void FT2232_PGA280_WriteData(unsigned char * sendbuf, unsigned char buflen); //escreve em um registrador do PGA280 via SPI
static void FT2232_PGA280_ReadData(unsigned char * sendbuf , unsigned char buflen, unsigned char * readbuf, unsigned char readlen); //lê um registrador do PGA280 via SPI

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
    FT2232SPI_SendRecvData(EcoGSPIData.ft2232spi, buflen, 0, sendbuf, NULL);

}

static void FT2232_PGA280_ReadData(unsigned char * sendbuf , unsigned char buflen, unsigned char * readbuf, unsigned char readlen)
{
    //lê dados: primeiro escreve o comando de leitura e depois recebe os dados
    FT2232SPI_SendRecvData(EcoGSPIData.ft2232spi, buflen, readlen, sendbuf, readbuf);

}
