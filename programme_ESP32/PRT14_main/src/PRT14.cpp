#include "PRT14.h"

WebServer Server;
AutoConnect Portal(Server);
AutoConnectConfig Config;

WiFiClient espClient;
PubSubClient mqttClient(espClient);
const char *mqtt_server = IP_ADR_MQTT_BROKER;

// variables used for the getEpochTime function
WiFiUDP _ntpUDP;
NTPClient _timeClient(_ntpUDP);

bool flag_sampling = false;
esp_timer_handle_t main_timer_handle;

// return True if connected to the Internet
bool check_internet()
{
    return Ping.ping("www.google.com");
}

void beginNTPClient()
{
    _timeClient.begin();
    _timeClient.setTimeOffset(0);
}

// return time in seconds since Jan. 1, 1970
unsigned long getEpochTime()
{
    while (!_timeClient.update())
    {
        _timeClient.forceUpdate();
    }
    return _timeClient.getEpochTime();
}

void init_SPI(uint32_t spi_clock)
{
    pinMode(ADC_CS_PIN, OUTPUT);
    digitalWrite(ADC_CS_PIN, HIGH); //initialement on bloque la connexion SPI avec le Chip Select (CP) au niveau haut (actif au niveau bas)
    // initialisation SPI:
    SPI.setBitOrder(MSBFIRST);  // Not strictly needed but just to be sure, permet d'etre sur qu'on envoie les bits dans le bon sens lors de l'usage de la fonction tranfert()
    SPI.setDataMode(SPI_MODE0); // Not strictly needed but just to be sure.
    SPI.setFrequency(spi_clock);
    SPI.begin();
}

uint16_t readAdc(uint8_t channel)
{
    uint8_t readBuffer[3], writeBuffer[3];
    writeBuffer[0] = 0x01;
    writeBuffer[1] = (channel << 4) | 0b10000000;
    writeBuffer[2] = 0;
    uint16_t adcResult;
    // noInterrupts();                // pas d'interruptions pendant la transmission SPI
    digitalWrite(ADC_CS_PIN, LOW); // on lance la connexion SPI
    SPI.transferBytes(writeBuffer, readBuffer, 3);
    digitalWrite(ADC_CS_PIN, HIGH); // on arrete la connexion SPI
    // interrupts();                   // la liaison SPI est terminee on peut de nouveau autoriser les interruptions
    adcResult = (readBuffer[1] & 0x03) << 8;
    adcResult |= readBuffer[2];
    return adcResult;
}

int16_t _count_max;
void init_trigger(int16_t count_max)
{
    pinMode(PCNT_INPUT_SIG_IO, INPUT_PULLDOWN);
    _count_max = count_max;
}

uint16_t _counter = 0;
void IRAM_ATTR trigger_GPIO()
{
    if (!flag_sampling)
    {
        flag_sampling = true;
        _counter = 0;
    }
    if (flag_sampling)
    {
        if (_counter < _count_max * 2 + 1)
            _counter += 1;
        else
            flag_sampling = false;
    }
}

void start_trigger()
{
    attachInterrupt(digitalPinToInterrupt(PCNT_INPUT_SIG_IO), trigger_GPIO, CHANGE);
    flag_sampling = false;
}

void stop_trigger()
{
    detachInterrupt(digitalPinToInterrupt(PCNT_INPUT_SIG_IO));
}

void average_bw_intervals(const int16_t *arrayIn, int16_t *arrayOut, int array_length)
{
    arrayOut[0] = 0;
    for (int i = 1; i < array_length; i++)
    {
        arrayOut[i] = (arrayIn[i - 1] + arrayIn[i]) / 2;
    }
}

void set_offset(int16_t *arrayIn, uint16_t length, int16_t offset)
{
    for (int i = 0; i < length; i++)
        arrayIn[i] += offset;
}

float average_power(int16_t *pvoltage, int16_t *pcurrent, int nb_samples,
                    float voltage_gain, float current_gain)
{
    float power = 0;

    // i start at 1 to skip the first value of I interpolated which is 0
    for (int i = 1; i < nb_samples; i++)
        // calculate the power value (I linear interpolated, both I and V compensated with offset)
        power += (pvoltage[i] - OFFSET_VOLTAGE) * ((pcurrent[i - 1] + pcurrent[i]) / 2 - OFFSET_CURRENT);
    nb_samples -= 1;
    return power / (float)nb_samples / voltage_gain / current_gain;
}

void init_timer()
{
    esp_timer_create_args_t my_timer_arg;
    my_timer_arg.callback = &timer_callback;
    my_timer_arg.arg = NULL;
    my_timer_arg.name = "hello_timer";

    // initialize the timer handler
    esp_err_t ret = esp_timer_create(&my_timer_arg, &main_timer_handle);
    ESP_ERROR_CHECK(ret);
}

void timer_start_periodic(uint16_t second)
{
    esp_timer_start_periodic(main_timer_handle, 1000000 * second);
}

void timer_stop()
{
    esp_timer_stop(main_timer_handle);
}

void rootPage()
{
    char content[] = "Hello, PRT14";
    Server.send(200, "text/plain", content);
}

void setup_MQTT()
{
    mqttClient.setServer(mqtt_server, 1883);
}

void init_AutoConnect()
{
    Config.apid = "ESP32-PRT14";
    Config.psk = "4ge4prt14";
    Config.autoReconnect = true;
    Config.portalTimeout = 120000; // It will time out in 120 seconds
    Portal.config(Config);
    delay(1000);
    Server.on("/", rootPage);
}

bool begin_AutoConnect()
{
    return Portal.begin();
}

void handle_client_AutoConnect()
{
    Portal.handleClient();
}

boolean is_MQTT_connected()
{
    return mqttClient.connected();
}

void formatage(float powerValue, unsigned long epoch, uint8_t buffer[], int dimension)
{
    uint64_t data = 0;
    uint32_t power1000 = powerValue * 1000;
    data = epoch;
    data = data << 32;
    data = data | power1000;

    for (int i = 0; i < dimension; i++)
    {
        buffer[i] = data & 0xFF;
        data = data >> 8;
    }
}