/* Interface SWIG para ADS1259*/
%module ADS1259

%{
#include "ads1259.h"
%}

unsigned char ADS1259_GetData(ADS1259 * data, unsigned char RegNum); //retorna dados de um registrador armazenado na estrutura
void ADS1259_SetData(ADS1259 * data, unsigned char RegNum, unsigned char RegVal); //seta dados de um registrador armazenado na esrtutura e flaga ele como sujo
void ADS1259_SelectiveUpdate(ADS1259 * data); //update seletivo: só atualiza os valores nos registradores flagados como sujo
void ADS1259_FullUpdate(ADS1259 * data); //update completo: força atualização de valores em todos os registradores

void ADS1259_WriteRegister(ADS1259 * data, unsigned char RegNum, unsigned char RegVal); //escreve um registrador
unsigned char ADS1259_ReadRegister(ADS1259 * data, unsigned char RegNum); //lê um registrador
void ADS1259_WriteMultiRegister(ADS1259 * data, unsigned char RegStartNum, unsigned char RegCount, unsigned char * RegValsSource); //escreve vários registradores de uma vez
void ADS1259_ReadMultiRegister(ADS1259 * data, unsigned char RegStartNum, unsigned char RegCount, unsigned char * RegValsDest); //lê vários registradores de uma vez

void ADS1259_Reset(ADS1259 * data);
void ADS1259_StopContinuous(ADS1259 * data);
void ADS1259_StartContinuous(ADS1259 * data);

void ADS1259_EnableSyncOut(ADS1259 * data);
void ADS1259_DisableSyncOut(ADS1259 * data);

//controla taxa de amostragem
void ADS1259_SetSampleRate(ADS1259 * data, unsigned char sampleRate);
