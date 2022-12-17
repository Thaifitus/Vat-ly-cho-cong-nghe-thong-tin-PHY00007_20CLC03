#include <WiFi.h>
#include <PubSubClient.h>
#include <string.h>

// ========== GLOBAL VARIABLE ==========
// Giá trị ánh sáng (đơn vị đo: lux - lumen per square metre)
float lux = 0;
// Trạng thái đèn (On/Off), dùng strcpy để gán
char ledState[4];
// Nhiệt độ môi trường (độ C)
float celsius = 0;
// Chế độ hệ thống (true: tự động, false: thủ công)
String sysState = "true";
// Photoresistor pin
int photoPin = 13;
// Leds pin
int ledPin0 = 12, ledPin1 = 14, ledPin2 = 27, ledPin3 = 26;

// ========== INTERNET CONNECTION ==========
const char *ssid = "Wokwi-GUEST";
const char *password = "";
//***Set server***
const char *mqttServer = "broker.hivemq.com";
int port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

void wifiConnect()
{
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" Connected!");
}
void mqttReconnect()
{
    while (!client.connected())
    {
        Serial.println("Attemping MQTT connection...");
        //***Change "123456789" by your student id***
        if (client.connect("172456625"))
        {
            Serial.println("connected");

            //***Subscribe all topic you need***
            client.subscribe(" ");
        }
        else
        {
            Serial.println("try again in 5 seconds");
            delay(5000);
        }
    }
}

// ========== SETUP ==========
void setup()
{
    Serial.begin(115200);
    Serial.print("Connecting to WiFi");

    wifiConnect();
    client.setServer(mqttServer, port);
    client.setCallback(callback);
}

// MQTT Receiver
void callback(char *topic, byte *message, unsigned int length)
{
    Serial.println(topic);
    String strMsg;
    for (int i = 0; i < length; i++)
    {
        strMsg += (char)message[i];
    }
    Serial.println(strMsg);
    //***Insert code here to control other devices***
}

// ========== LOOP ==========
void loop()
{
    if (!client.connected())
    {
        mqttReconnect();
    }
    client.loop();

    //***Change code to publish to MQTT Server***
    // int temp = random(0, 100);
    // char buffer[50];
    // sprintf(buffer, "%d", temp);
    // client.publish("mssv/temp", buffer);

    // Đo nhiệt độ môi trường bằng cảm biến nhiệt độ

    //
    int state = sysState.compareTo("true");
    switch (state)
    {
    // Hệ thống chạy tự động
    case 0:
    {
        // Bật/tắt đèn tự động
        // These constants should match the photoresistor's "gamma" and "rl10" attributes
        const float GAMMA = 0.7;
        const float RL10 = 50;

        // Convert the analog value into lux value:
        float analogValue = analogRead(13) * (float)1023 / 4095;
        float voltage = analogValue / 1024. * 5;
        float resistance = 2000 * voltage / (1 - voltage / 5);
        float lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA));
        if (lux <= 100)
        {
            digitalWrite(ledPin0, HIGH);
            digitalWrite(ledPin1, HIGH);
            digitalWrite(ledPin2, HIGH);
            digitalWrite(ledPin3, HIGH);
        }
        else
        {
            digitalWrite(ledPin0, LOW);
            digitalWrite(ledPin1, LOW);
            digitalWrite(ledPin2, LOW);
            digitalWrite(ledPin3, LOW);
        }

        //
        break;
    }

    // Hệ thống chạy thủ công
    default:
    {
        // Bật/tắt đèn bằng cảm biến âm thanh

        //
        break;
    }
    }

    delay(5000);
}
