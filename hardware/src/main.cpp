#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"
#include <ArduinoJson.h>

const char *ssid = WIFI_SSID; // Get SSID from config.h
const char *pass = WIFI_PASS; // Get PASSWORD from config.h
int counter = 0;

// Definición de pines
const int humiditySensorPin = 34; // Pin para el sensor de humedad HL-69
const int ldrPin = 32;            // Pin para el sensor LDR
const int lm35Pin = 33;           // Pin para el sensor LM35
const int numSamples = 10;        // Número de muestras para promediar

// Variables para el promedio móvil (Humedad)
const int numReadings = 20;          // Número de lecturas para el promedio
float humidityReadings[numReadings]; // Array para las lecturas de humedad
int humidityIndex = 0;               // Índice actual para la humedad
float totalHumidity = 0;             // Suma total para la humedad
float averageHumidity = 0;           // Humedad promedio

// Variables para el promedio móvil (20 segundos) para lm35
const int numSegundos = 20;      // Número de lecturas de 20 segundos
float lm35Readings[numSegundos]; // Array para las lecturas de temperatura
int lm35Index = 0;               // Índice actual para la temperatura
float totalTemp = 0;             // Suma total de temperatura
float averageTemp = 0;           // Temperatura promedio

// Variables para el promedio móvil (20 segundos) para luz
const int numLuxReadings = 20;     // Número de lecturas de 20 segundos para luz
float luxReadings[numLuxReadings]; // Array para las lecturas de luz
int luxIndex = 0;                  // Índice actual para la luz
float totalLux = 0;                // Suma total de luz
float averageLux = 0;              // Luz promedio

// Variables para calcular intervalos de 2 minutos
unsigned long previousMillis = 0;      // Almacena el último tiempo en el que se ejecutó la función
const unsigned long interval = 120000; // 2 minutos en milisegundos

const int plant_id = 1;

// Constantes para el rango de valores
float MAX_TEMP;
float MIN_TEMP;
float MAX_LUX;
float MIN_LUX;
float MAX_HUM;
float MIN_HUM;

unsigned long lastLuxAlertTime;
unsigned long lastTempAlertTime;
unsigned long lastHumAlertTime;

void alerta(String message);

