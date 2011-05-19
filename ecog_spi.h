#ifndef ECOG_SPI_H_INCLUDED
#define ECOG_SPI_H_INCLUDED

//interface de "colagem" entre as bibliotecas de comunicação
//para uso na placa - deveria ser em python??

#include "ft2232_spi.h"
#include "pga280.h"

typedef struct ECOG_GLUE
{

  FT2232SPI * ft2232spi;
  PGA280 * pga280;

} ECOGSPI;

int ECOGSPI_Init(void); //inicialização da interface


#endif // ECOG_SPI_H_INCLUDED
