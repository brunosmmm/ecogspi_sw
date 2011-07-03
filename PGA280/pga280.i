/* Interface SWIG para o PGA280 */

%module PGA280

%{
#include "pga280.h"
%}

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
    
void PGA280_EnableSyncIn(PGA280 * data); //habilita função sync in
void PGA280_DisableSyncIn(PGA280 * data); //desabilita função sync in
