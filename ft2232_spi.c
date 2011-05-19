/********************************************************************************
 * Interface SPI para FT2232H com suporte a interrupções por software
 * 
 * ******************************************************************************
 * arquivo: ft2232h_spi.c
 * autor: Bruno Morais (brunosmmm@gmail.com)
 * ******************************************************************************/
#include "ft2232_spi.h"

static int send_buf(struct ftdi_context *ftdic, const unsigned char *buf,int size);
static int get_buf(struct ftdi_context *ftdic, const unsigned char *buf,int size);

//gambiarra para debug output
static int verbose = 2;

const struct usbdev_status devs_ft2232spi[] = {
	{FTDI_VID, FTDI_FT2232H_PID, OK, "FTDI", "FT2232H"},
	{FTDI_VID, FTDI_FT4232H_PID, OK, "FTDI", "FT4232H"},
	{},
};

/* Set data bits low-byte command:
 *  value: 0x08  CS=high, DI=low, DO=low, SK=low
 *    dir: 0x0b  CS=output, DI=input, DO=output, SK=output
 *
 * JTAGkey(2) needs to enable its output via Bit4 / GPIOL0
 *  value: 0x18  OE=high, CS=high, DI=low, DO=low, SK=low
 *    dir: 0x1b  OE=output, CS=output, DI=input, DO=output, SK=output
 */
static uint8_t cs_bits = 0x08;
static uint8_t out_values = 0x09; //CS e CK high
static uint8_t pindir = 0x0b;

static int send_buf(struct ftdi_context *ftdic, const unsigned char *buf,
		    int size)
{
	int r;
	r = ftdi_write_data(ftdic, (unsigned char *) buf, size);
	if (r < 0) {
		msg_perr("ftdi_write_data: %d, %s\n", r,
				ftdi_get_error_string(ftdic));
		return 1;
	}
	return 0;
}

static int get_buf(struct ftdi_context *ftdic, const unsigned char *buf,
		   int size)
{
	int r;

	while (size > 0) {
		r = ftdi_read_data(ftdic, (unsigned char *) buf, size);
		if (r < 0) {
			msg_perr("ftdi_read_data: %d, %s\n", r,
					ftdi_get_error_string(ftdic));
			return 1;
		}
		buf += r;
		size -= r;
	}
	return 0;
}

int FT2232SPI_SendRecvData(FT2232SPI* data, unsigned int writecnt, unsigned int readcnt,
		const unsigned char *writearr, unsigned char *readarr)
{
	struct ftdi_context *ftdic = &data->ftdicContext;
	static unsigned char *buf = NULL;
	/* failed is special. We use bitwise ops, but it is essentially bool. */
	int i = 0, ret = 0, failed = 0;
	int bufsize;
	static int oldbufsize = 0;

	//não é possível escrever ou ler mais de 65536 bytes por vez.

	if (writecnt > 65536 || readcnt > 65536)
		return SPI_INVALID_LENGTH;

	/* buf is not used for the response from the chip. */
	bufsize = max(writecnt + 9, 260 + 9); //wtf?
	/* Never shrink. realloc() calls are expensive. */

	//se o buffer não couber os dados a serem enviados, realoca

	if (bufsize > oldbufsize) {
		buf = realloc(buf, bufsize);
		if (!buf) {
			msg_perr("Out of memory!\n");
			exit(1);
		}
		oldbufsize = bufsize;
	}

	/*
	 * Minimize USB transfers by packing as many commands as possible
	 * together. If we're not expecting to read, we can assert CS#, write,
	 * and deassert CS# all in one shot. If reading, we do three separate
	 * operations.
	 */
	msg_pspew("Assert CS#\n");

	//desliga CS (spi ativado)

	buf[i++] = SET_BITS_LOW; //comando para setar estado dos bits
	buf[i++] = out_values & ~cs_bits; /* assertive */ //por quê?
	buf[i++] = pindir; //direção dos pinos conservada

	//se writecnt é positivo, haverá transferência

	if (writecnt) {


		//escreve com o modo SPI_WRITE_MODE
		buf[i++] = SPI_WRITE_MODE;
		buf[i++] = (writecnt - 1) & 0xff; //quantidade de bytes a escrever L
		buf[i++] = ((writecnt - 1) >> 8) & 0xff; //quantidade de bytes a escrever H
		memcpy(buf + i, writearr, writecnt); //coloca no buffer para ser executado de uma vez
		i += writecnt;
	}

	/*
	 * Optionally terminate this batch of commands with a
	 * read command, then do the fetch of the results.
	 */
	if (readcnt) {
		buf[i++] = 0x20;
		buf[i++] = (readcnt - 1) & 0xff;
		buf[i++] = ((readcnt - 1) >> 8) & 0xff;
		ret = send_buf(ftdic, buf, i);
		failed = ret;
		/* We can't abort here, we still have to deassert CS#. */
		if (ret)
			msg_perr("send_buf failed before read: %i\n",
				ret);
		i = 0;
		if (ret == 0) {
			/*
			 * FIXME: This is unreliable. There's no guarantee that
			 * we read the response directly after sending the read
			 * command. We may be scheduled out etc.
			 */
			ret = get_buf(ftdic, readarr, readcnt);
			failed |= ret;
			/* We can't abort here either. */
			if (ret)
				msg_perr("get_buf failed: %i\n", ret);
		}
	}

	msg_pspew("De-assert CS#\n");
	buf[i++] = SET_BITS_LOW;
	buf[i++] = out_values;//cs_bits;
	buf[i++] = pindir;
	ret = send_buf(ftdic, buf, i);
	failed |= ret;
	if (ret)
		msg_perr("send_buf failed at end: %i\n", ret);

	return failed ? -1 : 0;
}

