# !/bin/bash

#build manual

#rm *.o
#rm *.py
#rm *.pyc
#rm *.so

swig2.0 -python ecog_spi.i

gcc -c -fpic ecog_spi_wrap.c -I/usr/include/python2.7

gcc -shared ../Debug/ecog_spi.o ecog_spi_wrap.o ../Debug/PGA280/pga280.o ../Debug/ft2232_spi.o ../Debug/ADS1259/ads1259.o ../Debug/ecog_spi_alerts.o -lftdi -o _EcoGSPI.so


