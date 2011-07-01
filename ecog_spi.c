#include "ecog_spi.h"
#include <pthread.h>
#include <string.h>
#include "ecog_spi_alerts.h"

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

  //inicialização FTDI2232
  EcoGSPIData.ft2232spi = FT2232SPI_INIT(FT2232SPI_OPMODE_CYCLE,0,FT2232SPI_CPHA1,TRUE,1200, FT2232_InterruptHandler);

  initStatus += FT2232SPI_HWINIT(EcoGSPIData.ft2232spi,FTDI_VID,FTDI_FT2232H_PID,INTERFACE_A);

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
  ADS1259_SetSampleRate(EcoGSPIData.ads1259, ADS1259_RATE_60SPS);

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

  return 0;

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

    }

  //desabilita interrupções do ft2232spi
  FT2232SPI_DisableInterrupts(EcoGSPIData.ft2232spi);
  
  //para conversões A/D
  FT2232SPI_SetLowBitsState(EcoGSPIData.ft2232spi, ECOGSPI_PIN_ADRST);

  //finaliza
  InternalData.cycleHandlerRunning = 0;

  pthread_exit(NULL);

  return NULL;

    }

static void * ECOGSPI_AlertHandler(void * arg)
{
  EcoGSPIAData * alert = NULL;

  if (!arg) return NULL; //alerta inválido

  //converte argumento para alerta
  alert = (EcoGSPIAData *)arg;

  //trata alerta
  switch (alert->alertType)
  {

    case ECOGSPI_ALERT_DATA_AVAILABLE:
      //dados estão disponíveis

      //chama função responsável
      (EcoGSPIData.dataAvailableAlert)(alert->alertParam);
      break;

    default:
      break; //alerta inválido?
  }

  //termina
  pthread_exit(NULL);

  return NULL;

}

//ciclo da interface ECOGSPI

void ECOGSPI_Cycle(void)
{
  unsigned char dataAv = 0;
  //ciclo do ft2232h
  FT2232SPI_CYCLE(EcoGSPIData.ft2232spi);

  //verifica se alertas serão disparados
  if (ECOGSPI_AlertsEnabled())
    {

    //alertas estão habilitados; verifica

    if ((dataAv = ECOGSPI_DataAvailable()))
      {

      //dados disponíveis -> alerta:
      //o alerta é dado em uma nova thread que é criada, por que não podemos parar de pegar amostras que chegam constantemente

      pthread_create(&InternalData.alertHandler, NULL, ECOGSPI_AlertHandler,
                      (void*)ECOGSPIALERT_NewAlert(ECOGSPI_ALERT_DATA_AVAILABLE,dataAv));

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
    if (EcoGSPIData.inBufPtr - EcoGSPIData.inBuf > 255) EcoGSPIData.inBufPtr = EcoGSPIData.inBuf;

    FT2232_ADS1259_WriteReadData(NULL,0,EcoGSPIData.inBufPtr,3);

    EcoGSPIData.inBufPtr += 3;

    EcoGSPIData.dataAvailable += 3;

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

void ECOGSPI_ReadBuffer(unsigned char * dest,unsigned char offset, unsigned char count)
{
  pthread_mutex_t mutex_buffer = PTHREAD_MUTEX_INITIALIZER;

  pthread_mutex_lock(&mutex_buffer);

  memcpy(dest,EcoGSPIData.inBuf + offset, count);

  pthread_mutex_unlock(&mutex_buffer);

}

unsigned char ECOGSPI_ReadBufferByte(unsigned char offset)
{
  pthread_mutex_t mutex_buffer = PTHREAD_MUTEX_INITIALIZER;
  unsigned char byte = 0;

  pthread_mutex_lock(&mutex_buffer);
  byte = EcoGSPIData.inBuf[offset];
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
