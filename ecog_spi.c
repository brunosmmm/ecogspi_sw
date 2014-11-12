#include "ecog_spi.h"
#include <pthread.h>
#include <string.h>

//estrutura global contendo os dados
ECOGSPI EcoGSPIData;

typedef struct EcoGSPIInternalData
{
  pthread_t cycleHandler;
  pthread_t alertHandler;

  int cycleHandlerRunning;
  int cycleHandlerStop;
  long int cycleDelay;

  unsigned char cksum;


} EcoGSPIIData;

static EcoGSPIIData InternalData;

//protótipos de funções locais para comunicação
static void FT2232_PGA280_ReadWriteData(unsigned char * sendbuf , unsigned char buflen, unsigned char * readbuf, unsigned char readlen); //escreve e/ou lê um registrador do PGA280 via SPI
static void FT2232_ADS1259_WriteReadData(unsigned char * writeBuf, unsigned char writeLen, unsigned char * readBuf, unsigned char readLen); //envia e recebe dados do ADS1259 através do PGA280
static void * ECOGSPI_CycleHandler(void * arg);
static void * ECOGSPI_AlertHandler(void * arg);
static void FT2232_InterruptHandler(FT2232SPI * data, unsigned char InterruptType, unsigned short InterruptPin);

int ECOGSPI_Init(unsigned char useChecksum)
{
  int initStatus = 0x00;

  //inicialização de estruturas locais
  EcoGSPIData.inBufPtr = EcoGSPIData.inBuf;
  EcoGSPIData.dataAvailable = 0x00;
  EcoGSPIData.enableAlerts = 0x00;
  EcoGSPIData.newDataInBuffer = 0x00;

  EcoGSPIData.dataAvailableAlert = NULL;
  EcoGSPIData.bufferFullAlert = NULL;

  //inicialização FTDI2232 - clock de 10 MHz
  EcoGSPIData.ft2232spi = FT2232SPI_INIT(FT2232SPI_OPMODE_CYCLE,0,FT2232SPI_CPHA1,FALSE,6, FT2232_InterruptHandler);

  initStatus += FT2232SPI_HWINIT(EcoGSPIData.ft2232spi,FTDI_VID,FTDI_FT2232H_PID,INTERFACE_A,ECOGSPI_FT2232_CHUNK);

  //direção dos pinos do FT2232H
  //saída: ADRST, UCRST, START
  //entrada: DRDY
  FT2232SPI_SetLowBitsDirection(EcoGSPIData.ft2232spi, ECOGSPI_PIN_ADRST | ECOGSPI_PIN_UCRST | ECOGSPI_PIN_START);

  //inicialização PGA280
  EcoGSPIData.pga280 = PGA280_INIT(FT2232_PGA280_ReadWriteData);

  //inicialização ADS1259
  EcoGSPIData.ads1259 = ADS1259_INIT(FT2232_ADS1259_WriteReadData);

  //usa checksum?
  InternalData.cksum = useChecksum;

  return initStatus;
}

//configura o hardware do ECOGSPI
int ECOGSPI_HwConfig(void)
{
  
  //configura PGA280

  PGA280_WriteGPIOState(EcoGSPIData.pga280,0x01); //GPIO0 alto

  PGA280_WriteGPIODirection(EcoGSPIData.pga280,0x01); //GPIO0 como saída

  PGA280_SetECSMode(EcoGSPIData.pga280,0x01); //GPIO0 em modo ECS
  
  //levanta ADRST, iniciando operação do ADC
  
  FT2232SPI_SetLowBitsState(EcoGSPIData.ft2232spi, ECOGSPI_PIN_ADRST);
  
  //espera para garantir que o ADC esteja pronto
  usleep(10000);
  
  //configuração ADS1259
  
  //comando SDATAC - é necessário para sair do modo RDATAC

  ADS1259_StopContinuous(EcoGSPIData.ads1259);

  //habilita syncout

  ADS1259_EnableSyncOut(EcoGSPIData.ads1259);
  
  //configura pga para utilizar syncin

  PGA280_EnableSyncIn(EcoGSPIData.pga280);
  
  //configura taxa de amostragem do ADS1259
  ADS1259_SetSampleRate(EcoGSPIData.ads1259, ADS1259_RATE_1200SPS);

  //configura interrupções no FT2232SPI
  //interrupção configurada: pino !DRDY -> borda de descida
  FT2232SPI_ConfigInterruptsLow(EcoGSPIData.ft2232spi,ECOGSPI_PIN_DRDY,0x00,0x00);
  
  //coloca o ADS1259 no modo RDATAC
  ADS1259_StartContinuous(EcoGSPIData.ads1259);
  
  /**Temporário**/
  //seleciona canal 1

  PGA280_SelectChannel(EcoGSPIData.pga280,PGA280_CHAN_1);

  //seta ganho

  PGA280_SetGain(EcoGSPIData.pga280,PGA280_GAIN_128);

  //limpa buffer
  ftdi_usb_purge_rx_buffer(&EcoGSPIData.ft2232spi->ftdicContext);

  return 0;

}

