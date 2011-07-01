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

//disponível somente durante desenvolvimento
typedef struct ECOG_GLUE
{

  FT2232SPI * ft2232spi;
  PGA280 * pga280;
  ADS1259 * ads1259;

  unsigned char inBuf[256];
  unsigned char * inBufPtr;

  unsigned char dataAvailable;

  unsigned char enableAlerts;

  //função de alerta de dados disponíveis
  void (* dataAvailableAlert)(unsigned char);

} ECOGSPI;

//interface com o mundo externo

int ECOGSPI_Init(unsigned char useChecksum); //inicialização da interface
int ECOGSPI_HwConfig(void); //configuração inicial do hardware
void ECOGSPI_Cycle(void); //Ciclo de execução
int ECOGSPI_StartHandling(void); //Inicia tratamento de ciclos
void ECOGSPI_StopHandling(void); //para tratamento de ciclos (para ciclos)
unsigned char ECOGSPI_DataAvailable(void); //retorna quantos pacotes de dados estão disponíveis
void ECOGSPI_ReadBuffer(unsigned char * dest,unsigned char offset, unsigned char count); //pega dados do buffer
unsigned char ECOGSPI_ReadBufferByte(unsigned char offset); //pega um byte do buffer

void ECOGSPI_EnableAlerts(void); //habilita alertas
void ECOGSPI_DisableAlerts(void); //desabilita alertas

unsigned char ECOGSPI_AlertsEnabled(void); //retorna se alertas estão habilitados ou não

#endif // ECOG_SPI_H_INCLUDED
