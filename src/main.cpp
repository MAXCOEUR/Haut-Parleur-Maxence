#include <Arduino.h>
#include "BluetoothA2DPSink.h"

BluetoothA2DPSink a2dp_sink;

const int LED_PIN = 2;           
const int RESET_BUTTON_PIN = 4; 

// Variables pour la gestion du bouton
unsigned long buttonPressStartTime = 0;
bool isButtonPressed = false;
bool longPressExecuted = false;

float volumes[] = {0.25, 0.50, 0.75}; 
int currentVolumeIndex = 0;
float volume_limit = volumes[currentVolumeIndex];

// Fonction qui bride le son en direct
void read_data_stream(const uint8_t *data, uint32_t length) {
    int16_t *samples = (int16_t *) data;
    uint32_t sample_count = length / 2;
    for (uint32_t i = 0; i < sample_count; i++) {
        samples[i] = samples[i] * volume_limit;
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    pinMode(RESET_BUTTON_PIN, INPUT_PULLUP); 

    i2s_pin_config_t my_pin_config = {
        .bck_io_num = 26,
        .ws_io_num = 25,
        .data_out_num = 33,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    a2dp_sink.set_pin_config(my_pin_config);
    
    // Activation du bridage
    a2dp_sink.set_stream_reader(read_data_stream, true);
    
    a2dp_sink.set_auto_reconnect(true);
    a2dp_sink.start("Haut-Parleur Maxence");
    
    Serial.println("Prêt ! Volume actuel : 50%");
}

void loop() {
    // 1. Gestion de la LED d'état
    if (a2dp_sink.get_connection_state() != ESP_A2D_CONNECTION_STATE_CONNECTED) {
        static unsigned long lastBlink = 0;
        if (millis() - lastBlink > 500) {
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            lastBlink = millis();
        }
    } else {
        digitalWrite(LED_PIN, HIGH);
    }

    // 2. Gestion intelligente du bouton sur D13
    int buttonState = digitalRead(RESET_BUTTON_PIN);

    if (buttonState == LOW) { // Bouton appuyé
        if (!isButtonPressed) {
            buttonPressStartTime = millis();
            isButtonPressed = true;
            longPressExecuted = false;
        }

        // Cas de l'appui LONG (Reset)
        if (!longPressExecuted && (millis() - buttonPressStartTime > 3000)) {
            Serial.println("!!! RESET FACTORY !!!");
            a2dp_sink.clean_last_connection();
            for(int i=0; i<10; i++){
                digitalWrite(LED_PIN, HIGH); delay(50);
                digitalWrite(LED_PIN, LOW); delay(50);
            }
            ESP.restart();
            longPressExecuted = true;
        }
    } 
    else { // Bouton relâché
        if (isButtonPressed) {
            unsigned long pressDuration = millis() - buttonPressStartTime;

            // Cas de l'appui COURT (Changement Volume)
            if (pressDuration > 50 && pressDuration < 1000) {
                currentVolumeIndex++;
                if (currentVolumeIndex > 2) currentVolumeIndex = 0; // On boucle
                
                volume_limit = volumes[currentVolumeIndex];
                
                Serial.print("Changement Volume : ");
                Serial.print(volume_limit * 100);
                Serial.println("%");
                delay(300); 
                
                // Clignote X fois selon l'index (Mode 1, 2 ou 3)
                for(int i = 0; i <= currentVolumeIndex; i++) {
                    digitalWrite(LED_PIN, LOW);
                    delay(200);
                    digitalWrite(LED_PIN, HIGH);
                    delay(200);
                }
            }
            isButtonPressed = false;
        }
    }

    delay(10); 
}