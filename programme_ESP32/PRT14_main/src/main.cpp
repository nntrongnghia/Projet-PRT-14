#include <Arduino.h>
#include "PRT14.h"
#include "PRT14_configuration.h"
#include "driver/gpio.h"

// Address of memory chunks for voltage and current samples
int16_t *pvoltage;
int16_t *pcurrent;
uint16_t nb_samples;
float power;
unsigned long epoch = 0;
bool flag_timer = false;
bool SENT_FLAG = 1;				//semaphore for sending message to MQTT broker
uint8_t dataBuffer[TAILLE_MSG]; //Tableau envoye au serveur MQTT
unsigned long duration = RETRY_INTERVAL;

void setup()
{
	pinMode(LED_R, OUTPUT);
	pinMode(LED_V, OUTPUT);
	digitalWrite(LED_V, LOW);
	digitalWrite(LED_R, LOW);
	Serial.begin(115200);
	init_SPI(SPI_CLOCK);
	init_trigger(NB_SAMPLE_PERIOD);
	init_timer();
	init_AutoConnect();
	if (begin_AutoConnect())
		Serial.println("WiFi connected: " + WiFi.localIP().toString());
	setup_MQTT();
	// Allocation une espace de mémoire de type DRAM d'une taille donnée
	pvoltage = (int16_t *)heap_caps_malloc(NB_SAMPLES * sizeof(int16_t), MALLOC_CAP_8BIT);
	pcurrent = (int16_t *)heap_caps_malloc(NB_SAMPLES * sizeof(int16_t), MALLOC_CAP_8BIT);
	timer_start_periodic(MEASURE_PERIOD);
	Serial.println("PRT14 Begin");
}

void loop()
{
	// If MQTT is not connected
	if (!mqttClient.connected())
	{
		if (millis() >= duration)
		{
			duration = millis() + RETRY_INTERVAL;
			//DESACTIVER F INTERRUPTION
			timer_stop();
			// Case 1: Internet connected, just retry to connect to MQTT broker
			if (check_internet())
			{
				//CMD LED
				digitalWrite(LED_V, LOW);
				digitalWrite(LED_R, HIGH);
				//RECONNEXION BROKER

				Serial.print("Attempting MQTT connection...");
				// Attempt to connect
				if (mqttClient.connect("prt14_proto", USER, MP))
				{
					Serial.println("connected");
					// mqttClient.subscribe("esp32/output");
				}
				else
				{
					Serial.print("MQTT connection failed, rc=");
					Serial.println(mqttClient.state());
					Serial.println(" Try again in 30 seconds");
				}
			}
			// Case 2: Internet not connected, try AutoConnect to WiFi
			else
			{
				heap_caps_free(pvoltage);
				heap_caps_free(pcurrent);
				ESP.restart();
			}
		}
	}
	else
	{
		//CMD LED
		digitalWrite(LED_V, HIGH);
		digitalWrite(LED_R, HIGH);
		//ACTIVER F INTERRUPTION
		timer_start_periodic(MEASURE_PERIOD);
		mqttClient.loop(); //maintient la connexion au BROKER

		if (!SENT_FLAG)
		{
			//RECUPERATION DE L HEURE
			epoch = getEpochTime();
			//FORMATAGE DES DONNEES
			formatage(power, epoch, dataBuffer, TAILLE_MSG);
			//ENVOIE DES DONNEE
			mqttClient.publish(TOPIC, dataBuffer, TAILLE_MSG); //La methode est surchargee
			SENT_FLAG = 1;
		}
	}
	handle_client_AutoConnect();
}

void timer_callback(void *arg)
{
	nb_samples = 0;
	// GPIO trigger controls 'flag_sampling'
	// which helps to start and stop the sampling at the right moment
	start_trigger();
	while (!flag_sampling)
		esp_timer_get_time(); //do nothing, if no code, ESP32 will be crashed
	while (flag_sampling)
	{
		pvoltage[nb_samples] = readAdc(VOLTAGE_CHANNEL);
		pcurrent[nb_samples] = readAdc(CURRENT_CHANNEL);
		nb_samples += 1;
	}
	stop_trigger();
	power = average_power(pvoltage, pcurrent, nb_samples, VOLTAGE_GAIN, CURRENT_GAIN);
#ifdef PRINT_POWER
	Serial.printf("P = %.3f\n", power);
#endif // PRINT_POWER
	SENT_FLAG = 0;
}