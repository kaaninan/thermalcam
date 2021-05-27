#include "K210.h"

K210::K210(int RX, int TX){
	serial = new HardwareSerial(RX, TX);
}


bool K210::begin(){
	serial->begin(BAUDRATE);
	isBegin = false; // Wait for incoming data
    return true;
}

void K210::conf(double value = -1){
}


void readTaskK210(void *pvParameters){
	(void) pvParameters;
	K210 *k210 = (K210 *)pvParameters;
	if(k210->lastMillis == 0){ k210->lastMillis = millis(); }
	for (;;){

		unsigned long timePassedData = millis() - k210->lastIncomingDataMillis;
		if(timePassedData > 1000){
			// 1sn boyunca data gelmezse eger
			k210->isBegin = false;
		}

		// Data Frequency = 10 Hz
		if(k210->serial->available()){

			k210->isBegin = true;
			k210->lastIncomingDataMillis = millis();

			// Calculate Data Frequency
			unsigned long timePassed = millis() - k210->lastMillis;
			if(timePassed > 1000){
				k210->lastMillis = millis();
				if(k210->debug){
					Serial.print(" => K210 Data Frequency ");
					Serial.print(k210->msgCount);
					Serial.println(" Hz");
				}
				k210->dataFrequency = k210->msgCount;
				k210->msgCount = 0;

			}else{
				k210->msgCount++;
			}

			String inputString = k210->serial->readStringUntil('\n');
			
			// inputString could be;
			// -> `#0` => No Face Detected
			// -> `#23,123,321,321` => Single Face Detected (#X,Y,W,H)
			// -> `#2` => Multiple Face Detected

			if(inputString == "#0"){
				// No Face
				k210->data[K210::dataType::status] = K210::statusCode::noFace;
				if(k210->debug){
					Serial.println("No Face Detected");
				}

			}else if(inputString == "#2"){
				// Multiple Faces
				k210->data[K210::dataType::status] = K210::statusCode::multipleFace;
				if(k210->debug){
					Serial.println("Multiple Face Detected");
				}

			}else{
				// Single Face
				inputString = inputString.substring(1); // remove #
				
				if(k210->debug){
					Serial.println(inputString);
					Serial.print("X: ");
					Serial.println(k210->getValue(inputString, ',', 0));
					Serial.print("Y: ");
					Serial.println(k210->getValue(inputString, ',', 1));
					Serial.print("W: ");
					Serial.println(k210->getValue(inputString, ',', 2));
					Serial.print("H: ");
					Serial.println(k210->getValue(inputString, ',', 3));
					Serial.println();
				}

				k210->data[K210::dataType::status] = K210::statusCode::singleFace;
				k210->data[K210::dataType::x] = k210->getValue(inputString, ',', 0).toDouble();
				k210->data[K210::dataType::y] = k210->getValue(inputString, ',', 1).toDouble();
				k210->data[K210::dataType::w] = k210->getValue(inputString, ',', 2).toDouble();
				k210->data[K210::dataType::h] = k210->getValue(inputString, ',', 3).toDouble();

				// Notifiy Main Task
			}
		}
	}
}


void K210::start(){
    xTaskCreate(readTaskK210, (const portCHAR *)"readTaskK210", 512, this, 1, NULL);
}


// Helper Functions

String K210::getValue(String data, char separator, int index){
	int found = 0;
	int strIndex[] = {0, -1};
	int maxIndex = data.length()-1;

	for(int i=0; i<=maxIndex && found<=index; i++){
		if(data.charAt(i)==separator || i==maxIndex){
			found++;
			strIndex[0] = strIndex[1]+1;
			strIndex[1] = (i == maxIndex) ? i+1 : i;
		}
	}

	return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}