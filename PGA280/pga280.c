/********************************************************************************
 * Interface com o CI amplificador de instrumentação programável PGA280 via SPI
 * com o FT2232H
 * ******************************************************************************
 * arquivo: pga280.c
 * autor: Bruno Morais (brunosmmm@gmail.com)
 * ******************************************************************************/

/**TO-DO: Implementar suporte a comunicação com Checksum**/

#include "pga280.h"

//valores padrão dos registradores do PGA280
static const unsigned char PGA280_DEFAULT[13] = {0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00};

PGA280* PGA280_INIT(void (*ReadWriteDataFunc)(unsigned char *, unsigned char, unsigned char *, unsigned char))
    {
  PGA280* pgaData;

  pgaData = (PGA280*)malloc(sizeof(PGA280));

  if (!pgaData) return NULL;

  //coloca valores default nos registradores -> por quê??
  memcpy(pgaData->REG_DATA,PGA280_DEFAULT,13);

  //seta funções de leitura e escrita em baixo nível
  pgaData->ReadWriteData = ReadWriteDataFunc;

  //reseta (software) pga280
  PGA280_WriteRegister(pgaData,0x01,0x01);

  return pgaData;
    }

void PGA280_FREE(PGA280 * data)
{
  if (!data) return;

  free(data);

}

unsigned char PGA280_GetData(PGA280 * data, unsigned char RegNum)
{
  //problemas
  if ((RegNum < 0) || (RegNum > 12) || (!data)) return 0xFF;

  //retorna o valor presente em REG_DATA
  return data->REG_DATA[RegNum];
}

void PGA280_SetData(PGA280 * data, unsigned char RegNum, unsigned char RegVal)
{
  //problemas
  if ((RegNum < 0) || (RegNum > 12) || (!data)) return;

  //seta o valor de REG_DATA no índice selecionado
  data->REG_DATA[RegNum] = RegVal;

  //flaga o bit xx como sujo
  data->DIRTY_FLAGS |= (1<<RegNum);

  return;
}

void PGA280_Update(PGA280 * data)
{
  int i = 0;

  if (!data) return;

  //realiza escrita

  //verifica se é necessário escrever os registradores de 0 à 12

  for (i = 0; i < 13; i++)
    {

    if (data->DIRTY_FLAGS & (1<<i)) PGA280_WriteRegister(data,i,data->REG_DATA[i]);

    //limpa os flags

    data->DIRTY_FLAGS &= ~(1<<i);

    }

  //realiza leitura

  for (i = 0; i < 13; i++)

    data->REG_DATA[i] = PGA280_ReadRegister(data,i);

  //pronto

}


void PGA280_WriteRegister(PGA280 * data, unsigned char RegNum, unsigned char RegVal)
{
  //dados para envio
  unsigned char sendbuf[2];

  if ((RegNum < 0) || (RegNum > 12) || (!data)) return;

  //formação do comando
  sendbuf[0] = PGA280_CMD_WRITE | (RegNum & 0x0F); //comando de escrita no registrador RegNum
  sendbuf[1] = RegVal; //valor

  (data->ReadWriteData)(sendbuf,2,NULL,0);
}

unsigned char PGA280_ReadRegister(PGA280 * data, unsigned char RegNum)
{

  //dados para envio e recepção
  unsigned char sendbuf;
  unsigned char recvbuf;

  if ((RegNum < 0) || (RegNum > 12) || (!data)) return 0xFF;

  //formação do comando
  sendbuf = PGA280_CMD_READ | (RegNum & 0x0F); //comando de leitura do registrador RegNum


  (data->ReadWriteData)(&sendbuf,1,&recvbuf,1);

  return recvbuf;
}

void PGA280_EnableGainMultiplier(PGA280 * data)
{
  if (!data) return;

  //se há bits flagados como sujos, é necessário primeiro realizar um update das configurações
  if (data->DIRTY_FLAGS) PGA280_Update(data);

  //ok

  //seta nova configuração (habilita multiplicador de ganho
  data->REG_DATA[0] |= PGA280_GAINMULT;

  //flaga o registrador como sujo e realiza update para forçar escrita de novas configurações
  //assim, já temos imediatamente o resultado da nova configuração.

  //um outro método seria não flagar o bit como sujo e escrever diretamente no registrador

  data->DIRTY_FLAGS |= 0x01;

  //realiza a escrita e nova leitura
  PGA280_Update(data);

}

//função similar a de habilitação; ver comentários existentes
void PGA280_DisableGainMultiplier(PGA280 * data)
{
  if (!data) return;

  //se há bits flagados como sujos, é necessário primeiro realizar um update das configurações
  if (data->DIRTY_FLAGS) PGA280_Update(data);

  //seta nova configuração (desabilita multiplicador de ganho)
  data->REG_DATA[0] &= ~PGA280_GAINMULT;

  data->DIRTY_FLAGS |= 0x01;

  //realiza a escrita e nova leitura
  PGA280_Update(data);

}

void PGA280_SetGain(PGA280 * data, unsigned char gain)
{

  if (!data) return;

  if (data->DIRTY_FLAGS) PGA280_Update(data);

  data->REG_DATA[0] &= 0x87; //reseta ganho, mantendo estado dos demais bits de configuração do registrador 0

  data->REG_DATA[0] |= ((gain & 0x0F) << 3); //seta o ganho

  //realiza update
  data->DIRTY_FLAGS |= 0x01;

  PGA280_Update(data);

}

