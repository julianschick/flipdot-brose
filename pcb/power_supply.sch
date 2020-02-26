EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 2 3
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
L power:GND #PWR0103
U 1 1 5DBB0D00
P 2800 5500
F 0 "#PWR0103" H 2800 5250 50  0001 C CNN
F 1 "GND" H 2805 5327 50  0000 C CNN
F 2 "" H 2800 5500 50  0001 C CNN
F 3 "" H 2800 5500 50  0001 C CNN
	1    2800 5500
	1    0    0    -1  
$EndComp
$Comp
L power:+24V #PWR0104
U 1 1 5DBB10B9
P 4700 4500
F 0 "#PWR0104" H 4700 4350 50  0001 C CNN
F 1 "+24V" H 4715 4673 50  0000 C CNN
F 2 "" H 4700 4500 50  0001 C CNN
F 3 "" H 4700 4500 50  0001 C CNN
	1    4700 4500
	1    0    0    -1  
$EndComp
$Comp
L Device:Fuse F1
U 1 1 5DBB315A
P 2950 4600
F 0 "F1" V 2753 4600 50  0000 C CNN
F 1 "Fuse 500mA" V 2844 4600 50  0000 C CNN
F 2 "Fuse_Holders_and_Fuses:Fuseholder_Fuse_TR5_Littlefuse-No560_No460" V 2880 4600 50  0001 C CNN
F 3 "~" H 2950 4600 50  0001 C CNN
	1    2950 4600
	0    1    1    0   
$EndComp
$Comp
L Device:D D1
U 1 1 5DBB4354
P 3450 4600
F 0 "D1" H 3450 4384 50  0000 C CNN
F 1 "D" H 3450 4475 50  0000 C CNN
F 2 "Diode_THT:D_A-405_P7.62mm_Horizontal" H 3450 4600 50  0001 C CNN
F 3 "~" H 3450 4600 50  0001 C CNN
	1    3450 4600
	-1   0    0    1   
$EndComp
Wire Wire Line
	3100 4600 3300 4600
$Comp
L power:+3.3V #PWR0105
U 1 1 5DBBF084
P 4800 3400
F 0 "#PWR0105" H 4800 3250 50  0001 C CNN
F 1 "+3.3V" H 4815 3573 50  0000 C CNN
F 2 "" H 4800 3400 50  0001 C CNN
F 3 "" H 4800 3400 50  0001 C CNN
	1    4800 3400
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR0106
U 1 1 5DBBF863
P 4700 2250
F 0 "#PWR0106" H 4700 2100 50  0001 C CNN
F 1 "+5V" H 4715 2423 50  0000 C CNN
F 2 "" H 4700 2250 50  0001 C CNN
F 3 "" H 4700 2250 50  0001 C CNN
	1    4700 2250
	1    0    0    -1  
$EndComp
$Comp
L Device:Jumper JP3
U 1 1 5DE8B703
P 4200 5000
F 0 "JP3" H 4200 5264 50  0000 C CNN
F 1 "Jumper" H 4200 5173 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" H 4200 5000 50  0001 C CNN
F 3 "~" H 4200 5000 50  0001 C CNN
	1    4200 5000
	1    0    0    -1  
$EndComp
$Comp
L power:VS #PWR0108
U 1 1 5DE94EA7
P 4700 4900
F 0 "#PWR0108" H 4500 4750 50  0001 C CNN
F 1 "VS" H 4717 5073 50  0000 C CNN
F 2 "" H 4700 4900 50  0001 C CNN
F 3 "" H 4700 4900 50  0001 C CNN
	1    4700 4900
	1    0    0    -1  
$EndComp
$Comp
L Device:Jumper JP4
U 1 1 5DE95646
P 5300 3500
F 0 "JP4" H 5300 3764 50  0000 C CNN
F 1 "Jumper" H 5300 3673 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" H 5300 3500 50  0001 C CNN
F 3 "~" H 5300 3500 50  0001 C CNN
	1    5300 3500
	1    0    0    -1  
