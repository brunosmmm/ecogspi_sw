#ifndef ECOG_SPI_H_INCLUDED
#define ECOG_SPI_H_INCLUDED

//interface de "colagem" entre as bibliotecas de comunicação
//para uso na placa - deveria ser em python?? SWIG

/**DEFINES**/

//Pinos LOW
#define ECOGSPI_PIN_START 0x10
#define ECOGSPI_PIN_ADRST 0x40
#define ECOGSPI_PIN_DRDY  0x20
#define ECOGSPI_PIN_UCRST 0x80


#include "ft2232_spi.h"
#include "pga280.h"
#include "ads1259.h"

typedef struct ECOG_GLUE
{

  FT2232SPI * ft2232spi;
  PGA280 * pga280;
  ADS1259 * ads1259;

  unsigned char inBuf[256];
  unsigned char * inBufPtr;

  unsigned char dataAvailable;

} ECOGSPI;

int ECOGSPI_Init(unsigned char useChecksum); //inicialização da interface
int ECOGSPI_HwConfig(void); //configuração inicial do hardware
void ECOGSPI_Cycle(void); //Ciclo de execução
int ECOGSPI_StartHandling(void); //Inicia tratamento de ciclos
void ECOGSPI_StopHandling(void); //para tratamento de ciclos (para ciclos)


#endif // ECOG_SPI_H_INCLUDED