void ECOGSPI_SetDataAvailableAlert(alertCallback func)
{
  pthread_mutex_t mutex_handler = PTHREAD_MUTEX_INITIALIZER;

  pthread_mutex_lock(&mutex_handler);
  EcoGSPIData.dataAvailableAlert = func;
  pthread_mutex_unlock(&mutex_handler);

}

void ECOGSPI_SetBufferFullAlert(alertCallback func)
{
  pthread_mutex_t mutex_handler = PTHREAD_MUTEX_INITIALIZER;

  pthread_mutex_lock(&mutex_handler);
  EcoGSPIData.bufferFullAlert = func;
  pthread_mutex_unlock(&mutex_handler);

}
  

int ECOGSPI_StartHandling(void)
{
  //usa threading para criar um tratador de ciclos independente do programa principal
  return pthread_create(&InternalData.cycleHandler, NULL, ECOGSPI_CycleHandler, NULL);
}

void ECOGSPI_StopHandling(void)
{
  pthread_mutex_t mutex_handler = PTHREAD_MUTEX_INITIALIZER;

  //Flaga a parada da thread
  pthread_mutex_lock(&mutex_handler);
  InternalData.cycleHandlerStop = 1;
  pthread_mutex_unlock(&mutex_handler);

  //espera término
  if (InternalData.cycleHandlerStop) {
    pthread_join(InternalData.cycleHandler, NULL);
    pthread_join(InternalData.alertHandler, NULL);
  }

}


static void * ECOGSPI_CycleHandler(void * arg)
    {

  pthread_mutex_t mutex_handler = PTHREAD_MUTEX_INITIALIZER;

  //inicialização

  //habilita interrupções do ft2232spi
  FT2232SPI_EnableInterrupts(EcoGSPIData.ft2232spi);
  
  //inicia conversões A/D
  FT2232SPI_SetLowBitsState(EcoGSPIData.ft2232spi, ECOGSPI_PIN_ADRST | ECOGSPI_PIN_START);

  //rodando
  InternalData.cycleHandlerRunning = 1;

  //previne stop antes mesmo de iniciar
  pthread_mutex_lock(&mutex_handler);
  InternalData.cycleHandlerStop = 0;
  pthread_mutex_unlock(&mutex_handler);


  //Executa ciclos
  while (!InternalData.cycleHandlerStop)
    {

    ECOGSPI_Cycle();

    //delay?

    /**TODO: usar delay adequado que não paralize toda a aplicação*/
    //usleep(10);

    }

  //desabilita interrupções do ft2232spi
  FT2232SPI_DisableInterrupts(EcoGSPIData.ft2232spi);
  
  //para conversões A/D
  FT2232SPI_SetLowBitsState(EcoGSPIData.ft2232spi, 0x00/*ECOGSPI_PIN_ADRST*/);

  //finaliza
  InternalData.cycleHandlerRunning = 0;

  pthread_exit(NULL);

  return NULL;

    }

