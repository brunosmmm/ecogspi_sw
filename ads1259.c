/********************************************************************************
 * Interface com o CI conversor A/D ADS1259 via SPI com o FT2232H e PGA280
 * ******************************************************************************
 * arquivo: ads1259.c
 * autor: Bruno Morais (brunosmmm@gmail.com)
 * ******************************************************************************/

 #include "ads1259.h"

/**Várias funções foram extraídas da biblioteca PGA280 e modificadas para funcionar aqui**/
static const unsigned char ADS1259_DEFAULT[9] = {0x25, 0x08, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x40} ;

//inicializa estruturas de dados do ads1259
ADS1259 * ADS1259_INIT(void (*ReadWriteData)(unsigned char *, unsigned char, unsigned char *, unsigned char))
{
    ADS1259 * adsData;

    adsData = (ADS1259*)malloc(sizeof(ADS1259));

    if (!adsData) return NULL;

    memcpy(adsData->REG_DATA,ADS1259_DEFAULT,9);

    adsData->ReadWriteData = ReadWriteData;

    return adsData; //ok
}

unsigned char ADS1259_GetData(ADS1259 * data, unsigned char RegNum)
{
    //problemas
    if ((RegNum < 0) || (RegNum > 8) || (!data)) return 0xFF;

    //retorna o valor presente em REG_DATA
    return data->REG_DATA[RegNum];
}

void ADS1259_SetData(ADS1259 * data, unsigned char RegNum, unsigned char RegVal)
{
    //problemas
    if ((RegNum < 0) || (RegNum > 8) || (!data)) return;

    //seta o valor de REG_DATA no índice selecionado
    data->REG_DATA[RegNum] = RegVal;

    //flaga o bit xx como sujo
    data->DIRTY_FLAGS |= (1<<RegNum);

    return;
}

//update seletivo: só atualiza os registradores flagados como sujos
void ADS1259_SelectiveUpdate(ADS1259 * data)
{
    int i = 0;

    if (!data) return;

    //realiza escrita

    //verifica se é necessário escrever os registradores de 0 à 12

    for (i = 0; i < 9; i++)
    {

        if (data->DIRTY_FLAGS & (1<<i)) ADS1259_WriteRegister(data,i,data->REG_DATA[i]);

        //limpa os flags

        data->DIRTY_FLAGS &= ~(1<<i);

    }

    //realiza leitura

    for (i = 0; i < 9; i++)

        data->REG_DATA[i] = ADS1259_ReadRegister(data,i);

    //pronto

}

//update completo de uma só vez, é mais econômico, só precisa de um comando
void ADS1259_FullUpdate(ADS1259 * data)
{

    if (!data) return;

    //realiza escrita

    ADS1259_WriteMultiRegister(data,0x00,9,data->REG_DATA);

    //realiza leitura

    ADS1259_ReadMultiRegister(data,0x00,9,data->REG_DATA);

    //ok!
}


void ADS1259_WriteRegister(ADS1259 * data, unsigned char RegNum, unsigned char RegVal)
{
    //dados para envio
    unsigned char sendbuf[3];

	if ((RegNum < 0) || (RegNum > 8) || (!data)) return;

    //formação do comando
    sendbuf[0] = ADS1259_CMD_WREG | (RegNum & 0x0F); //comando de escrita no registrador RegNum
    sendbuf[1] = 0x00; //escreve em somente um registrador
    sendbuf[2] = RegVal; //valor

	(data->ReadWriteData)(sendbuf,3,NULL,0);
}

unsigned char ADS1259_ReadRegister(ADS1259 * data, unsigned char RegNum)
{

    //dados para envio e recepção
    unsigned char sendbuf[2];
    unsigned char recvbuf;

	if ((RegNum < 0) || (RegNum > 8) || (!data)) return 0xFF;

    //formação do comando
    sendbuf[0] = ADS1259_CMD_RREG | (RegNum & 0x0F); //comando de leitura do registrador RegNum
    sendbuf[1] = 0x00; //lê somente um registrador


    (data->ReadWriteData)(sendbuf,2,&recvbuf,1);

    return recvbuf;
}

void ADS1259_WriteMultiRegister(ADS1259 * data, unsigned char RegStartNum, unsigned char RegCount, unsigned char * RegValsSource)
{
    //dados para envio
    unsigned char sendbuf[RegCount+2];

    if ((RegStartNum > 8) || (!data)) return;

    if (RegCount > 9) RegCount = 9;

    //formação do comando
    sendbuf[0] = ADS1259_CMD_WREG | (RegStartNum & 0x0F); //comando de escrita no registrador RegNum
    sendbuf[1] = RegCount - 1;
    memcpy(sendbuf,RegValsSource,RegCount); //valores

	(data->ReadWriteData)(sendbuf,RegCount+2,NULL,0);
}

void ADS1259_ReadMultiRegister(ADS1259 * data, unsigned char RegStartNum, unsigned char RegCount, unsigned char * RegValsDest)
{

    //dados para envio e recepção
    unsigned char sendbuf[2];

	if ((RegStartNum > 8) || (!data)) return;

    //formação do comando
    sendbuf[0] = ADS1259_CMD_RREG | (RegStartNum & 0x0F); //comando de leitura do registrador RegNum
    sendbuf[1] = RegCount - 1;

    (data->ReadWriteData)(sendbuf,2,RegValsDest,RegCount);

}

void ADS1259_Reset(ADS1259 * data)
{

    unsigned char sendbuf = ADS1259_CMD_RESET;

    if (!data) return;

    (data->ReadWriteData)(&sendbuf,1,NULL,0);

}

void ADS1259_StopContinuous(ADS1259 * data)
{

    unsigned char sendbuf = ADS1259_CMD_SDATAC;

    if (!data) return;

    (data->ReadWriteData)(&sendbuf,1,NULL,0);

}

void ADS1259_EnableSyncOut(ADS1259 * data)
{

    if (!data) return;

    data->REG_DATA[2] |= 0x20;

    data->DIRTY_FLAGS |= (1<<2);

    ADS1259_SelectiveUpdate(data);

}

