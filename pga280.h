/********************************************************************************
 * Interface com o CI amplificador de instrumentação programável PGA280 via SPI
 * com o FT2232H
 * ******************************************************************************
 * arquivo: pga280.h
 * autor: Bruno Morais (brunosmmm@gmail.com)
 * ******************************************************************************/
#ifndef PGA280_H_INCLUDED
#define PGA280_H_INCLUDED

#include <stdlib.h>
#include <string.h>

//definições de tipo
typedef unsigned char BOOL;

//constantes
#ifndef TRUE
#define TRUE 0x01
#endif

#ifndef FALSE
#define FALSE 0x00
#endif

//interface SPI com o PGA280

//comandos
#define PGA280_CMD_WRITE 0x40 //comando para leitura; é necessário adicionar o índice do registrador
#define PGA280_CMD_READ  0x80 //comando para escrita é necessário adicionar o índice do registrador

//definições de configurações
#define PGA280_GAINMULT 0x80

//possíveis definições de ganho
#define PGA280_GAIN__8  0x00 //ganho de 1/8
#define PGA280_GAIN__4  0x01 //ganho de 1/4
#define PGA280_GAIN__2  0x02 //ganho de 1/2
#define PGA280_GAIN_1   0x03 //ganho de 1
#define PGA280_GAIN_2   0x04 //ganho de 2
#define PGA280_GAIN_4   0x05 //ganho de 4
#define PGA280_GAIN_8   0x06 //ganho de 8
#define PGA280_GAIN_16  0x07 //ganho de 16
#define PGA280_GAIN_32  0x08 //ganho de 32
#define PGA280_GAIN_64  0x09 //ganho de 64
#define PGA280_GAIN_128 0x0A //ganho de 128

//canais do PGA280
#define PGA280_CHAN_1 0x01 //canal 1
#define PGA280_CHAN_2 0x02 //canal 2

//possíveis erros, definidos no registrador de erro do PGA280
#define PGA280_IOVERR  0x01
#define PGA280_GAINERR 0x02
#define PGA280_OUTERR  0x04
#define PGA280_EF      0x08 //OU de todos os erros possíveis
#define PGA280_ICAERR  0x10
#define PGA280_BUFA    0x20 //Flag de buffer ativo
#define PGA280_IARERR  0x40
#define PGA280_CHKERR  0x80 //Erro de checksum

typedef struct PGA280DATA {

  unsigned char REG_DATA[13]; //dados nos registradores do PGA280

  unsigned short DIRTY_FLAGS; //flaga os bits como "sujos", serão escritos na próxima escrita geral

  void (*ReadWriteData)(unsigned char *, unsigned char, unsigned char *, unsigned char); //ponteiro para função de leitura/escrita

} PGA280;

//a função de escrita deve ter: parametro 1: endereço dos dados a escrever, param 2: tamanho dos dados
//função de leitura (escrita-leitura): param 1: endereço dos dados a escrever, param 2: tamanho dos dados
//                                     param 3: endereço dos dados lidos, param 4: tamanho dos dados lidos


PGA280* PGA280_INIT(void (*ReadWriteDataFunc)(unsigned char *, unsigned char, unsigned char *, unsigned char)); //inicialização

void PGA280_FREE(PGA280 * data); //liberação

void PGA280_EnableGainMultiplier(PGA280 * data); //habilita multiplicador de ganho (1,375x)

void PGA280_DisableGainMultiplier(PGA280 * data); //desabilita multiplicador de ganho

void PGA280_SetGain(PGA280 * data, unsigned char gain); //seta o ganho do amplificador

void PGA280_WriteRegister(PGA280 * data, unsigned char RegNum, unsigned char RegVal); //escreve dados em um registrador imediatamente

unsigned char PGA280_ReadRegister(PGA280 * data, unsigned char RegNum); //lê dados de um registrador imediatamente

unsigned char PGA280_GetData(PGA280 * data, unsigned char RegNum); //retorna dados obtidos na última leitura geral (update)

void PGA280_SetData(PGA280 * data, unsigned char RegNum, unsigned char RegVal); //escreve dados para serem colocados no pga na próxima escrita geral (update)

void PGA280_Update(PGA280 * data); //realiza uma escrita geral e depois uma leitura geral

void PGA280_WriteGPIOState(PGA280 * data, unsigned char state); //seta estado dos pinos GPIO

unsigned char PGA280_ReadGPIOState(PGA280 * data, unsigned char update); //retorna estado dos pinos GPIO, dependendo de update

void PGA280_WriteGPIODirection(PGA280 * data, unsigned char direction); //seta direção dos pinos

void PGA280_SetECSMode(PGA280 * data, unsigned char ecsPins); //seta modo ECS nos pinos selecionados

void PGA280_SelectChannel(PGA280 * data, unsigned char channel); //seleciona o canal utilizado (1 ou 2)

void PGA280_SetBufferTimeout(PGA280 * data, unsigned char timeout); //seta timeout dos buffers internos

unsigned char PGA280_GetErrors(PGA280 * data); //retorna conteúdo do registrador de erro

void PGA280_SetErrorSupressionTime(PGA280 * data, unsigned char timeout); //seta um timeout após a ativação do buffer, no qual os erros detectados são suprimidos

void PGA280_EnableMuxControl(PGA280 * data, BOOL MUX0, BOOL MUX1, BOOL MUX2); //habilita controle de multiplexador externo através de GPIO0, GPIO1, GPIO2

void PGA280_DisableMuxControl(PGA280 * data); //desabilita controle de multiplexador externo

void PGA280_ControlMux(PGA280 * data, unsigned char select); //controla multiplexador externo (necessita ser habilitado antes)

//utiliza o esquema ECS (Extended Chip Select) para selecionar outros escravos SPI e enviar/receber dados
void PGA280_ECS_ReadWriteData(PGA280 * data, unsigned char * sendbuf, unsigned char buflen,
    unsigned char * recvbuf, unsigned char recvlen, unsigned char ecs);



#endif
