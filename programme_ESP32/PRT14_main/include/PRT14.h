/*
Cette librairie contient les fonctions de l'ESP32 du projet PRT 14.
*/

/**
 * CHAINE DE CALCUL
 * Attention aux types des valeurs !
 * Afin d'optimiser la mémoire, les valeurs restent le type int16_t (valeur bruite binaire)
 * La vrai unité ne sera calculée qu'à la fin de la chaîne de calcul
 * 
 * readAdc() en boucle -> offset() -> average_power()
 * 
 */

#ifndef __PRT14_H__
#define __PRT14_H__

#include <Arduino.h>
#include <ESP32Ping.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <math.h>
#include <SPI.h>
#include "driver/pcnt.h"
#include "driver/spi_master.h"
#include "PRT14_configuration.h"
#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>
#include <PubSubClient.h>


#define MY_PCNT_UNIT PCNT_UNIT_1 // we use the Pulse Counter 0

// return True if connected to the Internet
bool check_internet();

// start the Network Time Protocol client
void beginNTPClient();

// return time in seconds since Jan. 1, 1970
unsigned long getEpochTime();

/**
 * Calculer la moyenne de deux valeurs consécutives dans arrayIn et le stocker dans arrayOut
 * arrayIn et arrayOut ont la meme taille de array_length cases
 * La première valeur d'arrayOut sera 0 
 */
void average_bw_intervals(const int16_t *arrayIn, int16_t *arrayOut, int array_length);

/** 
 *  * Calculate the average power in raw binary value from arrays of voltage and current read from ADC
 * @param  {int16_t*} pvoltage : input array for voltage samples
 * @param  {int16_t*} pcurrent : input array for current samples
 * @param  {int} nb_samples    : number of samples read
 * @param  {float} voltage_gain : gain Vpp/adc_raw_value
 * @param  {float} current_gain : gain Ipp/adc_raw_value
 * @return {float}           : average power in Watt
 */
float average_power(int16_t *pvoltage, int16_t *pcurrent, int nb_samples, float voltage_gain, float current_gain);

/**
 * Add an offset value to a signal stored in arrayIn
 */
void set_offset(int16_t *arrayIn, uint16_t length, int16_t offset);

/**
 * SPI initialisation
 * @param  {uint8_t} cs_pin     : 
 * @param  {uint32_t} spi_clock : the frequency of the CLK line
 */
void init_SPI(uint32_t spi_clock = 3400000);

/**
 * Read value from ADC in Volt
 * @param  {uint8_t} channel : the channel of MCP3004
 * @return {float}           : the ADC value in Volt
 */
uint16_t readAdc(uint8_t channel);
/**
 * Trigger initialisation
 * @param  {uint16_t} count_max  : number of periods to be sampled
 */
void init_trigger(int16_t count_max);

/**
 * Reset the counter and sampling flags, enable trigger ISR
 */
void start_trigger();

/**
 * Pause the counter and disable trigger ISR
 */
void stop_trigger();

void IRAM_ATTR trigger_GPIO();

void init_timer();

// Empty callback, to be implemented in user code
void timer_callback(void *arg);

void timer_start_periodic(uint16_t second);

void timer_stop();

/**
 * Home page of AutoConnect
 */
void rootPage();

void setup_MQTT();

void init_AutoConnect();

bool begin_AutoConnect();

void handle_client_AutoConnect();

boolean is_MQTT_connected();
/**
 * This format will format the float powerValue with 3 number after the decimal
 */
void formatage(float powerValue, unsigned long epoch, uint8_t buffer[], int dimension);

// semaphore a.k.a. user flags
extern bool flag_sampling;

extern esp_timer_handle_t main_timer_handle;

extern PubSubClient mqttClient;

#endif // __PRT14_H__