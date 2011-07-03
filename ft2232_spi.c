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


//static int get_send_buf(struct ftdi_context *ftdic, const unsigned char *buf,
//    int size, unsigned char * buf2, int size2);

//gambiarra para debug output
static int verbose = 0;

const struct usbdev_status devs_ft2232spi[] =
    {
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
static int send_buf(struct ftdi_context *ftdic, const unsigned char *buf,
    int size)
{
  int r;
  r = ftdi_write_data(ftdic, (unsigned char *) buf, size);
  if (r < 0)
    {
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

  while (size > 0)
    {
    r = ftdi_read_data(ftdic, (unsigned char *) buf, size);
    if (r < 0)
      {
      msg_perr("ftdi_read_data: %d, %s\n", r,
          ftdi_get_error_string(ftdic));
      return 1;
      }
    buf += r;
    size -= r;
    }

  return 0;
}

//static int get_send_buf(struct ftdi_context *ftdic, const unsigned char *buf,
//    int size, unsigned char * buf2, int size2)
//{
//  int r;
//
//  while (size > 0)
//    {
//    r = ftdi_read_data(ftdic, (unsigned char *) buf, size);
//    if (r < 0)
//      {
//      msg_perr("ftdi_read_data: %d, %s\n", r,
//          ftdi_get_error_string(ftdic));
//      return 1;
//      }
//    buf += r;
//    size -= r;
//    }

//  //write
//  r = ftdi_write_data(ftdic, (unsigned char *) buf2, size2);
//  if (r < 0)
//    {
//    msg_perr("ftdi_write_data: %d, %s\n", r,
//        ftdi_get_error_string(ftdic));
//    return 1;
//    }
//
//  return 0;
//}

//seta o estado e direção dos GPIOs ADBUS0-7
static int busLow_setStateDir(struct ftdi_context * ftdic, unsigned char state, unsigned char direction)
{
  unsigned char buf[3];

  buf[0] = FT2232_CMD_SETDATA_LOW;
  buf[1] = state;
  buf[2] = direction;

  return send_buf(ftdic, buf, 3);
}

//seta o estado e direção dos GPIOs ACBUS0-7
static int busHigh_setStateDir(struct ftdi_context * ftdic, unsigned char state, unsigned char direction)
{
  unsigned char buf[3];

  buf[0] = FT2232_CMD_SETDATA_HIGH;
  buf[1] = state;
  buf[2] = direction;

  return send_buf(ftdic, buf, 3);
}

static int busLow_readState(struct ftdi_context * ftdic, unsigned char * dest)
{
  int retVal = 0;
  unsigned char getCmd = FT2232_CMD_READDATA_LOW;
  //unsigned char pins = 0;

  if ((retVal = send_buf(ftdic, &getCmd, 1))) return retVal;
  //if ((retVal = ftdi_read_pins(ftdic,&pins))) return retVal;

  return get_buf(ftdic, dest, 1);
  //return pins;
}

static int busHigh_readState(struct ftdi_context * ftdic, unsigned char * dest)
{
  int retVal = 0;
  unsigned char getCmd = FT2232_CMD_READDATA_HIGH;

  if ((retVal = send_buf(ftdic, &getCmd, 1))) return retVal;

  return get_buf(ftdic, dest, 1);
}

int FT2232SPI_SendRecvData(FT2232SPI* data, unsigned int writecnt, unsigned int readcnt,
    const unsigned char *writearr, unsigned char *readarr,unsigned char CSControl)
{
  struct ftdi_context *ftdic = &data->ftdicContext;
  static unsigned char *buf = NULL;
  /* failed is special. We use bitwise ops, but it is essentially bool. */
  int i = 0, ret = 0, failed = 0;
  int bufsize;
  static int oldbufsize = 0;
  
  
  if (!data) return -1;
    
  //parado, retorna
  if (data->OP_STATUS == FT2232SPI_OP_STOP) return -1;

  //espera término do ciclo para enviar dados
  //atenção: se o estado é OP_INT, realiza transferência normalmente.
  while (data->OP_STATUS == FT2232SPI_OP_CYCLE) ;  
    
  //indica que está transferindo dados
  data->OP_STATUS = FT2232SPI_OP_XFER;
      

  //não é possível escrever ou ler mais de 65536 bytes por vez.

  if (writecnt > 65536 || readcnt > 65536)
    return SPI_INVALID_LENGTH;

  /* buf is not used for the response from the chip. */
  bufsize = max(writecnt + 9, 260 + 9); //wtf?
  /* Never shrink. realloc() calls are expensive. */

  //se o buffer não couber os dados a serem enviados, realoca

  if (bufsize > oldbufsize)
    {
    buf = realloc(buf, bufsize);
    if (!buf)
      {
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

  if (CSControl & FT2232SPI_RW_ASSERTCS)
    {

    msg_pspew("Assert CS#\n");

    //desliga CS (spi ativado)

    buf[i++] = FT2232_CMD_SETDATA_LOW; //comando para setar estado dos bits
    buf[i++] = data->BUS_VAL_LOW & ~FT2232_PINS_CS; //desativa CS
    buf[i++] = data->BUS_DIR_LOW; //direção dos pinos conservada

    }
  //se writecnt é positivo, haverá transferência

  if (writecnt)
    {


    //escreve com o modo SPI_WRITE_MODE
    buf[i++] = data->SEND_MODE;
    buf[i++] = (writecnt - 1) & 0xff; //quantidade de bytes a escrever L
    buf[i++] = ((writecnt - 1) >> 8) & 0xff; //quantidade de bytes a escrever H
    memcpy(buf + i, writearr, writecnt); //coloca no buffer para ser executado de uma vez
    i += writecnt;
    }

  /*
   * Optionally terminate this batch of commands with a
   * read command, then do the fetch of the results.
   */
  if (readcnt)
    {
    buf[i++] = SPI_READ_MODE1;
    buf[i++] = (readcnt - 1) & 0xff;
    buf[i++] = ((readcnt - 1) >> 8) & 0xff;
    ret = send_buf(ftdic, buf, i);
    failed = ret;
    /* We can't abort here, we still have to deassert CS#. */
    if (ret)
      msg_perr("send_buf failed before read: %i\n",
          ret);
    i = 0;
    if (ret == 0)
      {
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
    buf[i++] = FT2232_CMD_SETDATA_LOW;
    data->BUS_VAL_LOW |= FT2232_PINS_CS;
    buf[i++] = data->BUS_VAL_LOW;
    buf[i++] = data->BUS_DIR_LOW;
    if (ret)
      msg_perr("send_buf failed at end: %i\n", ret);

  ret = send_buf(ftdic, buf, i);
  failed |= ret;
  
  
  //transferência finalizada
  data->OP_STATUS = FT2232SPI_OP_OK;

  return failed ? -1 : 0;
}

//retirado de cli_output.c
int print(int type, const char *fmt, ...)
{
  va_list ap;
  int ret;
  FILE *output_type;

  switch (type)
  {
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
FT2232SPI * FT2232SPI_INIT(unsigned char opMode, unsigned char csMode, unsigned char ckMode, unsigned char ckDiv5, unsigned short ckDiv,
    void (*InterruptHandler)(struct FT2232SPIDATA*,unsigned char,unsigned short))
    {
  FT2232SPI * ftStruct;

  ftStruct = (FT2232SPI *)malloc(sizeof(FT2232SPI));

  if (!ftStruct) return NULL;

  //modo de operação
  ftStruct->OP_MODE = opMode;

  /* inicialização do modo de operação aqui */

  ftStruct->CK_MODE = ckMode;

  /* inicialização do modo de clock aqui */


  //Tratamento de interrupções
  ftStruct->InterruptHandler = InterruptHandler;

  //limpa dados de interrupções
  ftStruct->InterruptMaskHigh = 0x00;
  ftStruct->InterruptMaskLow = 0x00;
  ftStruct->InterruptValueHigh = 0x00;
  ftStruct->InterruptValueLow = 0x00;
  ftStruct->InterruptTypeHigh = 0x00;
  ftStruct->InterruptTypeLow = 0x00;

  //desabilita interrupções
  ftStruct->EnableInterrupts = 0x00;

  switch (ckMode)
  {

  case FT2232SPI_CPHA1:
    ftStruct->BUS_VAL_LOW = FT2232_SPI_PINVAL_MODE1;
    ftStruct->SEND_MODE = SPI_WRITE_MODE1;
    break;

  case FT2232SPI_CPHA0:
    ftStruct->BUS_VAL_LOW = FT2232_SPI_PINVAL_MODE2;
    ftStruct->SEND_MODE = SPI_WRITE_MODE2;
    break;

  default: //sem proteção para problemas localizados entre o computador e cadeira
    break;
  }

  //valor padrão
  //note que é necessário esse valor para o funcionamento da interface SPI
  ftStruct->BUS_DIR_LOW = FT2232_SPI_PINDIR;

  //valores padrão
  ftStruct->BUS_DIR_HIGH = 0x00;
  ftStruct->BUS_VAL_HIGH = 0x00;


  if (ckDiv5) ftStruct->FT2232_FLAGS |= FT2232_CKDIV5;

  ftStruct->CKDIV_LOW = (ckDiv - 1) & 0xff;
  ftStruct->CKDIV_HIGH = ((ckDiv - 1) >> 8) & 0xff;


  //estrutura de dados inicializada, porém operação neste ponto: parado
  ftStruct->OP_STATUS = FT2232SPI_OP_STOP;

  return ftStruct;

    }

void FT2232SPI_SetCKMode(FT2232SPI * data, unsigned char ckMode)
{

  if (!data ) return;

  data->CK_MODE = ckMode;

  switch (ckMode)
  {

  case FT2232SPI_CPHA1:
    data->BUS_VAL_LOW |= FT2232_PINS_CK;
    data->SEND_MODE = SPI_WRITE_MODE1;
    break;

  case FT2232SPI_CPHA0:
    data->BUS_VAL_LOW &= ~FT2232_PINS_CK;
    data->SEND_MODE = SPI_WRITE_MODE2;
    break;

  default: //sem proteção para problemas localizados entre o computador e cadeira
    break;
  }

  busLow_setStateDir(&data->ftdicContext, data->BUS_VAL_LOW, data->BUS_DIR_LOW);
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
  if (ftdi_set_latency_timer(&data->ftdicContext, 0) < 0); //fatal?

  //chunk size
  if (ftdi_write_data_set_chunksize(&data->ftdicContext, 128)); //fatal?
  if (ftdi_read_data_set_chunksize(&data->ftdicContext, 128));

  //seta modo SPI
  if (ftdi_set_bitmode(&data->ftdicContext, 0x00, BITMODE_BITBANG_SPI) < 0) return FT2232SPI_HWERROR;


  //inicialização das configurações de hardware

  //configuração do divisor de clock
  if (!(data->FT2232_FLAGS & FT2232_CKDIV5))
    {

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

    
  //hardware inicializado corretamente: operação -> OK
  data->OP_STATUS = FT2232SPI_OP_OK;


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

  if (!(ckDiv5))
    {

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

//habilita CS
void FT2232SPI_EnableCS(FT2232SPI * data)
{
  unsigned char buf[3];

  if (!data) return;

  data->BUS_VAL_LOW &= ~FT2232_PINS_CS;

  buf[0] = FT2232_CMD_SETDATA_LOW; //comando para setar estado dos bits
  buf[1] = data->BUS_VAL_LOW; //desativa CS
  buf[2] = data->BUS_DIR_LOW; //direção dos pinos conservada

  send_buf(&data->ftdicContext, buf, 3);
}

//desabilita CS
void FT2232SPI_DisableCS(FT2232SPI * data)
{
  unsigned char buf[3];

  if(!data) return;

  data->BUS_VAL_LOW |= FT2232_PINS_CS;

  buf[0] = FT2232_CMD_SETDATA_LOW;
  buf[1] = data->BUS_VAL_LOW;
  buf[2] = data->BUS_DIR_LOW;

  send_buf(&data->ftdicContext, buf, 3);
}

void FT2232SPI_SetLowBitsState(FT2232SPI * data, unsigned char state)
{
  if (!data) return;

  data->BUS_VAL_LOW &= (0x0F); //apaga estados
  data->BUS_VAL_LOW |= (state & 0xF0); //preserva o estado dos 4 LSBs

  //seta estados
  busLow_setStateDir(&data->ftdicContext, data->BUS_VAL_LOW, data->BUS_DIR_LOW);
}

void FT2232SPI_SetHighBitsState(FT2232SPI * data, unsigned char state)
{
  if (!data) return;

  data->BUS_VAL_HIGH = state; //seta estados

  //seta estados
  busHigh_setStateDir(&data->ftdicContext, data->BUS_VAL_HIGH, data->BUS_DIR_HIGH);
}

unsigned char FT2232SPI_GetLowBitsLastState(FT2232SPI * data)
{
  if (!data) return 0xFF;
  else return data->BUS_VAL_LOW;
}
unsigned char FT2232SPI_GetHighBitsLastState(FT2232SPI * data)
{
  if (!data) return 0xFF;
  else return data->BUS_VAL_HIGH;
}

unsigned char FT2232SPI_GetLowBitsState(FT2232SPI * data)
{
  unsigned char state = 0x00;

  if (!data) return 0xFF;

  if (busLow_readState(&data->ftdicContext,&state)) return 0xFF;

  return (data->BUS_VAL_LOW = state);
}
unsigned char FT2232SPI_GetHighBitsState(FT2232SPI * data)
{
  unsigned char state = 0x00;

  if (!data) return 0xFF;

  if (busHigh_readState(&data->ftdicContext,&state)) return 0xFF;

  return (data->BUS_VAL_HIGH = state);
}

void FT2232SPI_SetLowBitsDirection(FT2232SPI * data, unsigned char direction)
{
  if (!data) return;

  data->BUS_DIR_LOW &= 0x0F; //reseta direção dos 4 MSBs
  data->BUS_DIR_LOW |= (direction & 0xF0); //seta direção dos 4 MSBs, preserva direção dos 4 LSBs

  //seta direção
  busLow_setStateDir(&data->ftdicContext, data->BUS_VAL_LOW, data->BUS_DIR_LOW);
}

void FT2232SPI_SetHighBitsDirection(FT2232SPI * data, unsigned char direction)
{
  if (!data) return;

  data->BUS_DIR_HIGH = direction; //seta direções

  //seta direção
  busHigh_setStateDir(&data->ftdicContext, data->BUS_VAL_HIGH, data->BUS_DIR_HIGH);


}

void FT2232SPI_CYCLE(FT2232SPI * data)
{
  unsigned char HighPinsState = 0x00, LowPinsState = 0x00;
  int i = 0;

  if (!data) return;

  //verifica se o modo de operação é o correto
  if (data->OP_MODE != FT2232SPI_OPMODE_CYCLE) return;
  
  //se está parado ou transmitindo dados, não executa o ciclo
  if (data->OP_STATUS != FT2232SPI_OP_OK) return;
  
  //operação -> ciclo
  data->OP_STATUS = FT2232SPI_OP_CYCLE;

  //armazena ultimo valor dos pinos

  HighPinsState = data->BUS_VAL_HIGH;
  LowPinsState = data->BUS_VAL_LOW;

  //realiza update
  FT2232SPI_GetHighBitsState(data);
  FT2232SPI_GetLowBitsState(data);

  //verifica interrupções

  if (data->EnableInterrupts)
    {

    if (data->InterruptMaskHigh) //há interrupções habilitadas
      {

      for (i=0;i < 8; i++)
        {

        if ((data->InterruptMaskHigh & (1<<i)) && !(data->BUS_DIR_HIGH & (1<<i))) //este bit está com interrupção habilitada e é uma entrada
          {

          if (data->InterruptTypeHigh & (1<<i)) //a interrupção é do tipo nível
            {

            if (data->InterruptValueHigh & (1<<i)) //a interrupção é do tipo nível alto
              {

              if (data->BUS_VAL_HIGH & (1<<i))
                {
                
                data->OP_STATUS = FT2232SPI_OP_INT;
                (data->InterruptHandler)(data,FT2232SPI_INT_LEVEL_HIGH, (1<<(i+8)));//ocorreu interrupção
                
                }

              }
            else //a interrupção é do tipo nível baixo
              {

              if (!(data->BUS_VAL_HIGH & (1<<i))) 
                
                {
                
                data->OP_STATUS = FT2232SPI_OP_INT;
                (data->InterruptHandler)(data,FT2232SPI_INT_LEVEL_LOW,(1<<(i+8))); //ocorreu interrupção

                }
              
              }


            }
          else //a interrupção é do tipo borda
            {

            if (data->InterruptValueHigh & (1<<i)) //a interrupção é tipo borda de subida
              {

              if (!(HighPinsState & (1<<i)) && (data->BUS_VAL_HIGH & (1<<i)))
              
                {
                
                data->OP_STATUS = FT2232SPI_OP_INT;
                (data->InterruptHandler)(data,FT2232SPI_INT_EDGE_RISE,(1<<(i+8))); //ocorreu interrupção
                
                }

              }
            else //a interrupção é tipo borda de descida
              {

              if ((HighPinsState & (1<<i)) && !(data->BUS_VAL_HIGH & (1<<i)))
              
                {
                
                data->OP_STATUS = FT2232SPI_OP_INT;
                (data->InterruptHandler)(data,FT2232SPI_INT_EDGE_FALL,(1<<(i+8))); //ocorreu interrupção
                
                }

              }

            }

          }

        }

      }

    if (data->InterruptMaskLow)
      {

      //mesma coisa!
      for (i=0;i < 8; i++)
        {

        if ((data->InterruptMaskLow & (1<<i)) && !(data->BUS_DIR_LOW & (1<<i))) //este bit está com interrupção habilitada e é uma entrada
          {

          if (data->InterruptTypeLow & (1<<i)) //a interrupção é do tipo nível
            {

            if (data->InterruptValueLow & (1<<i)) //a interrupção é do tipo nível alto
              {

              if (data->BUS_VAL_LOW & (1<<i))
                {
                
                data->OP_STATUS = FT2232SPI_OP_INT;
                (data->InterruptHandler)(data,FT2232SPI_INT_LEVEL_HIGH, (1<<i));//ocorreu interrupção
                
                }


              }
            else //a interrupção é do tipo nível baixo
              {

              if (!(data->BUS_VAL_LOW & (1<<i)))
              
                {
                
                data->OP_STATUS = FT2232SPI_OP_INT;
                (data->InterruptHandler)(data,FT2232SPI_INT_LEVEL_LOW,(1<<i)); //ocorreu interrupção
                
                }
                

              }


            }
          else //a interrupção é do tipo borda
            {

            if (data->InterruptValueLow & (1<<i)) //a interrupção é tipo borda de subida
              {

              if (!(LowPinsState & (1<<i)) && (data->BUS_VAL_LOW & (1<<i)))
              
                {
                    
                data->OP_STATUS = FT2232SPI_OP_INT;
                (data->InterruptHandler)(data,FT2232SPI_INT_EDGE_RISE,(1<<i)); //ocorreu interrupção
                
                }


              }
            else //a interrupção é tipo borda de descida
              {

              if ((LowPinsState & (1<<i)) && !(data->BUS_VAL_LOW & (1<<i)))
              
                {
                
                data->OP_STATUS = FT2232SPI_OP_INT;
                (data->InterruptHandler)(data,FT2232SPI_INT_EDGE_FALL,(1<<i)); //ocorreu interrupção
                
                }

              }

            }

          }

        }

      }

    }
    
  //fim do ciclo
  data->OP_STATUS = FT2232SPI_OP_OK;

}

//habilita interrupções
void FT2232SPI_EnableInterrupts(FT2232SPI * data)
{

  if (!data) return;

  //verifica se a função de tratamento de interrupções é valida e habilita as interrupções
  if (!data->InterruptHandler) data->EnableInterrupts = 0x00;
  else data->EnableInterrupts = 0x01;


}

//desabilita interrupções
void FT2232SPI_DisableInterrupts(FT2232SPI * data)
{

  if (!data) return;

  //desabilita interrupções
  data->EnableInterrupts = 0x00;

}

//configura interrupções
void FT2232SPI_ConfigInterruptsLow(FT2232SPI * data, unsigned char interruptMask, unsigned char interruptValues,
                                    unsigned char interruptTypes)
{
  
  if (!data) return;
  
  data->InterruptMaskLow = interruptMask;
  data->InterruptValueLow = interruptValues;
  data->InterruptTypeLow = interruptTypes;
  
}

void FT2232SPI_ConfigInterruptsHigh(FT2232SPI * data, unsigned char interruptMask, unsigned char interruptValues,
                                    unsigned char interruptTypes)
{
  
  if (!data) return;
  
  data->InterruptMaskHigh = interruptMask;
  data->InterruptValueHigh = interruptValues;
  data->InterruptTypeHigh = interruptTypes;
  
}
