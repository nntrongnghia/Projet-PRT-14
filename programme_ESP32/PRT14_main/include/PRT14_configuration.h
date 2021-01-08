#ifndef __PRT14_CONFIGURATION_H__
#define __PRT14_CONFIGURATION_H__

// CHANGE USER, MP AND IP ADDRESS

// Experimental result: if SPI clock = 3.4MHz , then the true sampling rate ~= 68 ksps
// which means 34 ksps for voltage and current each, then we should have ~680 samples per period of a 50Hz signal
// To avoid overflow, multiply the number of period with 800
#undef  AC_DEBUG    // deactive AutoConnect debug via Serial
#define PRINT_POWER // debug purpose, print out the power value, comment out this line if not use
#define VREF 5.0                 // la tension de référence pour le CAN MCP3004
#define ADC_CS_PIN 5             // CS line for MCP3004
#define PCNT_INPUT_SIG_IO 15     // Pulse Input GPIO
#define NB_SAMPLE_PERIOD 20 // number of period to be sampled
#define SPI_CLOCK 3400000 // Hz
#define NB_SAMPLES NB_SAMPLE_PERIOD * SPI_CLOCK / 4250
#define OFFSET_VOLTAGE 494 // raw ADC value ~ 2.41V 
#define OFFSET_CURRENT 522 // raw ADC value ~ 2.54V D
#define ADC_GAIN 1024.0 / VREF
#define VOLTAGE_GAIN ADC_GAIN*5.43E-3
#define CURRENT_GAIN ADC_GAIN*0.056
#define VOLTAGE_CHANNEL 1 // channel du CAN extern
#define CURRENT_CHANNEL 0 // channel du CAN extern
#define MEASURE_PERIOD 30 //Intervalle en seconde de la mesure de la puissance [s]
#define USER "XXX" //User ID pour se connecter au BROKER MQTT
#define MP "XXX" //Mot de Passe pour se connecter au BROKER
#define IP_ADR_MQTT_BROKER "XXX.XXX.XXX.XXX" //Adr  IP du BROKER MQTT
#define TOPIC "PRT14/power" //topic de publication
#define TAILLE_MSG 8 //Taille en octet du msg envoyer au broker MQTT
#define LED_V 16 //Pin number - Indique que nous sommes connectés au broker
#define LED_R 4 //Pin number - Indique que nous ne sommes pas connectés à internet, broker
#define RETRY_INTERVAL 30000 // ms. Time between reconnection retries


#endif // __PRT14_CONFIGURATION_H__