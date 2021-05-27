#include <Arduino.h>
#include <Math.h>
// #include <Communication.h>
#include "src/AMG.h"
#include "src/K210.h"
#include "src/ToF.h"

#include "pin.h"

#include <SoftwareSerial.h>
#include <STM32FreeRTOS.h>

void TaskBlink( void *pvParameters );
void TaskMain( void *pvParameters );

// Communication *com = new Communication();
ToF *tof = new ToF(I2C2_SDA, I2C2_SCL);
AMG *amg8833 = new AMG(I2C1_SDA, I2C1_SCL);
K210 *k210 = new K210(UART1_RX, UART1_TX);

// HardwareSerial Serial2(USART2); // STM32F103C8
// HardwareSerial Serial1(PA10, PA9); // K210 / D0-D1

void TaskBlink(void *pvParameters){
	(void) pvParameters;
	pinMode(LED_BUILTIN, OUTPUT);

  	for (;;){
    	digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    	vTaskDelay( 250 / portTICK_PERIOD_MS ); // wait for one second
    	digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    	vTaskDelay( 250 / portTICK_PERIOD_MS ); // wait for one second
  	}
}


void TaskMain(void *pvParameters){
	(void) pvParameters;
	for (;;){

        if(tof->isBegin && k210->isBegin && amg8833->isBegin){

            if(k210->data[K210::dataType::status] == K210::statusCode::singleFace){
                
                if(DEBUG){
                    Serial.print("Face Detected\n");

                    Serial.print("\tCam Coordinates: ");
                    Serial.print(k210->data[K210::dataType::x]);
                    Serial.print(" ");
                    Serial.print(k210->data[K210::dataType::y]);
                    Serial.print(" ");
                    Serial.print(k210->data[K210::dataType::w]);
                    Serial.print(" ");
                    Serial.print(k210->data[K210::dataType::h]);
                    Serial.print("\n");

                    Serial.print("\tAMG Max Temp: ");
                    Serial.print(amg8833->data[AMG::dataType::maxTemp]);
                    Serial.print("\n");
                    Serial.print("\tAMG Min Temp: ");
                    Serial.print(amg8833->data[AMG::dataType::minTemp]);
                    Serial.print("\n");
                    Serial.print("\tAMG Average Temp: ");
                    Serial.print(amg8833->data[AMG::dataType::averageTemp]);
                    Serial.print("\n");

                    Serial.print("\tToF Status: ");
                    Serial.print(tof->rangeStatusToString(tof->data[ToF::dataType::status]));
                    Serial.print("\n");
                    Serial.print("\tToF Distance: ");
                    Serial.print(tof->data[ToF::dataType::range]);
                    Serial.print("\n");

                    Serial.print("\n");
                }    
                
                // ==> Check Conditions
                bool everythingIsOkay = true;
                // Kameranin cozunurlugu 320x240 px
                // AMG interpolasyon sonrasi 48x48 px

                // 1. Bulunan yuzun x kordinatlarinin 240x240 icerisinde olmasi gerekir
                // Ilk ve son 40 pixelde bulunmamali
                if(k210->data[K210::dataType::x] < 40){
                    if(DEBUG){ Serial.println("\t==> ERROR: Out of frame X Left <=="); }
                    else { Serial.print("A"); }
                    everythingIsOkay = false;
                }
                if((k210->data[K210::dataType::x] + k210->data[K210::dataType::w]) > 281){
                    if(DEBUG){ Serial.println("\t==> ERROR: Out of frame X Right <=="); }
                    else { Serial.print("B"); }
                    everythingIsOkay = false;
                }

                // 2. Maksimum mesafe 1m olmali ve tof duzgun olcum yapmis olmali
                // if(tof->data[ToF::dataType::status] != 0){
                //     if(DEBUG){ Serial.println("\t==> ERROR: ToF distance is not valid <=="); }
                //     else { Serial.print("C"); }
                //     everythingIsOkay = false;
                // }

                // if(tof->data[ToF::dataType::range] > 1000){
                //     if(DEBUG){ Serial.println("\t==> ERROR: ToF distance couldn't be bigger than 1m  <=="); }
                //     else { Serial.print("D"); }
                //     everythingIsOkay = false;
                // }

                if(everythingIsOkay){
                    // Calculate temp of AMG pixels using by face coordinates
                    amg8833->calculatePixels(
                        k210->resolutionY,
                        k210->data[K210::dataType::x] - 40, // Shift first 40 pixels
                        k210->data[K210::dataType::y],
                        k210->data[K210::dataType::w] - 40, // Shift first 40 pixels
                        k210->data[K210::dataType::h]
                    );


                    if(DEBUG){
                        Serial.print("\tFace Max Temp: ");
                        Serial.print(amg8833->calculatedData[AMG::dataType::maxTemp]);
                        Serial.print("\n");
                        Serial.print("\tFace Min Temp: ");
                        Serial.print(amg8833->calculatedData[AMG::dataType::minTemp]);
                        Serial.print("\n");
                        Serial.print("\tFace Average Temp: ");
                        Serial.print(amg8833->calculatedData[AMG::dataType::averageTemp]);
                        Serial.print("\n");
                    }else{
                        Serial.print("H");
                        Serial.print(amg8833->calculatedData[AMG::dataType::maxTemp]);
                        Serial.print("&");
                        Serial.print(amg8833->calculatedData[AMG::dataType::minTemp]);
                        Serial.print("&");
                        Serial.print(amg8833->calculatedData[AMG::dataType::averageTemp]);
                        Serial.print("&");
                    }
                }
            }
            
            else if(k210->data[K210::dataType::status] == K210::statusCode::multipleFace){
                if(DEBUG){ Serial.println("Multiple Face"); }
                else { Serial.print("E"); }
            }
            
            else if(k210->data[K210::dataType::status] == K210::statusCode::noFace){
                if(DEBUG){ Serial.println("No Face"); }
                else { Serial.print("F"); }
            }
            
            Serial.print("\n");
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        
        else{
            if(DEBUG){
				Serial.print("Connection Error -> ");
				if(!tof->isBegin){ Serial.print("TOF "); }
				if(!k210->isBegin){ Serial.print("K210 "); }
				if(!amg8833->isBegin){ Serial.print("AMG "); }
				Serial.print("\n");
			}else{
                Serial.print("G\n");
            }

            vTaskDelay(1 / portTICK_PERIOD_MS);
        }
    }
}


void setup() {

    Serial.begin(9600); // UART2 - STLINK

    k210->debug = false;
    amg8833->debug = false;
    tof->debug = false;
    
    tof->begin();
    amg8833->begin();
    k210->begin();

    // Serial.println("SETUP");

    // Start K210 Task
    tof->start();
    amg8833->start();
    k210->start();

    xTaskCreate(TaskBlink, (const portCHAR *)"TaskBlink", 128, NULL, 2, NULL);
    xTaskCreate(TaskMain, (const portCHAR *)"TaskMain", 128, NULL, 2, NULL);

  	vTaskStartScheduler();
  	Serial.println("Insufficient RAM");
}

void loop(){}