static void * ECOGSPI_AlertHandler(void * arg)
{
  EcoGSPIAData * alert = NULL;
  pthread_mutex_t mutex_alert = PTHREAD_MUTEX_INITIALIZER;

  if (!arg) return NULL; //alerta inválido

  //converte argumento para alerta
  alert = (EcoGSPIAData *)arg;

  //trata alerta
  switch (alert->alertType)
  {

    case ECOGSPI_ALERT_DATA_AVAILABLE:
      //dados estão disponíveis

      //chama função responsável
      if (EcoGSPIData.dataAvailableAlert) (EcoGSPIData.dataAvailableAlert)(alert->alertParam);
      break;

    case ECOGSPI_ALERT_BUFFER_FULL:

      if (EcoGSPIData.bufferFullAlert) (EcoGSPIData.bufferFullAlert)(alert->alertParam);
      break;

    default:
      break; //alerta inválido?
  }

  //termina

  ECOGSPIALERT_FreeAlert(alert);

  //novos dados já foram reportados
  pthread_mutex_lock(&mutex_alert);
  EcoGSPIData.newDataInBuffer = 0x00;
  pthread_mutex_unlock(&mutex_alert);

  //pthread_exit(NULL);

  return NULL;

}

//ciclo da interface ECOGSPI

void ECOGSPI_Cycle(void)
{
  unsigned char dataAv = 0;
  int dataOffset = 0;
  pthread_mutex_t mutex_databuf = PTHREAD_MUTEX_INITIALIZER;

  unsigned char buffer[ECOGSPI_FT2232_CHUNK];

  //ciclo do ft2232h
  FT2232SPI_CYCLE(EcoGSPIData.ft2232spi);

  //if (!FT2232SPI_ReceiveImmediatly(EcoGSPIData.ft2232spi,ECOGSPI_FT2232_CHUNK,buffer))
  //  printf("dados\n");

  //verifica se alertas serão disparados
  if (ECOGSPI_AlertsEnabled())
    {

    //alertas estão habilitados; verifica
    pthread_mutex_lock(&mutex_databuf);
    dataAv = EcoGSPIData.newDataInBuffer;
    dataOffset = EcoGSPIData.inBufPtr - EcoGSPIData.inBuf - 3;
    pthread_mutex_unlock(&mutex_databuf);

    if (dataAv)
      {

      //dados disponíveis -> alerta:
      //o alerta é dado em uma nova thread que é criada, por que não podemos parar de pegar amostras que chegam constantemente

      //copia dados para buffer da aplicação
      memcpy(EcoGSPIData.appBuffer,EcoGSPIData.inBuf,ECOGSPI_BUF_SIZE);

      pthread_create(&InternalData.alertHandler, NULL, ECOGSPI_AlertHandler,
                      (void*)ECOGSPIALERT_NewAlert(ECOGSPI_ALERT_DATA_AVAILABLE,dataOffset));

      pthread_join(InternalData.alertHandler, NULL);
      //ECOGSPI_AlertHandler((void*)ECOGSPIALERT_NewAlert(ECOGSPI_ALERT_DATA_AVAILABLE,dataOffset));

      }

    }

}

static void FT2232_PGA280_ReadWriteData(unsigned char * sendbuf , unsigned char buflen, unsigned char * readbuf, unsigned char readlen)
{
  //lê dados: primeiro escreve o comando de leitura e depois recebe os dados
  FT2232SPI_SendRecvData(EcoGSPIData.ft2232spi, buflen, readlen, sendbuf, readbuf,FT2232SPI_RW_ASSERTCS);

}


static void FT2232_ADS1259_WriteReadData(unsigned char * writeBuf, unsigned char writeLen, unsigned char * readBuf, unsigned char readLen)
{
  unsigned char  myBuf[writeLen + 1];

  myBuf[0] = 0xC0; //comando para utilizar o modo ECS (GPIO0) do PGA280 para enviar dados

  memcpy(&myBuf[1],writeBuf,writeLen); //copia buffer de escrita para variável local

  //escreve e lê
  FT2232SPI_SendRecvData(EcoGSPIData.ft2232spi, writeLen+1, readLen, myBuf, readBuf, FT2232SPI_RW_ASSERTCS);


}