void PGA280_WriteGPIOState(PGA280 * data, unsigned char state)
{

  if (!data) return;

  data->REG_DATA[6] = state & 0x7F; //seta, ignorando o valor do MSB, que não é usado

  data->DIRTY_FLAGS |= (1<<6);

  PGA280_Update(data);

}

unsigned char PGA280_ReadGPIOState(PGA280 * data, unsigned char update)
{

  if (!data) return 0xFF;

  //se update for verdadeiro, realiza um update para retornar o valor mais recente

  if (update) PGA280_Update(data);


  return data->REG_DATA[6];

}

void PGA280_WriteGPIODirection(PGA280 * data, unsigned char direction)
{

  if (!data) return;

  data->REG_DATA[8] = direction & 0x7F;

  data->DIRTY_FLAGS |= (1<<8);

  PGA280_Update(data);

}

void PGA280_SetECSMode(PGA280 * data, unsigned char ecsPins)
{

  if (!data) return;

  data->REG_DATA[9] = ecsPins & 0x7F;

  data->DIRTY_FLAGS |= (1<<9);

  PGA280_Update(data);

}

void PGA280_SelectChannel(PGA280 * data, unsigned char channel)
{

  if (!data) return;

  switch (channel)
  {
  case PGA280_CHAN_1:

    //canal1 : abre chaves do canal 2
    data->REG_DATA[6] &= ~(0x18);

    //fecha chaves do canal 1
    data->REG_DATA[6] |= 0x60;

    //o resto das chaves tem seu estado mantido
    break;

  case PGA280_CHAN_2:

    //abre chaves do canal 1
    data->REG_DATA[6] &= ~(0x60);

    //fecha chaves do canal 2
    data->REG_DATA[6] |= 0x18;

    break;

  default:
    return; //não faz nada
  }

  //realiza update

  data->DIRTY_FLAGS |= (1<<6);

  PGA280_Update(data);

}

void PGA280_SetBufferTimeout(PGA280 * data, unsigned char timeout)
{

  if (!data) return;

  timeout &= 0x3F; //zera bits não usados (só por segurança)

  data->REG_DATA[3] = timeout;

  //realiza update
  data->DIRTY_FLAGS |= (1<<3);

  PGA280_Update(data);

}

unsigned char PGA280_GetErrors(PGA280 * data)
{

  if (!data) return 0xFF;

  //realiza update
  PGA280_Update(data);

  //retorna registrador de erro
  return data->REG_DATA[4];

}

void PGA280_SetErrorSupressionTime(PGA280 * data, unsigned char timeout)
{

  if (!data) return;

  timeout &= 0x3C; //zera bits que não são controle do timeout

  data->DIRTY_FLAGS |= (1<<11);

  PGA280_Update(data);

}

void PGA280_EnableMuxControl(PGA280 * data, unsigned char MUX0, unsigned char MUX1, unsigned char MUX2)
{
  if (!data) return;

  //GPIOs precisam ser saídas
  data->REG_DATA[8] |= 0x07;

  //reseta o valor de MUX-D no registrador 10
  data->REG_DATA[10] &= ~(0x80);

  //habilita controle para os bits selecionados
  data->REG_DATA[12] |= ((MUX0) | (MUX1<<1) | (MUX2<<2));

  //update
  data->DIRTY_FLAGS |= ((1<<8) | (1<<10) | (1<<12));

  PGA280_Update(data);

}

void PGA280_DisableMuxControl(PGA280 * data)
{

  if (!data) return;

  //reseta bits de seleção (não é necessário)
  data->REG_DATA[0] &= ~(0x07);

  //é necessário resetar o estado e direção dos GPIOs manualmente!!

  //seta MUX-D
  data->REG_DATA[10] |= 0x80;

  //reseta bits de controle
  data->REG_DATA[12] &= ~(0x07);

  //update
  data->DIRTY_FLAGS |= (0x01 | (1<<10) | (1<<12));

  PGA280_Update(data);

}

void PGA280_ControlMux(PGA280 * data, unsigned char select)
{

  if (!data) return;

  //controla mux pelo registrador 0

  //reseta bits de controle do MUX
  data->REG_DATA[0] &= 0xF8;

  //seta estado
  data->REG_DATA[0] |= (select & 0x07);

  //update
  data->DIRTY_FLAGS |= 0x01;

  PGA280_Update(data);

}

void PGA280_ECS_ReadWriteData(PGA280 * data, unsigned char * sendbuf, unsigned char buflen,
    unsigned char * recvbuf, unsigned char recvlen, unsigned char ecs)
{
  unsigned char sendData[buflen+1];

  if (!data) return;

  //forma o comando ecs

  sendData[0] = 0xC0 | (ecs & 0x0F);

  //copia buffer para o buffer local de envio

  memcpy(&sendData[1],sendbuf,buflen);

  //envia comando de ecs + dados para escrita
  //e lê dados

  (data->ReadWriteData)(sendData,buflen+1,recvbuf,recvlen);

}

void PGA280_EnableSyncIn(PGA280 * data)
{
  if (!data) return;
  
  data->REG_DATA[12] |= 0x40;
  
  //update
  data->DIRTY_FLAGS |= (1<<12);
  
  PGA280_Update(data);

}

void PGA280_DisableSyncIn(PGA280 * data)
{
  if (!data) return;
  
  data->REG_DATA[12] &= ~0x40;
  
  //update
  data->DIRTY_FLAGS |= (1<<12);
  
  PGA280_Update(data);

}
