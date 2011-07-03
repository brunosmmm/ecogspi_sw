/*
 * ecog_spi_alerts.h
 *
 *  Created on: 01/07/2011
 *      Author: bruno
 */

#ifndef ECOG_SPI_ALERTS_H_
#define ECOG_SPI_ALERTS_H_

//tipos de alerta
#define ECOGSPI_ALERT_DATA_AVAILABLE 0x01 //alerta de dados novos disponíveis: param = localização dos dados no buffer (offset)
#define ECOGSPI_ALERT_BUFFER_FULL    0x02 //alerta de buffer de dados cheio: param = tamanho do buffer

//funções de callback de alertas
typedef void (* alertCallback)(int);

typedef struct EcoGSPIAlertData
{

  unsigned char alertType;
  int alertParam;

} EcoGSPIAData;

EcoGSPIAData * ECOGSPIALERT_NewAlert(unsigned char alertType, int alertParam);
void ECOGSPIALERT_FreeAlert(EcoGSPIAData * alert);


#endif /* ECOG_SPI_ALERTS_H_ */