void connect_wifi()
{
    while (WiFi.status() != WL_CONNECTED)
    {
        WiFi.disconnect(true);
        WiFi.begin(ssid, pass);

        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
            counter++;
            if (counter >= 60)
            { // 30 seconds timeout
                Serial.println("");
                Serial.print("Retrying connecting to ");
                Serial.println(ssid);
                counter = 0;
                break;
            }
        }
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void get_plant_range()
{
    Serial.println("Getting plant ranges");

    if (WiFi.status() == WL_CONNECTED)
    { // Check WiFi connection status
        HTTPClient http;
        String url = "http://grillos.synology.me:8080/getrange/" + String(plant_id); // Update to POST URL
        http.begin(url);

        // Send the POST request with JSON data
        int httpResponseCode = http.GET();

        if (httpResponseCode > 0)
        {
            // Success, print the response
            String response = http.getString();
            Serial.println("HTTP Response code: " + String(httpResponseCode));
            Serial.println("Received JSON: ");
            Serial.println(response);

            // Parse JSON response
            DynamicJsonDocument doc(1024); // Adjust size as necessary
            DeserializationError error = deserializeJson(doc, response);

            if (!error)
            {
                // Extract maxTemp and minTemp from the JSON object
                MAX_TEMP = doc["max_temp"];
                MIN_TEMP = doc["min_temp"];
                MAX_LUX = doc["max_luz"];
                MIN_LUX = doc["min_luz"];
                MAX_HUM = doc["max_hum"];
                MIN_HUM = doc["min_hum"];
                const char *nombre = doc["nombre"];

                // Print the extracted values
                Serial.println("Plant Type: " + String(nombre));
                Serial.println("Max Humidity: " + String(MAX_HUM));
                Serial.println("Min Humidity: " + String(MIN_HUM));
                Serial.println("Max Light Level: " + String(MAX_LUX));
                Serial.println("Min Light Level: " + String(MIN_LUX));
                Serial.println("Max Temperature: " + String(MAX_TEMP));
                Serial.println("Min Temperature: " + String(MIN_TEMP));
            }
            else
            {
                Serial.println("Failed to parse JSON: " + String(error.c_str()));
            }
        }
        else
        {
            // Failure, print the error
            Serial.println("Error on HTTP request: " + String(httpResponseCode));
        }

        // End the HTTP connection
        http.end();
    }
    else
    {
        connect_wifi(); // Ensure Wi-Fi is connected
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.print("Connecting to network: ");
    Serial.println(ssid);

    WiFi.disconnect(true);
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        counter++;
        if (counter >= 60)
        { // 30 seconds timeout
            ESP.restart();
        }
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    get_plant_range();

    unsigned long currentMil = millis();
    unsigned long lastLuxAlertTime = 0;
    unsigned long lastTempAlertTime = 0;
    unsigned long lastHumAlertTime = 0;
    Serial.println(lastLuxAlertTime);
    // Inicializar los arrays de lecturas a 0
    for (int i = 0; i < numReadings; i++)
    {
        humidityReadings[i] = 0;
    }
    for (int i = 0; i < numSegundos; i++)
    {
        lm35Readings[i] = 0;
    }
}

void medirLuz()
{
    // ---- LUZ ----
    // Leer el valor analógico del LDR
    int lightSensorValue = analogRead(ldrPin);

    // Convertir el valor analógico a voltaje
    float lightVoltage = lightSensorValue * (3.3 / 4095.0); // Usar 3.3V

    // Calcular lux a partir del valor de luz leído
    float lux = (lightSensorValue / 4095.0) * 10000; // Ajustar el factor según el rango esperado

    // ---- PROMEDIO MÓVIL DE 20 SEGUNDOS ----
    totalLux -= luxReadings[luxIndex];          // Restar la lectura más antigua
    luxReadings[luxIndex] = lux;                // Guardar la nueva lectura
    totalLux += luxReadings[luxIndex];          // Añadir la nueva lectura
    luxIndex = (luxIndex + 1) % numLuxReadings; // Mover al siguiente índice circular

    // Calcular el promedio de luz de los últimos 20 segundos
    averageLux = totalLux / numLuxReadings;

    // Imprimir la luz en lux y el promedio
    Serial.print("Luz: ");
    Serial.print(lux, 2); // Imprimir la luz actual en lux
    Serial.println(" lux");

    Serial.print("Promedio luz 20 segundos: ");
    Serial.print(averageLux, 2); // Imprimir el promedio de luz
    Serial.println(" lux");
}

void medirTemperatura()
{
    // ---- TEMPERATURA ----
    float lm35Voltage = 0;

    // Tomar varias muestras para reducir fluctuaciones
    for (int i = 0; i < numSamples; i++)
    {
        int sensorValue = analogRead(lm35Pin);     // Leer valor analógico del LM35DZ
        lm35Voltage += sensorValue * (5 / 4095.0); // Convertir el valor ADC a voltaje (ajustado a 3.3V)
        delay(10);                                 // Pequeño retardo entre lecturas
    }
    lm35Voltage /= numSamples; // Promediar las muestras

    // Convertir el voltaje a temperatura en grados Celsius
    float temperatureC = lm35Voltage * 100 + 5; // 10mV por grado Celsius, multiplicado por 100 para convertir

    // ---- PROMEDIO MÓVIL DE 20 SEGUNDOS ----
    totalTemp -= lm35Readings[lm35Index];      // Restar la lectura más antigua
    lm35Readings[lm35Index] = temperatureC;    // Guardar la nueva lectura
    totalTemp += lm35Readings[lm35Index];      // Añadir la nueva lectura
    lm35Index = (lm35Index + 1) % numSegundos; // Mover al siguiente índice circular

    // Calcular el promedio de temperatura de los últimos 20 segundos
    averageTemp = totalTemp / numSegundos;

    // Imprimir la temperatura actual y la temperatura promedio
    Serial.print("Temperatura: ");
    Serial.print(temperatureC, 2); // Imprimir la temperatura actual con 2 decimales
    Serial.println(" °C");

    Serial.print("Temperatura promedio 20 segundos: ");
    Serial.print(averageTemp, 2); // Imprimir el promedio de 20 segundos
    Serial.println(" °C");
}

void medirHumedad()
{
    // ---- HUMEDAD ----
    int humiditySensorValue = analogRead(humiditySensorPin);

    // Convertir el valor analógico a voltaje (3.3V es el voltaje de referencia)
    float humidityVoltage = humiditySensorValue * (3.3 / 4095.0); // Usar 3.3V

    // Convertir el voltaje a porcentaje de humedad (invertido)
    float humidityPercentage = 100 - (humidityVoltage / 3.3) * 100;
    humidityPercentage = map(humidityPercentage, 0, 70, 0, 100);

    // Aplicar el promedio móvil para la humedad
    totalHumidity -= humidityReadings[humidityIndex];     // Restar la lectura más antigua
    humidityReadings[humidityIndex] = humidityPercentage; // Guardar la nueva lectura
    totalHumidity += humidityReadings[humidityIndex];     // Añadir la nueva lectura
    humidityIndex = (humidityIndex + 1) % numReadings;    // Mover al siguiente índice

    // Calcular la humedad promedio
    averageHumidity = totalHumidity / numReadings;

    // Asegurarse de que el porcentaje esté dentro del rango 0-100
    if (averageHumidity < 0)
    {
        averageHumidity = 0;
    }
    else if (averageHumidity > 100)
    {
        averageHumidity = 100;
    }

    // Imprimir el valor instantáneo de humedad y el promedio
    Serial.print("Humedad instantánea: ");
    Serial.print(humidityPercentage, 2); // Imprimir la humedad instantánea en %
    Serial.println(" %");

    Serial.print("Humedad promedio: ");
    Serial.print(averageHumidity, 2); // Imprimir la humedad promedio en %
    Serial.println(" %");
}

// Función para ejecutar algo cada 5 minutos
void ejecutarFuncionCada5Minutos()
{
    Serial.println("Han pasado 2 minutos. Ejecutando función...");

    if (WiFi.status() == WL_CONNECTED)
    { // Check WiFi connection status
        HTTPClient http;
        String url = "http://grillos.synology.me:8080/logdata"; // Update to POST URL
        http.begin(url);

        // Set content type to JSON
        http.addHeader("Content-Type", "application/json");

        Serial.println(averageHumidity);
        Serial.println(String(averageHumidity));
        // Create JSON data
        String jsonData = "{\"plant_id\": \"" + String(plant_id) + "\", \"soil_humidity\": " + String(averageHumidity) + ", \"light_level\": " + String(averageLux) + ", \"temperature\": " + String(averageTemp) + "}";

        // Send the POST request with JSON data
        int httpResponseCode = http.POST(jsonData);

        if (httpResponseCode > 0)
        {
            // Success, print the response
            String response = http.getString();
            Serial.println("HTTP Response code: " + String(httpResponseCode));
            Serial.println("Received JSON: ");
            Serial.println(response);
        }
        else
        {
            // Failure, print the error
            Serial.println("Error on HTTP request: " + String(httpResponseCode));
        }

        // End the HTTP connection
        http.end();
    }
    else
    {
        connect_wifi(); // Ensure Wi-Fi is connected
    }
}

String serverUrl = "http://grillos.synology.me:8080/alert";

void alerta(String message)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        http.begin(serverUrl);
        http.addHeader("Content-Type", "application/json"); // Set header for JSON data

        // Create JSON payload
        StaticJsonDocument<200> doc;
        doc["message"] = message;
        doc["plant_id"] = plant_id;

        String requestBody;
        serializeJson(doc, requestBody);

        // Send POST request
        int httpResponseCode = http.POST(requestBody);

        Serial.println("");
        Serial.println("");
        Serial.println("");
        Serial.println("");
        Serial.println("");
        Serial.println("");
        Serial.println("");
        Serial.println(message);
        Serial.println("Alert sent successfully!");
        Serial.println("");
        Serial.println("");
        Serial.println("");

        // Check response
        if (httpResponseCode > 0)
        { // 201 Created
            String response = http.getString();
            Serial.println("");
            Serial.println("");
            Serial.println("");
            Serial.println("");
            Serial.println("");
            Serial.println("");
            Serial.println("");
            Serial.println("");
            Serial.println("Alert sent successfully!");
            Serial.println("Response: " + response);
            Serial.println("");
            Serial.println("");
            Serial.println("");
        }
        else
        {
            Serial.print("Error sending alert, HTTP response code: ");
            Serial.println(httpResponseCode);
        }

        http.end(); // Free resources
    }
    else
    {
        Serial.println("WiFi not connected");
    }
}

