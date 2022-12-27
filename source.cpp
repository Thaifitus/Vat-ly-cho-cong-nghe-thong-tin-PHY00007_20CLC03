#include <WiFi.h>
#include <PubSubClient.h>
#include <string.h>
#include <string>

// ========== GLOBAL VARIABLE ==========
// Giá trị ánh sáng (đơn vị đo: lux - lumen per square metre)
char lux[7];
// Trạng thái đèn (On/Off), dùng strcpy để gán
char ledState[4];
// Nhiệt độ môi trường (độ C)
char celsius[4];
// Chế độ hệ thống (true: tự động, false: thủ công)
String sysState = "true";
// Photoresistor pin
int photoPin = 35;
// Leds pin
int ledPin0 = 12, ledPin1 = 14, ledPin2 = 27, ledPin3 = 26;
// Temperature sensor pin
int tempPin = 32;
// PIR sensor
int pirPin = 33;
int pirState = LOW; // mặc định lúc chưa khởi động
// Thông báo về ifttt
char beNotifi[6]; // Bật/tắt thông báo từ người dùng
bool notifi = false;
int notifiType = -1; // Loại thông báo (trạng thái đèn = 0, cảnh báo nhiệt độ = 1)

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
            // Topic chế độ hệ thống (tự động/thủ công)
            client.subscribe("sys/state");

            // Topic bật/tắt đèn trên website
            client.subscribe("sys/ledOn");

            // Topic nhận thông báo từ ifttt
            client.subscribe("sys/notifi");
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
    Serial.begin(9600);
    Serial.print("Connecting to WiFi");

    wifiConnect();
    client.setServer(mqttServer, port);
    client.setCallback(callback);

    // Hệ thống đèn
    pinMode(ledPin0, OUTPUT);
    pinMode(ledPin1, OUTPUT);
    pinMode(ledPin2, OUTPUT);
    pinMode(ledPin3, OUTPUT);
    // Cảm biến ánh sáng
    pinMode(photoPin, INPUT);
    // Cảm biến nhiệt độ
    pinMode(tempPin, INPUT);
    // Cảm biến chuyển động
    pinMode(pirPin, INPUT);
}

// ========== HÀM HỖ TRỢ ==========
// Hàm bật/tắt hệ thống đèn
void TurnLed(int status)
{
    switch (status)
    {
    // Tắt đèn
    case 0:
    {
        digitalWrite(ledPin0, LOW);
        digitalWrite(ledPin1, LOW);
        digitalWrite(ledPin2, LOW);
        digitalWrite(ledPin3, LOW);
        if (strcmp(ledState, "On") == 0)
        {
            notifi = true;
            notifiType = 0;
        }
        strcpy(ledState, "Off");
        break;
    }
    // Bật đèn
    case 1:
    {
        digitalWrite(ledPin0, HIGH);
        digitalWrite(ledPin1, HIGH);
        digitalWrite(ledPin2, HIGH);
        digitalWrite(ledPin3, HIGH);
        if (strcmp(ledState, "Off") == 0)
        {
            notifi = true;
            notifiType = 0;
        }
        strcpy(ledState, "On");
        break;
    }
    }
    client.publish("sys/led", ledState);
}
// MQTT Receiver
void callback(char *topic, byte *message, unsigned int length)
{
    Serial.println("Wokwi received from topic: ");
    Serial.println(topic);
    String strMsg;
    for (int i = 0; i < length; i++)
    {
        strMsg += (char)message[i];
    }
    Serial.println(strMsg);

    //***Insert code here to control other devices***
    // Trạng thái hệ thống
    if (strcmp(topic, "sys/state") == 0)
    {
        sysState = strMsg;
    }
    // Bật/tắt đèn thủ công
    else if (strcmp(topic, "sys/ledOn") == 0)
    {
        // Tắt đèn
        if (strMsg.compareTo("false") == 0)
        {
            TurnLed(0);
        }
        // Bật đèn
        else
        {
            TurnLed(1);
        }
    }
    else if (strcmp(topic, "sys/notifi") == 0)
    {
        strcpy(beNotifi, strMsg.c_str());
    }
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
    const float BETA = 3950; // should match the Beta Coefficient of the thermistor
    float analogValue = analogRead(tempPin) * (float)1023 / 4095;
    int celsiusI = 1 / (log(1 / (1023. / analogValue - 1)) / BETA + 1.0 / 298.15) - 273.15;
    sprintf(celsius, "%i", celsiusI);
    client.publish("sys/temperature", celsius);
    // Nhiệt độ trên 50C -> thông báo ifttt
    if (celsiusI > 50)
    {
        notifi = true;
        notifiType = 1;
    }
    //

    // Kiểm tra trạng thái của hệ thống (tự động hoặc thủ công)
    int state = sysState.compareTo("true");
    // Chức năng theo trạng thái hệ thống
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
        float analogValue = analogRead(photoPin) * (float)1023 / 4095;
        float voltage = analogValue / 1024. * 5;
        float resistance = 2000 * voltage / (1 - voltage / 5);
        int luxI = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA));
        sprintf(lux, "%i", luxI);
        client.publish("sys/photo", lux);

        if (luxI <= 100)
        {
            TurnLed(1);
        }
        else
        {
            TurnLed(0);
        }
        //
        break;
    }

    // Hệ thống chạy thủ công
    default:
    {
        // Bật/tắt đèn bằng cảm biến chuyển động
        int val = digitalRead(pirPin);
        if (val == HIGH)
        {
            TurnLed(1);
            if (pirState == LOW)
            {
                // bật cảm biến lên
                Serial.println("Motion detected!");
                Serial.println("Delay for 5s");
                pirState = HIGH;
            }
        }
        else
        {
            TurnLed(0);
            if (pirState == HIGH)
            {
                // Tắt cảm biến
                Serial.println("Motion ended!");
                Serial.println("Wait 1.2s for next Motion");
                pirState = LOW;
            }
        }
        break;
    }
    }

    // Thông báo ifttt
    if (notifi == true && strcmp(beNotifi, "true") == 0)
    {
        switch (notifiType)
        {
        // Thông báo tình trạng đèn
        case 0:
        {
            client.publish("ifttt/led", ledState);
            break;
        }
        // Cảnh báo nhiệt độ
        case 1:
        {
            client.publish("ifttt/temperature", celsius);
            break;
        }
        }
        // Đã thông báo ifttt
        notifi = false;
        notifiType = -1;
    }
    delay(5000);
}