static void FT2232_InterruptHandler(FT2232SPI * data, unsigned char InterruptType, unsigned short InterruptPin)
{
  pthread_mutex_t mutex_handler = PTHREAD_MUTEX_INITIALIZER;
  //atenção: usar mutexes em todas as variáveis compartilhadas

  if (InterruptPin == ECOGSPI_PIN_DRDY)
    {

    //lê dados
    //printf("Interrupt!! DRDY; %x\n",InterruptType);

    //buffer circular
    pthread_mutex_lock(&mutex_handler);
    if (EcoGSPIData.inBufPtr - EcoGSPIData.inBuf > ECOGSPI_BUF_SIZE-1)
      {
      EcoGSPIData.inBufPtr = EcoGSPIData.inBuf;

      pthread_create(&InternalData.alertHandler, NULL, ECOGSPI_AlertHandler,
                      (void*)ECOGSPIALERT_NewAlert(ECOGSPI_ALERT_BUFFER_FULL,ECOGSPI_BUF_SIZE));

      }

    FT2232_ADS1259_WriteReadData(NULL,0,EcoGSPIData.inBufPtr,3);

    if (FT2232SPI_BytesToReceive(data) > ECOGSPI_FT2232_CHUNK-1)
      {
      //printf("bytes_to_rec: %d\n",FT2232SPI_BytesToReceive(EcoGSPIData.ft2232spi));
      //já temos um chunk para receber, é hora de receber??
      FT2232SPI_ReceiveImmediatly(data, ECOGSPI_FT2232_CHUNK, EcoGSPIData.inBufPtr);
      EcoGSPIData.inBufPtr += ECOGSPI_FT2232_CHUNK;

      if (EcoGSPIData.dataAvailable >= ECOGSPI_BUF_SIZE) EcoGSPIData.dataAvailable = ECOGSPI_BUF_SIZE;
      else EcoGSPIData.dataAvailable += ECOGSPI_FT2232_CHUNK;

      EcoGSPIData.newDataInBuffer = 0x01;

      }

    //EcoGSPIData.inBufPtr += 3;

    //if (EcoGSPIData.dataAvailable >= ECOGSPI_BUF_SIZE) EcoGSPIData.dataAvailable = ECOGSPI_BUF_SIZE;
    //else EcoGSPIData.dataAvailable += 3;

    //EcoGSPIData.newDataInBuffer = 0x01;

    pthread_mutex_unlock(&mutex_handler);

    }

}

//verifica se há dados disponíveis
unsigned char ECOGSPI_DataAvailable(void)
{
  pthread_mutex_t mutex_dataAv = PTHREAD_MUTEX_INITIALIZER;
  unsigned char dataAv = 0;

  //essa paranóia toda é realmente necessária?

  pthread_mutex_lock(&mutex_dataAv);
  dataAv = EcoGSPIData.dataAvailable;
  pthread_mutex_unlock(&mutex_dataAv);

  return dataAv;

}

void ECOGSPI_ReadBuffer(unsigned char * dest,unsigned char offset, unsigned int count)
{
  pthread_mutex_t mutex_buffer = PTHREAD_MUTEX_INITIALIZER;

  pthread_mutex_lock(&mutex_buffer);

  //memcpy(dest,EcoGSPIData.inBuf + offset, count);
  memcpy(dest,EcoGSPIData.appBuffer + offset, count);

  pthread_mutex_unlock(&mutex_buffer);

}

unsigned char ECOGSPI_ReadBufferByte(unsigned char offset)
{
  pthread_mutex_t mutex_buffer = PTHREAD_MUTEX_INITIALIZER;
  unsigned char byte = 0;

  pthread_mutex_lock(&mutex_buffer);
  //byte = EcoGSPIData.inBuf[offset];
  byte = EcoGSPIData.appBuffer[offset];
  pthread_mutex_unlock(&mutex_buffer);

  return byte;

}

void ECOGSPI_EnableAlerts(void)
{
  pthread_mutex_t mutex_alert = PTHREAD_MUTEX_INITIALIZER;

  pthread_mutex_lock(&mutex_alert);
  EcoGSPIData.enableAlerts = 0x01;
  pthread_mutex_unlock(&mutex_alert);


}

void ECOGSPI_DisableAlerts(void)
{
  pthread_mutex_t mutex_alert = PTHREAD_MUTEX_INITIALIZER;

  pthread_mutex_lock(&mutex_alert);
  EcoGSPIData.enableAlerts = 0x00;
  pthread_mutex_unlock(&mutex_alert);


}

unsigned char ECOGSPI_AlertsEnabled(void)
{
  pthread_mutex_t mutex_alert = PTHREAD_MUTEX_INITIALIZER;
  unsigned char alerts = 0;

  pthread_mutex_lock(&mutex_alert);
  alerts = EcoGSPIData.enableAlerts;
  pthread_mutex_unlock(&mutex_alert);

  return alerts;

}
