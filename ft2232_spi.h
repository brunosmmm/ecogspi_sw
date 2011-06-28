/********************************************************************************
 * Interface SPI para FT2232H com suporte a interrupções por software
 *
 * ******************************************************************************
 * arquivo: ft2232h_spi.h
 * autor: Bruno Morais (brunosmmm@gmail.com)
 * ******************************************************************************/

#ifndef FT2232_SPI_H_INCLUDED
#define FT2232_SPI_H_INCLUDED

//Header para interface spi ft2232 cirurgicamente removida do projeto flashrom


//includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <ftdi.h>
#include <stdarg.h>


//flash.h @214
#define OK 0
#define NT 1

//flash.h @222
int print(int type, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
#define MSG_ERROR	0
#define MSG_INFO	1
#define MSG_DEBUG	2
#define MSG_BARF	3
#define msg_gerr(...)	print(MSG_ERROR, __VA_ARGS__)	/* general errors */
#define msg_perr(...)	print(MSG_ERROR, __VA_ARGS__)	/* programmer errors */
#define msg_cerr(...)	print(MSG_ERROR, __VA_ARGS__)	/* chip errors */
#define msg_ginfo(...)	print(MSG_INFO, __VA_ARGS__)	/* general info */
#define msg_pinfo(...)	print(MSG_INFO, __VA_ARGS__)	/* programmer info */
#define msg_cinfo(...)	print(MSG_INFO, __VA_ARGS__)	/* chip info */
#define msg_gdbg(...)	print(MSG_DEBUG, __VA_ARGS__)	/* general debug */
#define msg_pdbg(...)	print(MSG_DEBUG, __VA_ARGS__)	/* programmer debug */
#define msg_cdbg(...)	print(MSG_DEBUG, __VA_ARGS__)	/* chip debug */
#define msg_gspew(...)	print(MSG_BARF, __VA_ARGS__)	/* general debug barf  */
#define msg_pspew(...)	print(MSG_BARF, __VA_ARGS__)	/* programmer debug barf  */
#define msg_cspew(...)	print(MSG_BARF, __VA_ARGS__)	/* chip debug barf  */

//spi.h @125
#define SPI_INVALID_LENGTH	-4

//modo de escrita
#define SPI_WRITE_MODE1 0x10 //dado sai na descida do clock
#define SPI_WRITE_MODE2 0x11 //dado sai na subida do clock
//modo de leitura
#define SPI_READ_MODE1 0x20 //sample -> subida do clock
#define SPI_READ_MODE2 0x24 //sample -> descida do clock

//pinos
#define FT2232_PINS_CS 0x08
#define FT2232_PINS_CK 0x01

//implementação bmorais
#define FT2232_CKDIV5 0x01 //flag de divisão de clock x5

#define FT2232_SPI_PINVAL_MODE1 0x09 //CPHA = 1; dados escritos na descida do clock
#define FT2232_SPI_PINVAL_MODE2 0x08 //CPHA = 0; dados escritos na subida do clock
#define FT2232_SPI_PINDIR 0x0B //direção obrigatória para realizar SPI

#define FT2232_BAD_COMMAND 0xFA //byte enviado de volta pelo FT2232 quando recebe um comando mal-formado

//comandos do dispositivo
#define FT2232_CMD_NOCKDIV5 0x8A //desativar divisor de clock por 5
#define FT2232_CMD_CKDIV 0x86 //comando para configurar divisão de clock
#define FT2232_CMD_LOOPBACK_ON 0x84 //comando para ativar loopback de DO/DI
#define FT2232_CMD_LOOPBACK_OFF 0x85 //comando para desativar loopback
#define FT2232_CMD_SETDATA_LOW 0x80 //comando para setar estado e direção dos bits baixos ADBUS0-7
#define FT2232_CMD_SETDATA_HIGH 0x82 //comando para setar estado e direção dos bits altos ACBUS0-7
#define FT2232_CMD_READDATA_LOW 0x81 //comando para ler o estado dos bits baixos ADBUS0-7
#define FT2232_CMD_READDATA_HIGH 0x83 //comando para ler o estado dos bits altos ACBUS0-7

//modo de operação
#define FT2232SPI_OPMODE_CYCLE 0 //modo de operação com ciclo de execução
#define FT2232SPI_OPMODE_ASYNC 1 //modo de operação assíncrono

//modo clock
#define FT2232SPI_CPHA1 1 //dados escritos na descida do clock
#define FT2232SPI_CPHA0 0 //dados escritos na subida do clock

//erros
#define FT2232SPI_HWERROR 1

//controle de CS na escrita/leitura
#define FT2232SPI_RW_ASSERTCS      0x01 //baixa cs antes da escrita/leitura e levanta no final (normal)
#define FT2232SPI_RW_HOLDCS        0x02 //não baixa cs antes da escrita e nem levanta no final (depende do estado inicial de cs, não faz nada)
#define FT2232SPI_RW_ASSERTHOLDCS  0x03 //baixa cs antes da escrita e não levanta no final
#define FT2232SPI_RW_NONE          0x00 //não baixa cs antes da escrita, mas levanta no final

//tipos de interrupção
#define FT2232SPI_INT_LEVEL_HIGH 0x01
#define FT2232SPI_INT_LEVEL_LOW  0x02
#define FT2232SPI_INT_EDGE_RISE  0x03
#define FT2232SPI_INT_EDGE_FALL  0x04

//etc
#ifndef TRUE
#define TRUE 0x01
#endif

#ifndef FALSE
#define FALSE 0x00
#endif

#define FTDI_VID		0x0403
#define FTDI_FT2232H_PID	0x6010
#define FTDI_FT4232H_PID	0x6011

#define BITMODE_BITBANG_SPI	2

//declaração retirada de programmer.h @477 (modificado)
struct usbdev_status
{
    uint16_t vendor_id;
    uint16_t device_id;
    int status;
    const char *vendor_name;
    const char *device_name;
};

//flash.h
int max(int a, int b);

//implementação real

//armazena estado e configurações da interface SPI no FT2232H
typedef struct FT2232SPIDATA
{

    unsigned char OP_MODE; //modo de operação

    unsigned char BUS_VAL_LOW; //estado dos pinos baixos (ADBUS0-7)
    unsigned char BUS_VAL_HIGH; //estado dos pinos altos (ACBUS0-7)
    unsigned char BUS_DIR_LOW; //direção dos pinos baixos (ADBUS0-7)
    unsigned char BUS_DIR_HIGH; //direção dos pinos altos (ACBUS0-7)

    unsigned char FT2232_FLAGS; //flags do FT2232H

    unsigned char CKDIV_LOW; //divisor de clock, byte baixo
    unsigned char CKDIV_HIGH; //divisor de clock, byte alto

    //configuração em alto nível
    unsigned char CK_MODE; //modo do clock -> dados gravados na subida/descida
    unsigned char CS_MODE; //modo do cs -> ativo alto/baixo

    //habilita interrupções por software no modo cíclico
    unsigned char EnableInterrupts;

    //mascaramento de "interrupções" em ACBUS0-7  e ADBUS0-7, respectivamente
    unsigned char InterruptMaskHigh;
    unsigned char InterruptMaskLow;

    //valor de interrupção -> 0 ou 1
    unsigned char InterruptValueHigh;
    unsigned char InterruptValueLow;

    //tipo de interrupção: borda (0) ou nível (1)
    unsigned char InterruptTypeHigh;
    unsigned char InterruptTypeLow;

    //handler de interrupção:
    //param1 = struct FT2232SPIDATA que disparou o tratamento
    //param2 = tipo de interrupção
    //param3 = causador da interrupção (16 bits, representa High (MSB) e Low (LSB) concatenados)
    void (*InterruptHandler)(struct FT2232SPIDATA*,unsigned char,unsigned short);
    //struct da libftdi
    struct ftdi_context ftdicContext;

    //comando de envio, depende de CPHA
    unsigned char SEND_MODE;

    //modo de leitura (Sample na subida ou descida do clock)
    unsigned char READ_MODE;

} FT2232SPI;

//aloca espaço para uma nova estrutura FT2232SPI e inicializa
FT2232SPI * FT2232SPI_INIT(unsigned char opMode, unsigned char csMode, unsigned char ckMode, unsigned char ckDiv5, unsigned short ckDiv,
                            void (*InterruptHandler)(struct FT2232SPIDATA*,unsigned char,unsigned short));
//libera memória ocupada por uma estrutura FT2232SPI
void FT2232SPI_FREE(FT2232SPI * data);

int FT2232SPI_HWINIT(FT2232SPI * data, unsigned int VID, unsigned int PID, unsigned int INTERFACE); //inicialização do hardware

void FT2232SPI_CYCLE(FT2232SPI * data); //ciclo de excecução

//habilita CS
void FT2232SPI_EnableCS(FT2232SPI * data);

//desabilita CS
void FT2232SPI_DisableCS(FT2232SPI * data);

double FT2232_GetClock(FT2232SPI * data); //retorna valor do clock

int FT2232_SetClock(FT2232SPI * data, unsigned char ckDiv5, unsigned short ckDiv); //seta clock (divisor)

unsigned char FT2232SPI_GetLowBitsState(FT2232SPI * data); //lê o estado dos bits (pinos) baixos imediatamente
unsigned char FT2232SPI_GetHighBitsState(FT2232SPI * data); //lê o estado dos bits altos imediatamente

unsigned char FT2232SPI_GetLowBitsLastState(FT2232SPI * data); //lê o último estado dos bits (pinos) baixos
unsigned char FT2232SPI_GetHighBitsLastState(FT2232SPI * data); //lê o último estado dos bits altos

void FT2232SPI_SetLowBitsState(FT2232SPI * data, unsigned char state); //escreve o estado dos bits (pinos) baixos imediatamente
void FT2232SPI_SetHighBitsState(FT2232SPI * data, unsigned char state); //escreve o estado dos bits altos imediatamente

void FT2232SPI_SetLowBitsDirection(FT2232SPI * data, unsigned char direction);
void FT2232SPI_SetHighBitsDirection(FT2232SPI * data, unsigned char direction);

//envia e recebe dados
int FT2232SPI_SendRecvData(FT2232SPI* data, unsigned int writecnt, unsigned int readcnt,
                           const unsigned char *writearr, unsigned char *readarr, unsigned char CSControl); //holdCS = 1 não desativa CS ao final da transmissão

void FT2232SPI_EnableInterrupts(FT2232SPI * data);
void FT2232SPI_DisableInterrupts(FT2232SPI * data);

void FT2232SPI_SetCKMode(FT2232SPI * data, unsigned char CKMode); //seta modo do clock


#endif // FT2232_SPI_H_INCLUDED
