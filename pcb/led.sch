EESchema Schematic File Version 4
LIBS:flipdot-brose-cache
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 4 4
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Connector_Generic:Conn_01x03 J?
U 1 1 5DDEFC38
P 9800 1550
F 0 "J?" H 9880 1592 50  0000 L CNN
F 1 "Conn_01x03" H 9880 1501 50  0000 L CNN
F 2 "" H 9800 1550 50  0001 C CNN
F 3 "~" H 9800 1550 50  0001 C CNN
	1    9800 1550
	1    0    0    -1  
$EndComp
Wire Wire Line
	9600 1450 9350 1450
Wire Wire Line
	9600 1550 8950 1550
Wire Wire Line
	9600 1650 8950 1650
$Comp
L power:GND #PWR?
U 1 1 5DDF14CE
P 8950 1650
F 0 "#PWR?" H 8950 1400 50  0001 C CNN
F 1 "GND" H 8955 1477 50  0000 C CNN
F 2 "" H 8950 1650 50  0001 C CNN
F 3 "" H 8950 1650 50  0001 C CNN
	1    8950 1650
	1    0    0    -1  
$EndComp
Text GLabel 8950 1350 0    50   Input ~ 0
VCC-LED
Wire Wire Line
	8950 1350 9350 1350
Wire Wire Line
	9350 1350 9350 1450
Text GLabel 8550 1550 2    50   Output ~ 0
DATA-LED
$EndSCHEMATC