$EndComp
Text GLabel 5800 3500 2    50   Output ~ 0
VCC-ESP32
$Comp
L Device:Jumper JP5
U 1 1 5DE97A3E
P 4200 2750
F 0 "JP5" H 4200 3014 50  0000 C CNN
F 1 "Jumper" H 4200 2923 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" H 4200 2750 50  0001 C CNN
F 3 "~" H 4200 2750 50  0001 C CNN
	1    4200 2750
	1    0    0    -1  
$EndComp
Text GLabel 4700 2750 2    50   Output ~ 0
VCC-FP2800
Wire Wire Line
	3700 2350 3900 2350
$Comp
L Device:Jumper JP1
U 1 1 5DF0BCCA
P 4200 4600
F 0 "JP1" H 4200 4864 50  0000 C CNN
F 1 "Jumper" H 4200 4773 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" H 4200 4600 50  0001 C CNN
F 3 "~" H 4200 4600 50  0001 C CNN
	1    4200 4600
	1    0    0    -1  
$EndComp
Wire Wire Line
	4500 2350 4700 2350
Wire Wire Line
	4700 2350 4700 2250
$Comp
L Device:Jumper JP2
U 1 1 5DF0EAAC
P 4200 2350
F 0 "JP2" H 4200 2614 50  0000 C CNN
F 1 "Jumper" H 4200 2523 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" H 4200 2350 50  0001 C CNN
F 3 "~" H 4200 2350 50  0001 C CNN
	1    4200 2350
	1    0    0    -1  
$EndComp
Wire Wire Line
	4500 4600 4700 4600
Wire Wire Line
	4700 4600 4700 4500
Wire Wire Line
	2650 5500 2800 5500
Text GLabel 4700 1450 2    50   Output ~ 0
VCC-LED
$Comp
L Device:Fuse F3
U 1 1 5DCE13DC
P 2900 1450
F 0 "F3" V 2703 1450 50  0000 C CNN
F 1 "Fuse 5A" V 2794 1450 50  0000 C CNN
F 2 "Fuse_Holders_and_Fuses:Fuseholder_Fuse_TR5_Littlefuse-No560_No460" V 2830 1450 50  0001 C CNN
F 3 "~" H 2900 1450 50  0001 C CNN
	1    2900 1450
	0    1    1    0   
$EndComp
$Comp
L Device:D D2
U 1 1 5DCE676E
P 3400 1450
F 0 "D2" H 3400 1234 50  0000 C CNN
F 1 "D" H 3400 1325 50  0000 C CNN
F 2 "Diode_THT:D_P600_R-6_P12.70mm_Horizontal" H 3400 1450 50  0001 C CNN
F 3 "~" H 3400 1450 50  0001 C CNN
	1    3400 1450
	-1   0    0    1   
$EndComp
Wire Wire Line
	3050 1450 3250 1450
Wire Wire Line
	2800 4600 2550 4600
Wire Wire Line
	2750 1450 2550 1450
Wire Wire Line
	3550 1450 3700 1450
Connection ~ 3700 1450
Wire Wire Line
	3700 1450 4700 1450
Wire Wire Line
	5600 3500 5800 3500
Wire Wire Line
	5000 3500 4800 3500
Wire Wire Line
	3700 2750 3900 2750
Wire Wire Line
	4500 2750 4700 2750
Connection ~ 3700 2350
Wire Wire Line
	3700 2350 3700 2750
Wire Wire Line
	3700 1450 3700 1650
$Comp
L Device:Fuse F2
U 1 1 5DF1C197
P 3700 1800
F 0 "F2" H 3760 1846 50  0000 L CNN
F 1 "Fuse 1A" H 3760 1755 50  0000 L CNN
F 2 "Fuse_Holders_and_Fuses:Fuseholder_Fuse_TR5_Littlefuse-No560_No460" V 3630 1800 50  0001 C CNN
F 3 "~" H 3700 1800 50  0001 C CNN
	1    3700 1800
	1    0    0    -1  
$EndComp
Wire Wire Line
	3700 1950 3700 2350
