#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "ecog_spi.h"

#define SAMPLE_RATE 60

extern ECOGSPI EcoGSPIData;

void ex_program(int sig);

int main()
{


  int dataCount = 0;
  int dataAvailable = 0;

  int i = 0;

  int sampleBuf[SAMPLE_RATE];

  //pega sinal de término
  (void) signal(SIGINT, ex_program);

  //inicializa estruturas de dados
  ECOGSPI_Init(0);

  //inicializa hardware
  ECOGSPI_HwConfig();

  //inicia ciclos de execução
  ECOGSPI_StartHandling();

  //mutex para garantir sincronia
  while (dataCount < SAMPLE_RATE)
    {

  if (ECOGSPI_DataAvailable() > dataAvailable)
    {

    //printf("data disp = %d bytes\n",EcoGSPIData.dataAvailable);

    if (ECOGSPI_DataAvailable() - dataAvailable == 3) dataCount++;

    dataAvailable = ECOGSPI_DataAvailable();

    }

    }

  printf("Parando threads...\n");
  ECOGSPI_StopHandling();

  //forma amostras
  for (i = 0;i < SAMPLE_RATE; i++)
    {

    //sampleBuf[i] = (EcoGSPIData.inBuf[3*i] << 24) | (EcoGSPIData.inBuf[3*i+1] << 16) | (EcoGSPIData.inBuf[3*i+2] << 8);
    sampleBuf[i] = (ECOGSPI_ReadBufferByte(3*i) << 24) | (ECOGSPI_ReadBufferByte(3*i+1) < 16) | (ECOGSPI_ReadBufferByte(3*i+2) << 8);

    //printf("sample %d = %d\n",i,sampleBuf[i]);
    printf("%f,%f\n",(float)i/(float)SAMPLE_RATE,((float)(sampleBuf[i])/(float)(ADS1259_VAL_MAX)));

    }


  printf("FIM\n");
  return 0;
}

void ex_program(int sig) {
 printf("Parando threads e saindo...\n");

 ECOGSPI_StopHandling();

 exit(0);
}
