/* Interface SWIG  EcoGSPI */
%module EcoGSPI

%{
#include "../ecog_spi.h"
%}

/* isso é realmente necessário? */

int ECOGSPI_Init(unsigned char useChecksum, void (* dataAvailableAlert)(unsigned char)); //inicialização da interface
int ECOGSPI_HwConfig(void); //configuração inicial do hardware
void ECOGSPI_Cycle(void); //Ciclo de execução
int ECOGSPI_StartHandling(void); //Inicia tratamento de ciclos
void ECOGSPI_StopHandling(void); //para tratamento de ciclos (para ciclos)
unsigned char ECOGSPI_DataAvailable(void); //retorna quantos pacotes de dados estão disponíveis
void ECOGSPI_ReadBuffer(unsigned char * dest,unsigned char offset, unsigned char count); //pega dados do buffer
unsigned char ECOGSPI_ReadBufferByte(unsigned char offset); //pega um byte do buffer

void ECOGSPI_EnableAlerts(void); //habilita alertas
void ECOGSPI_DisableAlerts(void); //desabilita alertas

unsigned char ECOGSPI_AlertsEnabled(void); //retorna se alertas estão habilitados ou não