$Comp
L Regulator_Linear:LD1117S33TR_SOT223 U2
U 1 1 5DF22514
P 4250 3500
F 0 "U2" H 4250 3742 50  0000 C CNN
F 1 "LD1117S33TR_SOT223" H 4250 3651 50  0000 C CNN
F 2 "TO_SOT_Packages_THT:TO-220-3_Horizontal" H 4250 3700 50  0001 C CNN
F 3 "http://www.st.com/st-web-ui/static/active/en/resource/technical/document/datasheet/CD00000544.pdf" H 4350 3250 50  0001 C CNN
	1    4250 3500
	1    0    0    -1  
$EndComp
Wire Wire Line
	3950 3500 3700 3500
Wire Wire Line
	3700 3500 3700 2750
Connection ~ 3700 2750
Wire Wire Line
	4550 3500 4800 3500
$Comp
L Device:C C1
U 1 1 5DF2C29E
P 3700 3700
F 0 "C1" H 3815 3746 50  0000 L CNN
F 1 "100n" H 3815 3655 50  0000 L CNN
F 2 "Capacitors_THT:C_Disc_D5.0mm_W2.5mm_P2.50mm" H 3738 3550 50  0001 C CNN
F 3 "~" H 3700 3700 50  0001 C CNN
	1    3700 3700
	1    0    0    -1  
$EndComp
Wire Wire Line
	3700 3850 3700 3900
Connection ~ 3700 3900
Wire Wire Line
	3700 3900 2650 3900
Wire Wire Line
	3700 3550 3700 3500
Connection ~ 3700 3500
Wire Wire Line
	4800 3550 4800 3500
Wire Wire Line
	4800 3850 4800 3900
Wire Wire Line
	3700 3900 4250 3900
Wire Wire Line
	4250 3800 4250 3900
Connection ~ 4250 3900
Wire Wire Line
	4250 3900 4800 3900
Connection ~ 4800 3500
Wire Wire Line
	3600 4600 3800 4600
Wire Wire Line
	3900 5000 3800 5000
Wire Wire Line
	3800 5000 3800 4600
Connection ~ 3800 4600
Wire Wire Line
	3800 4600 3900 4600
Wire Wire Line
	4500 5000 4700 5000
Wire Wire Line
	4700 5000 4700 4900
$Comp
L Device:CP C2
U 1 1 5DF89079
P 4800 3700
F 0 "C2" H 4918 3746 50  0000 L CNN
F 1 "10u" H 4918 3655 50  0000 L CNN
F 2 "Capacitors_THT:CP_Radial_D5.0mm_P2.50mm" H 4838 3550 50  0001 C CNN
F 3 "~" H 4800 3700 50  0001 C CNN
	1    4800 3700
	1    0    0    -1  
$EndComp
Wire Wire Line
	4800 3500 4800 3400
$Comp
L Connector_Generic:Conn_01x02 J3
U 1 1 5E16D297
P 2350 1450
F 0 "J3" H 2350 1100 50  0000 C CNN
F 1 "Conn_01x02" H 2350 1200 50  0000 C CNN
F 2 "TerminalBlock_MetzConnect:TerminalBlock_MetzConnect_Type055_RT01502HDWU_Pitch5.00mm" H 2350 1450 50  0001 C CNN
F 3 "~" H 2350 1450 50  0001 C CNN
	1    2350 1450
	-1   0    0    1   
$EndComp
$Comp
L Connector_Generic:Conn_01x02 J4
U 1 1 5E171364
P 2350 4600
F 0 "J4" H 2350 4250 50  0000 C CNN
F 1 "Conn_01x02" H 2350 4350 50  0000 C CNN
F 2 "TerminalBlock_MetzConnect:TerminalBlock_MetzConnect_Type055_RT01502HDWU_Pitch5.00mm" H 2350 4600 50  0001 C CNN
F 3 "~" H 2350 4600 50  0001 C CNN
	1    2350 4600
	-1   0    0    1   
$EndComp
Wire Wire Line
	2650 3900 2650 4500
Wire Wire Line
	2550 4500 2650 4500
Connection ~ 2650 4500
Wire Wire Line
	2650 4500 2650 5500
Wire Wire Line
	2650 3900 2650 1350
Wire Wire Line
	2650 1350 2550 1350
Connection ~ 2650 3900
$EndSCHEMATC
