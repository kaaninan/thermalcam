#include "ToF.h"

ToF::ToF(int SDA, int SCL){
    Wire2 = new TwoWire(SDA, SCL); //I2C2 STM32L432KC
}


bool ToF::begin(){
    Wire2->begin();
    Wire2->setClock(400000);

    vl53l1x.setBus(Wire2);
    vl53l1x.setTimeout(500);

    isBegin = vl53l1x.init();
    
    if(debug){
        Serial.print("TOF -> ");
        Serial.println(status);
    }

    // Use long distance mode and allow up to 50000 us (50 ms) for a measurement.
    // You can change these settings to adjust the performance of the sensor, but
    // the minimum timing budget is 20 ms for short distance mode and 33 ms for
    // medium and long distance modes. See the VL53L1X datasheet for more
    // information on range and timing limits.
    vl53l1x.setDistanceMode(VL53L1X::DistanceMode::Long);
    vl53l1x.setMeasurementTimingBudget(50000);

    // Start continuous readings at a rate of one measurement every 50 ms (the
    // inter-measurement period). This period should be at least as long as the
    // timing budget.
    vl53l1x.startContinuous(50);

    return isBegin;
}

void ToF::conf(double val){
}


void ToF::read(double* val){
    vl53l1x.read();
  
    if(debug){
        Serial.print("\nrange: ");
        Serial.print(vl53l1x.ranging_data.range_mm);
        Serial.print("\tstatus: ");
        Serial.print(VL53L1X::rangeStatusToString(vl53l1x.ranging_data.range_status));
        Serial.print("\tpeak signal: ");
        Serial.print(vl53l1x.ranging_data.peak_signal_count_rate_MCPS);
        Serial.print("\tambient: ");
        Serial.println(vl53l1x.ranging_data.ambient_count_rate_MCPS);
        Serial.println();
    }

    val[ToF::dataType::range] = vl53l1x.ranging_data.range_mm;
    val[ToF::dataType::status] = vl53l1x.ranging_data.range_status;
    val[ToF::dataType::peakSignal] = vl53l1x.ranging_data.peak_signal_count_rate_MCPS;
    val[ToF::dataType::ambient] = vl53l1x.ranging_data.ambient_count_rate_MCPS;

}


void readTaskTOF(void *pvParameters){
	(void) pvParameters;
	ToF *tof = (ToF *)pvParameters;
	for (;;){
		// Calculate Data Frequency
		if(tof->lastMillis == 0){
			tof->lastMillis = millis();
		}else{
			unsigned long timePassed = millis() - tof->lastMillis;
			if(timePassed > 1000){
				tof->lastMillis = millis();
				if(tof->debug){
					Serial.print(" => ToF Data Frequency ");
					Serial.print(tof->msgCount);
					Serial.println(" Hz");
				}
				tof->dataFrequency = tof->msgCount;
				tof->msgCount = 0;

			}
		}

		tof->read(tof->data);
        tof->msgCount++;
	}
}

void ToF::start(){
    xTaskCreate(readTaskTOF, (const portCHAR *)"readTaskTOF", 512, this, 1, NULL);
}


const char* ToF::rangeStatusToString(double status){
    switch (int(status)){
        case VL53L1X::RangeValid:
            return "range valid";

        case VL53L1X::SigmaFail:
            return "sigma fail";

        case VL53L1X::SignalFail:
            return "signal fail";

        case VL53L1X::RangeValidMinRangeClipped:
            return "range valid, min range clipped";

        case VL53L1X::OutOfBoundsFail:
            return "out of bounds fail";

        case VL53L1X::HardwareFail:
            return "hardware fail";

        case VL53L1X::RangeValidNoWrapCheckFail:
            return "range valid, no wrap check fail";

        case VL53L1X::WrapTargetFail:
            return "wrap target fail";

        case VL53L1X::XtalkSignalFail:
            return "xtalk signal fail";

        case VL53L1X::SynchronizationInt:
            return "synchronization int";

        case VL53L1X::MinRangeFail:
            return "min range fail";

        case VL53L1X::None:
            return "no update";

        default:
            return "unknown status";
  }
}