//retirado de cli_output.c
int print(int type, const char *fmt, ...)
{
	va_list ap;
	int ret;
	FILE *output_type;

	switch (type) {
	case MSG_ERROR:
		output_type = stderr;
		break;
	case MSG_BARF:
		if (verbose < 2)
			return 0;
	case MSG_DEBUG:
		if (verbose < 1)
			return 0;
	case MSG_INFO:
	default:
		output_type = stdout;
		break;
	}

	va_start(ap, fmt);
	ret = vfprintf(output_type, fmt, ap);
	va_end(ap);
	return ret;
}

//flashrom.c @632
int max(int a, int b)
{
	return (a > b) ? a : b;
}


//nova estrutura FT2232SPI
FT2232SPI * FT2232SPI_INIT(unsigned char opMode, unsigned char csMode, unsigned char ckMode, unsigned char ckDiv5, unsigned short ckDiv)
{
	FT2232SPI * ftStruct;

	ftStruct = (FT2232SPI *)malloc(sizeof(FT2232SPI));

	if (!ftStruct) return NULL;

	//modo de operação

	ftStruct->OP_MODE = opMode;

	/* inicialização do modo de operação aqui */

	ftStruct->CK_MODE = ckMode;

	/* inicialização do modo de clock aqui */

	switch (ckMode)
	{

		case FT2232SPI_CPHA1:
			ftStruct->BUS_DIR_LOW = FT2232_SPI_PINVAL_MODE1;
			break;

		case FT2232SPI_CPHA0:
			ftStruct->BUS_DIR_LOW = FT2232_SPI_PINVAL_MODE2;
			break;

		default: //sem proteção para problemas localizados entre o computador e cadeira
			break;
	}

	//valor padrão
	//note que é necessário esse valor para o funcionamento da interface SPI
	ftStruct->BUS_VAL_LOW = FT2232_SPI_PINDIR;

	//valores padrão
	ftStruct->BUS_DIR_HIGH = 0x00;
	ftStruct->BUS_VAL_LOW = 0x00;


	if (ckDiv5) ftStruct->FT2232_FLAGS |= FT2232_CKDIV5;

	ftStruct->CKDIV_LOW = (ckDiv - 1) & 0xff;
	ftStruct->CKDIV_HIGH = ((ckDiv - 1) >> 8) & 0xff;


	return ftStruct;

}