const unsigned long alertInterval = 30000; // 30 secs in milliseconds

void checkRanges()
{
    bool error = false;
    String alert = "Alerta! Problemas con planta con id: " + String(plant_id) + "\n";
    unsigned long currentMillis = millis(); // Get the current time

    // Check Lux
    if (averageLux < MIN_LUX && currentMillis - lastLuxAlertTime >= alertInterval)
    {
        alert += "Luz por debajo del rango minimo. Exponer a la planta a mas luz.\n";
        error = true;
        lastLuxAlertTime = currentMillis; // Update last alert time for Lux
    }
    else if (averageLux > MAX_LUX && currentMillis - lastLuxAlertTime >= alertInterval)
    {
        alert += "Luz por arriba del rango maximo. Alejar la planta de la luz.\n";
        error = true;
        lastLuxAlertTime = currentMillis; // Update last alert time for Lux
    }

    // Check Temp
    if (averageTemp < MIN_TEMP && currentMillis - lastTempAlertTime >= alertInterval)
    {
        alert += "Temperatura mas baja de lo recomendado. Subir la temperatura del ambiente.\n";
        error = true;
        lastTempAlertTime = currentMillis; // Update last alert time for Temp
    }
    else if (averageTemp > MAX_TEMP && currentMillis - lastTempAlertTime >= alertInterval)
    {
        alert += "Temperatura mas alta de lo recomendado. Bajar la temperatura del ambiente.\n";
        error = true;
        lastTempAlertTime = currentMillis; // Update last alert time for Temp
    }

    // Check Hum
    if (averageHumidity < MIN_HUM && currentMillis - lastHumAlertTime >= alertInterval)
    {
        alert += "Humedad de la tierra muy baja. Se recomienda regar la planta.\n";
        error = true;
        lastHumAlertTime = currentMillis; // Update last alert time for Humidity
    }
    else if (averageHumidity > MAX_HUM && currentMillis - lastHumAlertTime >= alertInterval)
    {
        alert += "Humedad de la tierra muy alta.\n";
        error = true;
        lastHumAlertTime = currentMillis; // Update last alert time for Humidity
    }

    // Send alert if there is an error
    if (error)
    {
        Serial.print(alert);
        alerta(alert);
    }
}

void loop()
{
    unsigned long currentMillis = millis(); // Tiempo actual

    // Comprueba si han pasado 2 minutos (120,000 ms)
    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis; // Actualiza el tiempo para la próxima comparación

        ejecutarFuncionCada5Minutos(); // Llama a la función que quieres ejecutar cada 5 minutos
    }

    // Llamar a las funciones que miden luz, temperatura y humedad
    medirLuz();
    medirTemperatura();
    medirHumedad(); // La función de humedad ya estaba implementada

    checkRanges();

    Serial.print("");
    Serial.println("===============================");

    // Esperar un segundo antes de la siguiente lectura
    delay(1000);
}