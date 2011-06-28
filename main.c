#include <stdio.h>
#include <stdlib.h>

#include "ft2232_spi.h"
#include "ecog_spi.h"

unsigned char PGA_REGISTER_VALUES[13];

unsigned char buf[10];

int i = 0;

extern ECOGSPI EcoGSPIData;

int main()
{

  ECOGSPI_Init();

  //leitura inicial completa

  //leitura inicial utilizando biblioteca PGA280
  PGA280_Update(EcoGSPIData.pga280);

  printf("valores dos registradores do PGA280\n");

  for (i = 0; i < 13; i++) printf("valor r%d: 0x%x\n",i,PGA280_GetData(EcoGSPIData.pga280,i));

  //configura PGA280

  PGA280_WriteGPIOState(EcoGSPIData.pga280,0x01);

  PGA280_WriteGPIODirection(EcoGSPIData.pga280,0x01);

  PGA280_SetECSMode(EcoGSPIData.pga280,0x01);

  //FT2232SPI_SendRecvData(ft2232Data,2,0,PGA_CONF_ECS,NULL,FT2232SPI_RW_ASSERTCS); //seta GPIO0 como ECS

  //seleciona canal 1

  PGA280_SelectChannel(EcoGSPIData.pga280,PGA280_CHAN_1);

  //seta ganho

  PGA280_SetGain(EcoGSPIData.pga280,PGA280_GAIN_64);

  //PGA280_EnableGainMultiplier(EcoGSPIData.pga280);

  //tenta resetar o conversor A/D
  //FT2232SPI_SetLowBitsState(ft2232Data, 0x00);
  FT2232SPI_SetLowBitsState(EcoGSPIData.ft2232spi, 0x40); //levanta ADRST, iniciando operação do A/D

  //é preciso aguardar algum tempo após o reset do ADS1259 para iniciar as operações com ele

  usleep(10000);

  //comando SDATAC - é necessário para sair do modo RDATAC

  ADS1259_StopContinuous(EcoGSPIData.ads1259);

  //habilita syncout

  ADS1259_EnableSyncOut(EcoGSPIData.ads1259);

  //lê os registradores do ADS1259

  ADS1259_FullUpdate(EcoGSPIData.ads1259);

  //FT2232SPI_SendRecvData(EcoGSPIData.ft2232spi,2,0,PGA_ECS_ADS_SDATAC,NULL,FT2232SPI_RW_ASSERTCS);

  //FT2232SPI_SendRecvData(EcoGSPIData.ft2232spi,3,9,PGA_ECS_ADS_READ_0,buf,FT2232SPI_RW_ASSERTCS);

  //configura pga para utilizar syncin

  //PGA280_WriteRegister(EcoGSPIData.pga280,0x0C,0x40);

  printf("registradores do ADS1259\n");

  for (i = 0; i < 9; i++) printf("valor de r%d : 0x%x\n",i, ADS1259_GetData(EcoGSPIData.ads1259,i));

  /*FT2232SPI_SendRecvData(ft2232Data,2,0,PGA_ECS_ADS_SDATAC,NULL,FT2232SPI_RW_ASSERTCS);


    //habilita syncout
    FT2232SPI_SendRecvData(ft2232Data,4,0,PGA_ECS_ADS_SYNCOUT,NULL,FT2232SPI_RW_ASSERTCS);


    //configuração do PGA280 pós-config ADS1259

    FT2232SPI_SendRecvData(ft2232Data,2,0,PGA_CONF_SYNC,NULL,FT2232SPI_RW_ASSERTCS); //configura oscilador para utilizar SYNCOUT do ADS125

    //ok, this is a mess but at least we can do something interessant now!

    //lê os registradores do ADS1259
    FT2232SPI_SendRecvData(ft2232Data,3,9,PGA_ECS_ADS_READ_0,buf,FT2232SPI_RW_ASSERTCS);

    printf("valores dos registradores do ADS1259\n");

    int i;
    for (i = 0; i < 9; i++)
        printf("valor de r%d: 0x%x\n",i,buf[i]);*/


  printf("FIM\n");
  return 0;
}