//inicialização do hardware FT2232H
int FT2232SPI_HWINIT(FT2232SPI * data, unsigned int VID, unsigned int PID, unsigned int INTERFACE)
{
	int f;
	unsigned char buf[10];


	if (!data) return -1; //cai fora

	//inicializa struct da libftdi
	if (ftdi_init(&data->ftdicContext) < 0) return FT2232SPI_HWERROR;

	//inicializa o dispositivo usb através da libftdi
	f = ftdi_usb_open(&data->ftdicContext, VID, PID);

	//verifica se a inicialização funcionou
	if (f < 0 && f != -5) return FT2232SPI_HWERROR;

	//verifica o tipo, de forma que é possível também utilizar
	//FT2232D, FT2232C (não são high-speed)

	if (data->ftdicContext.type != TYPE_2232H && data->ftdicContext.type != TYPE_4232H) data->FT2232_FLAGS &= ~FT2232_CKDIV5; //não é high speed, não precisa dividir o clock

	//seleciona a interface escolhida (A ou B)
	if (ftdi_set_interface(&data->ftdicContext, INTERFACE) < 0) return FT2232SPI_HWERROR;

	//reseta o dispositivo usb
	if (ftdi_usb_reset(&data->ftdicContext) < 0) return FT2232SPI_HWERROR;

	//latency timer
	if (ftdi_set_latency_timer(&data->ftdicContext, 2) < 0); //fatal?

	//chunk size
	if (ftdi_write_data_set_chunksize(&data->ftdicContext, 256)); //fatal?

	//seta modo SPI
	if (ftdi_set_bitmode(&data->ftdicContext, 0x00, BITMODE_BITBANG_SPI) < 0) return FT2232SPI_HWERROR;


	//inicialização das configurações de hardware

	//configuração do divisor de clock
	if (!(data->FT2232_FLAGS & FT2232_CKDIV5)) {

		buf[0] = FT2232_CMD_NOCKDIV5;

		if (send_buf(&data->ftdicContext,buf,1)) return FT2232SPI_HWERROR; //erro

	}

	//comando para setar o divisor de clock:
	//byte 0: comando de divisor de clock (0x86)
	//byte 1: byte baixo do valor divisor de clock
	//byte 2: byte alto do valor divisor de clock

	buf[0] = FT2232_CMD_CKDIV;
	buf[1] = data->CKDIV_LOW;
	buf[2] = data->CKDIV_HIGH;

	//envia
	if (send_buf(&data->ftdicContext, buf, 3))
		return FT2232SPI_HWERROR;



	//desabilita o loopback de DI/DO
	buf[0] = FT2232_CMD_LOOPBACK_OFF;

	//envia
	if (send_buf(&data->ftdicContext, buf, 1))
		return FT2232SPI_HWERROR;

	//seta direção e estado dos bits da interface

	//FT2232_CMD_SETDATA_LOW (0x80) é o comando para setar a direção e valor
	//dos GPIOs baixos (ADBUS0-7)

	//byte 1: 0x80 (comando)
	//byte 2: valor
	//byte 3: direção

	buf[0] = FT2232_CMD_SETDATA_LOW; //seta ADBUS0-7
	buf[1] = data->BUS_VAL_LOW; //padrão: 0x08 -> CS idle high por padrão ou 0x09 -> CS idle high e CK idle high
	buf[2] = data->BUS_DIR_LOW; //0x0b -> SCK/MOSI/CS são saídas, MISO é entrada por padrão.
	if (send_buf(&data->ftdicContext, buf, 3)) //envia
		return FT2232SPI_HWERROR;


	return 0; //sucesso
}

double FT2232_GetClock(FT2232SPI * data)
{
	double clock;
	unsigned short divisor;

	if (data->FT2232_FLAGS & FT2232_CKDIV5) clock = 12.0;
	else clock = 60.0;

	divisor = ((data->CKDIV_HIGH)<<8) + data->CKDIV_LOW  + 2;

	return (double)(clock / (((divisor - 1) + 1) * 2));
}

int FT2232_SetClock(FT2232SPI * data, unsigned char ckDiv5, unsigned short ckDiv)
{
	unsigned char buf[4];

	if (!(ckDiv5)) {

		buf[0] = FT2232_CMD_NOCKDIV5;

		if (send_buf(&data->ftdicContext,buf,1)) return FT2232SPI_HWERROR; //erro

		//sucesso - guarda informação
		data->FT2232_FLAGS |= FT2232_CKDIV5;

	}

	//seta o divisor de clock

	buf[0] = FT2232_CMD_CKDIV;
	buf[1] = (ckDiv - 1) & 0xff;
	buf[2] = ((ckDiv - 1)>>8) & 0xff;

	//envia
	if (send_buf(&data->ftdicContext, buf, 3))
		return FT2232SPI_HWERROR;

	//sucesso - guarda informações
	data->CKDIV_LOW = buf[1];
	data->CKDIV_HIGH = buf[2];

	return 0; //sucesso
}