/********************************************************************************
 * Interface com o CI conversor A/D ADS1259 via SPI com o FT2232H e PGA280
 * ******************************************************************************
 * arquivo: ads1259.h
 * autor: Bruno Morais (brunosmmm@gmail.com)
 * ******************************************************************************/
#ifndef ADS1259_H_INCLUDED
#define ADS1259_H_INCLUDED

#include <stdlib.h>
#include <string.h>

//comandos SPI
#define ADS1259_CMD_RREG     0x20 //base do comando de leitura, é necessário colocar o número do registrador nos 4 LSBs
#define ADS1259_CMD_WREG     0x40 //base do comando de escrita, é necessário colocar o número do registrador nos 4 LSBs
#define ADS1259_CMD_SDATAC   0x11 //stop read data continuous mode
#define ADS1259_CMD_RDATAC   0x10 //set read data continuous mode : nota, nesse modo é necessário enviar o comando SDATAC antes de qualquer outro comando SPI ser enviado
#define ADS1259_CMD_RDATA    0x12 //lê dados de conversão através de comando
#define ADS1259_CMD_OFSCAL   0x18 //realiza calibração de offset
#define ADS1259_CMD_GANCAL   0x19 //reliza calibração de ganho
#define ADS1259_CMD_START    0x08 //inicia conversão
#define ADS1259_CMD_STOP     0x0A //para conversão
#define ADS1259_CMD_RESET    0x06 //software reset
#define ADS1259_CMD_SLEEP    0x04 //modo sleep
#define ADS1259_CMD_WAKEUP   0x02 //sai do modo sleep

//a estruturação e funcionamento deste bloco de código é muito similar ao já implementado para o PGA280.

//NOTA: no ADS1259 é mais econômico re-escrever o máximo possível de registradores de uma só vez

//no modo RDATAC (padrão), basta esperar o pino DRDY ficar baixo e ler diretamente os valores via spi, sem ser necessário nenhum comando.

typedef struct ADS1259DATA
{

    unsigned char REG_DATA[9]; //registradores internos do ADS1259

    unsigned short DIRTY_FLAGS; //flags de escrita

    void (*ReadWriteData)(unsigned char *, unsigned char, unsigned char *, unsigned char); //ponteiro para função de leitura/escrita

} ADS1259;

ADS1259 * ADS1259_INIT(void (*ReadWriteData)(unsigned char *, unsigned char, unsigned char *, unsigned char));

unsigned char ADS1259_GetData(ADS1259 * data, unsigned char RegNum); //retorna dados de um registrador armazenado na estrutura
void ADS1259_SetData(ADS1259 * data, unsigned char RegNum, unsigned char RegVal); //seta dados de um registrador armazenado na esrtutura e flaga ele como sujo
void ADS1259_SelectiveUpdate(ADS1259 * data); //update seletivo: só atualiza os valores nos registradores flagados como sujo
void ADS1259_FullUpdate(ADS1259 * data); //update completo: força atualização de valores em todos os registradores

void ADS1259_WriteRegister(ADS1259 * data, unsigned char RegNum, unsigned char RegVal); //escreve um registrador
unsigned char ADS1259_ReadRegister(ADS1259 * data, unsigned char RegNum); //lê um registrador
void ADS1259_WriteMultiRegister(ADS1259 * data, unsigned char RegStartNum, unsigned char RegCount, unsigned char * RegValsSource); //escreve vários registradores de uma vez
void ADS1259_ReadMultiRegister(ADS1259 * data, unsigned char RegStartNum, unsigned char RegCount, unsigned char * RegValsDest); //lê vários registradores de uma vez



#endif // ADS1259_H_INCLUDED
