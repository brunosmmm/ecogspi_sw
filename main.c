#include <stdio.h>
#include <stdlib.h>

#include "ft2232_spi.h"

const unsigned char PGA_RESET[2] = {0x41, 0x01};
const unsigned char PGA_READ_0_DUMMY[2] = {0x80, 0x00};
const unsigned char PGA_READ_1_DUMMY[2] = {0x81, 0x00};
const unsigned char PGA_READ_2_DUMMY[2] = {0x82, 0x00};
const unsigned char PGA_READ_3_DUMMY[2] = {0x83, 0x00};
const unsigned char PGA_READ_4_DUMMY[2] = {0x84, 0x00};
const unsigned char PGA_READ_5_DUMMY[2] = {0x85, 0x00};
const unsigned char PGA_READ_6_DUMMY[2] = {0x86, 0x00};
const unsigned char PGA_READ_7_DUMMY[2] = {0x87, 0x00};
const unsigned char PGA_READ_8_DUMMY[2] = {0x88, 0x00};
const unsigned char PGA_READ_9_DUMMY[2] = {0x89, 0x00};
const unsigned char PGA_READ_10_DUMMY[2] = {0x8A, 0x00};
const unsigned char PGA_READ_11_DUMMY[2] = {0x8B, 0x00};
const unsigned char PGA_READ_12_DUMMY[2] = {0x8C, 0x00};

const unsigned char PGA_CONF_SPI[2] = {0x42, 0x00};
const unsigned char PGA_CONF_GPIO[2] = {0x48, 0x01};
const unsigned char PGA_CONF_GPIO_STATE[2] = {0x45, 0x01};
const unsigned char PGA_CONF_ECS[2] = {0x49, 0x01};
const unsigned char PGA_CONF_SYNC[2] = {0x4C, 0x40};

const unsigned char PGA_ECS_ADS_READ_0[3] = {0xC0, 0x20, 0x07};
const unsigned char PGA_ECS_ADS_RESET[2] = {0xC0, 0x06};
const unsigned char PGA_ECS_ADS_SDATAC[2] = {0xC0, 0x11};
const unsigned char PGA_ECS_ADS_SYNCOUT[5] = {0xC0, 0x42, 0x00, 0x20};


unsigned char PGA_REGISTER_VALUES[13];

unsigned char buf[10];

FT2232SPI * ft2232Data;
int teste = 0;

