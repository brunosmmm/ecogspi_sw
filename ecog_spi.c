#include "ecog_spi.h"

//estrutura global contendo os dados
static ECOGSPI EcoGSPIData;


//protótipos de funções locais para comunicação
static void FT2232_PGA280_WriteData(unsigned char * sendbuf, unsigned char buflen); //escreve em um registrador do PGA280 via SPI
static void FT2232_PGA280_ReadData(unsigned char * sendbuf , unsigned char buflen, unsigned char * readbuf, unsigned char readlen); //lê um registrador do PGA280 via SPI




void ECOGSPI_Init(void)
{

    //inicialização FTDI2232
    //EcoGSPIData.ft2232spi = FT2232SPI_INIT();


    //inicialização PGA280
    EcoGSPIData.pga280 = PGA280_INIT(FT2232_PGA280_WriteData,FT2232_PGA280_ReadData);


}

static void FT2232_PGA280_WriteData(unsigned char * sendbuf , unsigned char buflen)
{

    FT2232SPI_SendRecvData(EcoGSPIData.ft2232spi, buflen, 0, sendbuf, NULL);

}

static void FT2232_PGA280_ReadData(unsigned char * sendbuf , unsigned char buflen, unsigned char * readbuf, unsigned char readlen)
{

    FT2232SPI_SendRecvData(EcoGSPIData.ft2232spi, buflen, readlen, sendbuf, readbuf);

}
