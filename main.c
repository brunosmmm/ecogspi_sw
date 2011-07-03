#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "ecog_spi.h"

#define SAMPLE_RATE 400
int sampleBuf[SAMPLE_RATE];

extern ECOGSPI EcoGSPIData;

void ex_program(int sig);

alertCallback dataAvailableAlert(int offset);

unsigned int x = 0;

int main()
{
  //int dataCount = 0;
  //int dataAvailable = 0;

  //int i = 0;

  //pega sinal de término
  (void) signal(SIGINT, ex_program);

  //inicializa estruturas de dados
  ECOGSPI_Init(0);

  //inicializa hardware
  ECOGSPI_HwConfig();

  //configura alerta
  ECOGSPI_SetDataAvailableAlert(dataAvailableAlert);

  //habilita alertas
  ECOGSPI_EnableAlerts();

  //inicia ciclos de execução
  ECOGSPI_StartHandling();

  //loop infinito
  while(1);


  printf("FIM\n");
  return 0;
}

void ex_program(int sig) {
  int i = 0;
 printf("Parando threads e saindo...\n");

 ECOGSPI_StopHandling();

 printf("%d amostras coletadas\n",x);

 for (i = 0; i < x; i++)

   printf("%f,%f\n",(float)i/(float)SAMPLE_RATE,((float)(sampleBuf[i])/(float)(ADS1259_VAL_MAX)));

 exit(0);
}

alertCallback dataAvailableAlert(int offset)
{

  if (x > SAMPLE_RATE-1) ex_program(0);

  sampleBuf[x] = (ECOGSPI_ReadBufferByte(offset) << 24) | (ECOGSPI_ReadBufferByte(offset+1) < 16) | (ECOGSPI_ReadBufferByte(offset+2) << 8);

  x++;
  //printf("saindo\n");

}