int main()
{

	//inicializa ft2232 para spi
	ft2232Data = FT2232SPI_INIT(0,0,FT2232SPI_CPHA1,TRUE,1200);

	teste = FT2232SPI_HWINIT(ft2232Data,FTDI_VID,FTDI_FT2232H_PID,INTERFACE_A);

	//seta direção dos pinos
	FT2232SPI_SetLowBitsDirection(ft2232Data, 0xD0);

	//envia reset
	FT2232SPI_SendRecvData(ft2232Data,2,0,PGA_RESET,NULL,FT2232SPI_RW_ASSERTCS);

    printf("valores dos registradores do PGA280\n");

	FT2232SPI_SendRecvData(ft2232Data,1,1,PGA_READ_0_DUMMY,PGA_REGISTER_VALUES,FT2232SPI_RW_ASSERTCS);

	printf("valor r0: 0x%x\n",PGA_REGISTER_VALUES[0]);

	FT2232SPI_SendRecvData(ft2232Data,1,1,PGA_READ_1_DUMMY,&PGA_REGISTER_VALUES[1],FT2232SPI_RW_ASSERTCS);

	printf("valor r1: 0x%x\n",PGA_REGISTER_VALUES[1]);

	FT2232SPI_SendRecvData(ft2232Data,1,1,PGA_READ_2_DUMMY,&PGA_REGISTER_VALUES[2],FT2232SPI_RW_ASSERTCS);

	printf("valor r2: 0x%x\n",PGA_REGISTER_VALUES[2]);

	FT2232SPI_SendRecvData(ft2232Data,1,1,PGA_READ_3_DUMMY,&PGA_REGISTER_VALUES[3],FT2232SPI_RW_ASSERTCS);

	printf("valor r3: 0x%x\n",PGA_REGISTER_VALUES[3]);

	FT2232SPI_SendRecvData(ft2232Data,1,1,PGA_READ_4_DUMMY,&PGA_REGISTER_VALUES[4],FT2232SPI_RW_ASSERTCS);

	printf("valor r4: 0x%x\n",PGA_REGISTER_VALUES[4]);

	FT2232SPI_SendRecvData(ft2232Data,1,1,PGA_READ_5_DUMMY,&PGA_REGISTER_VALUES[5],FT2232SPI_RW_ASSERTCS);

	printf("valor r5: 0x%x\n",PGA_REGISTER_VALUES[5]);

	FT2232SPI_SendRecvData(ft2232Data,1,1,PGA_READ_6_DUMMY,&PGA_REGISTER_VALUES[6],FT2232SPI_RW_ASSERTCS);

	printf("valor r6: 0x%x\n",PGA_REGISTER_VALUES[6]);

	FT2232SPI_SendRecvData(ft2232Data,1,1,PGA_READ_7_DUMMY,&PGA_REGISTER_VALUES[7],FT2232SPI_RW_ASSERTCS);

	printf("valor r7: 0x%x\n",PGA_REGISTER_VALUES[7]);

	FT2232SPI_SendRecvData(ft2232Data,1,1,PGA_READ_8_DUMMY,&PGA_REGISTER_VALUES[8],FT2232SPI_RW_ASSERTCS);

	printf("valor r8: 0x%x\n",PGA_REGISTER_VALUES[8]);

	FT2232SPI_SendRecvData(ft2232Data,1,1,PGA_READ_9_DUMMY,&PGA_REGISTER_VALUES[9],FT2232SPI_RW_ASSERTCS);

	printf("valor r9: 0x%x\n",PGA_REGISTER_VALUES[9]);

	FT2232SPI_SendRecvData(ft2232Data,1,1,PGA_READ_10_DUMMY,&PGA_REGISTER_VALUES[10],FT2232SPI_RW_ASSERTCS);

	printf("valor r10: 0x%x\n",PGA_REGISTER_VALUES[10]);

	FT2232SPI_SendRecvData(ft2232Data,1,1,PGA_READ_11_DUMMY,&PGA_REGISTER_VALUES[11],FT2232SPI_RW_ASSERTCS);

	printf("valor r11: 0x%x\n",PGA_REGISTER_VALUES[11]);

	FT2232SPI_SendRecvData(ft2232Data,1,1,PGA_READ_12_DUMMY,&PGA_REGISTER_VALUES[12],FT2232SPI_RW_ASSERTCS);

	printf("valor r12: 0x%x\n",PGA_REGISTER_VALUES[12]);

	//leitura inicial completa

    //configura PGA280
    FT2232SPI_SendRecvData(ft2232Data,2,0,PGA_CONF_SPI,NULL,FT2232SPI_RW_ASSERTCS);   //config comunicação SPI

    FT2232SPI_SendRecvData(ft2232Data,2,0,PGA_CONF_GPIO_STATE,NULL,FT2232SPI_RW_ASSERTCS); //seta estado dos GPIOs, antes de mudar a direção p/ evitar resultados inesperados

	FT2232SPI_SendRecvData(ft2232Data,2,0,PGA_CONF_GPIO,NULL,FT2232SPI_RW_ASSERTCS); //seta direção dos GPIOs

	FT2232SPI_SendRecvData(ft2232Data,2,0,PGA_CONF_ECS,NULL,FT2232SPI_RW_ASSERTCS); //seta GPIO0 como ECS

	//configurado

	//tenta resetar o conversor A/D
	//FT2232SPI_SetLowBitsState(ft2232Data, 0x00);
	FT2232SPI_SetLowBitsState(ft2232Data, 0x40); //levanta ADRST, iniciando operação do A/D

	//comando SDATAC - é necessário para sair do modo RDATAC
	FT2232SPI_SendRecvData(ft2232Data,2,0,PGA_ECS_ADS_SDATAC,NULL,FT2232SPI_RW_ASSERTCS);

	//habilita syncout
	FT2232SPI_SendRecvData(ft2232Data,4,0,PGA_ECS_ADS_SYNCOUT,NULL,FT2232SPI_RW_ASSERTCS);

    //lê os registradores do ADS1259
    FT2232SPI_SendRecvData(ft2232Data,3,9,PGA_ECS_ADS_READ_0,buf,FT2232SPI_RW_ASSERTCS);


    //configuração do PGA280 pós-config ADS1259

    FT2232SPI_SendRecvData(ft2232Data,2,0,PGA_CONF_SYNC,NULL,FT2232SPI_RW_ASSERTCS); //configura oscilador para utilizar SYNCOUT do ADS1259

    //ok, this is a mess but at least we can do something interessant now!

    printf("valores dos registradores do ADS1259\n");

    int i;
    for (i = 0; i < 9; i++)
    printf("valor de r%d: 0x%x\n",i,buf[i]);


    printf("FIM\n");
    return 0;
}
