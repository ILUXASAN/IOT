#include "Adafruit_INA219.h"
#include "Wire.h"
#include "ESP8266WiFi.h"
#include "PubSubClient.h"

#define power_topic "user_e65d8e7a/iot/power_consumption"
#define current_topic "user_e65d8e7a/iot/current_consumption"
#define voltage_topic "user_e65d8e7a/iot/voltage_consumption"
#define relay_control_topic "user_e65d8e7a/iot/relay_control"
#define pushups_topic "user_e65d8e7a/iot/pushups"
#define PIN_RELAY 2
#define PIN_VOLT_SENSOR A0
#define port 9124

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_INA219 ina219;

const char* ssid = "Wi-Fi login";
const char* password = "Wi-Fi pass";
const char* mqttserver = "srv1.clusterfly.ru";
const char* mqtt_user = "mqtt_login";
const char* mqtt_password = "mqtt_password";

#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];

void setup_wifi() {

	delay(10);
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid);

	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	randomSeed(micros());

	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (int i = 0; i < length; i++) {
		Serial.print((char)payload[i]);
	}
	Serial.println();

	if ((char)payload[0] == '0') {
		digitalWrite(PIN_RELAY, LOW);   // Выключаем реле - замыкаем цепь
	}
	else {
		digitalWrite(PIN_RELAY, HIGH);  // Вкключаем реле - размыкаем цепь
	}

}

void reconnect() {
	while (!client.connected()) {
		Serial.print("Attempting MQTT connection...");
		String clientId = "user_e65d8e7a_wattmeter";
		if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
			Serial.println("connected");
			client.publish(power_topic, "hello world");
			client.publish(voltage_topic, "hello world");
			client.publish(voltage_topic, "hello world");
			client.subscribe(relay_control_topic);
		}
		else {
			Serial.print("failed, rc=");
			Serial.print(client.state());
			Serial.println(" try again in 5 seconds");
			delay(5000);
		}
	}
}

void setup() {
	pinMode(PIN_RELAY, OUTPUT); // Объявляем пин реле как выход
	digitalWrite(PIN_RELAY, LOW); // Выключаем реле
	Serial.begin(115200);       // Инициализируем последовательную связь на скорости 115200
	setup_wifi();
	client.setServer(mqttserver, port);
	client.setCallback(callback);
	if (!ina219.begin()) {
		Serial.println("Failed to find INA219 chip");
		while (1) { delay(10); }
	}
}

void loop()
{
	if (!client.connected()) {
		reconnect();
	}
	client.loop();

	float current_mA = 0;       // Создаем переменную current_mA, куда будут записываться показания датчика тока
	float power_W = 0;			// Создаем переменную power_W, куда будет записываться вычисленное значение мощности
	float voltage_V = 0;		// Создаем переменную voltage_V, куда будут записываться показания датчика напряжения

	current_mA = ina219.getCurrent_mA();              // Получение значение тока в мА
	voltage_V = analogRead(PIN_VOLT_SENSOR) * 5.0 / 1024;            // Получение значения напряжения в В
	power_W = current_mA / 1000 * voltage_V;                  // Получение значение мощности в Вт    

	if (power_W > 2)
	{
		digitalWrite(PIN_RELAY, HIGH); // Включаем реле, т.е. размыкаем цепь
		client.publish(power_topic, "Power consumption exceeded. Check the condition of the equipment", true);
    	client.publish(pushups_topic, "Power consumption exceeded!");
	}

	// Поочередно отправляем полученные значение в последовательный порт.
	Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
	Serial.print("Power:         "); Serial.print(power_W); Serial.println(" W");
	Serial.print("Voltage:         "); Serial.print(voltage_V); Serial.println(" V");

	// Публикуем полученные значения в соотвествующие топики на MQTT сервере
	client.publish(power_topic, String(power_W).c_str());
	client.publish(current_topic, String(current_mA).c_str());
	client.publish(voltage_topic, String(voltage_V).c_str());
	delay(2000);

}