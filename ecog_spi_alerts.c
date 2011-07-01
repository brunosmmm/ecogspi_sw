/*
 * ecog_spi_alerts.c
 *
 *  Created on: 01/07/2011
 *      Author: bruno
 */
#include "ecog_spi_alerts.h"
#include <stdlib.h>

EcoGSPIAData * ECOGSPIALERT_NewAlert(unsigned char alertType, int alertParam)
{
  EcoGSPIAData * newAlert = NULL;

  newAlert = (EcoGSPIAData*)malloc(sizeof(EcoGSPIAData));

  if (!newAlert) return NULL;

  newAlert->alertType = alertType;
  newAlert->alertParam = alertParam;

  return newAlert;

}
void ECOGSPIALERT_FreeAlert(EcoGSPIAData * alert)
{
  if (!alert) return;

  free(alert);

}
