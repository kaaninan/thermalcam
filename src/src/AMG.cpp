#include "AMG.h"

AMG::AMG(int SDA, int SCL){
	SDA_PIN = SDA;
	SCL_PIN = SCL;
}


bool AMG::begin(){
	Wire.setSDA(SDA_PIN);
    Wire.setSCL(SCL_PIN);
    Wire.begin();
    isBegin = amg.begin();
	if(debug){
		Serial.print("AMG -> ");
		Serial.println(status);
	}

    if (!status) { return false; }
    return isBegin;
}

void AMG::conf(double value = -1){
}


void AMG::read(double* val){
	amg.readPixels(pixels);
	val[AMG::dataType::status] = AMG::statusCode::ok;
	val[AMG::dataType::thermistor] = amg.readThermistor();
}


void readTaskAMG(void *pvParameters){
	(void) pvParameters;
	AMG *amg = (AMG *)pvParameters;
	for (;;){

		// Calculate Data Frequency
		if(amg->lastMillis == 0){
			amg->lastMillis = millis();
		}else{
			unsigned long timePassed = millis() - amg->lastMillis;
			if(timePassed > 1000){
				amg->lastMillis = millis();
				if(amg->debug){
					Serial.print(" => AMG8833 Data Frequency ");
					Serial.print(amg->msgCount);
					Serial.println(" Hz");
				}
				amg->dataFrequency = amg->msgCount;
				amg->msgCount = 0;

			}else{
				amg->msgCount++;
			}
		}

		// Read Sensor
		amg->read(amg->data);

		// Calibration Offset
		for(int i=1; i<=AMG88xx_PIXEL_ARRAY_SIZE; i++){
			amg->pixels[i-1] = amg->pixels[i-1] + PIXEL_TEMP_OFFSET;
		}

		// DEBUG Pixels
		if(amg->debug && false){
			amg->printPixels();
		}

		// Mirror Columns
		if(0){ // Not Needed
			int row = 1;
			for(int rowIndex = 0; rowIndex < AMG88xx_PIXEL_ARRAY_SIZE; rowIndex = rowIndex + 8){
				for(int columnIndex = 0; columnIndex < 4; columnIndex++) {
					float temp = amg->pixels[rowIndex + columnIndex];
					amg->pixels[rowIndex + columnIndex] = amg->pixels[((8*row)-columnIndex)-1];
					amg->pixels[((8*row)-columnIndex)-1] = temp;
				}
				row++;
			}
		}

		// Mirror Rows
		if(1){
			// First 4 Row
			for(int rowIndex = 0; rowIndex < AMG88xx_PIXEL_ARRAY_SIZE / 2; rowIndex = rowIndex + 8){
				// Every Column in a row

				int diff = 0;
				if(rowIndex == 0){ diff = 56; }
				if(rowIndex == 8){ diff = 40; }
				if(rowIndex == 16){ diff = 24; }
				if(rowIndex == 24){ diff = 8; }

				for(int columnIndex = 0; columnIndex < 8; columnIndex++) {
					float temp = amg->pixels[rowIndex + columnIndex];
					amg->pixels[rowIndex + columnIndex] = amg->pixels[(rowIndex + columnIndex) + diff];
					amg->pixels[(rowIndex + columnIndex) + diff] = temp;
				}
			}
		}

		// DEBUG Pixels
		if(amg->debug && false){
			amg->printPixels();
		}

		// Max, Min, Average
		amg->data[AMG::dataType::maxTemp] = -1;
		amg->data[AMG::dataType::minTemp] = -1;
		double tAverageTemp = 0;

		for(int i = 1; i <= AMG88xx_PIXEL_ARRAY_SIZE; i++){

			tAverageTemp += amg->pixels[i-1];

			if(amg->data[AMG::dataType::maxTemp] == -1.00){
				amg->data[AMG::dataType::maxTemp] = amg->pixels[i-1];
			}

			if(amg->data[AMG::dataType::minTemp] == -1){
				amg->data[AMG::dataType::minTemp] = amg->pixels[i-1];
			}

			// Check highest temp
			if(amg->data[AMG::dataType::maxTemp] < amg->pixels[i-1]){
				amg->data[AMG::dataType::maxTemp] = amg->pixels[i-1];
			}

			// Check lowest temp
			if(amg->data[AMG::dataType::minTemp] > amg->pixels[i-1]){
				amg->data[AMG::dataType::minTemp] = amg->pixels[i-1];
			}
		}

		amg->data[AMG::dataType::averageTemp] = tAverageTemp / AMG88xx_PIXEL_ARRAY_SIZE;

		// Interpolate
		int32_t t = millis();
		amg->interpolate_image(amg->pixels, AMG_ROWS, AMG_COLS, amg->dest_2d, INTERPOLATED_ROWS, INTERPOLATED_COLS);
		
		if(amg->debug){
			Serial.print("Interpolation took "); Serial.print(millis()-t); Serial.println(" ms");
		}

		// DEBUG Pixels
		if(amg->debug && false){
			amg->printInterpolatedPixels();
		}

		if(amg->debug){
			Serial.print("MAX TEMP -> ");
			Serial.println(amg->data[AMG::dataType::maxTemp]);
			Serial.print("MIN TEMP -> ");
			Serial.println(amg->data[AMG::dataType::minTemp]);
			Serial.println();
		}


	}
}

void AMG::start(){
    xTaskCreate(readTaskAMG, (const portCHAR *)"readTaskAMG", 512, this, 1, NULL);
}


void AMG::printPixels(){
	Serial.println("\n-> PIXELS");
	for(int i=1; i <= AMG88xx_PIXEL_ARRAY_SIZE; i++){
		Serial.print(pixels[i-1]);
		Serial.print(", ");
		if( i%8 == 0 ) Serial.println();
	}
	Serial.println();
	Serial.println();
}


void AMG::printInterpolatedPixels(){
	Serial.println("\n-> INTERPOLATED PIXELS");
	for(int i=1; i <= INTERPOLATED_ROWS * INTERPOLATED_COLS; i++){
		Serial.print(dest_2d[i-1]);
		Serial.print(", ");
		if( i%24 == 0 ) Serial.println();
	}
	Serial.println();
	Serial.println();
}



void AMG::calculatePixels(long cameraResolution, long x, long y, long w, long h){
	// Recalculate face pixels to amg pixel size
	int scaleFactor = cameraResolution / INTERPOLATED_ROWS;

	int scaledX = x / scaleFactor;
	int scaledY = y / scaleFactor;
	int scaledW = w / scaleFactor;
	int scaledH = h / scaleFactor;

	// Max, Min, Average
	calculatedData[AMG::dataType::maxTemp] = -1;
	calculatedData[AMG::dataType::minTemp] = -1;
	double tAverageTemp = 0;
	int howManyPixelMatch = 0;


	// Find Matched Pixels
	for (size_t i = 0; i < INTERPOLATED_COLS * INTERPOLATED_ROWS; i++){
		// Find Pixel Row and Column (both starts from 1)
		// ex: topLeftPixel => {1,1} - bottomRightPixel => {48,48}
		int row = int(i / INTERPOLATED_ROWS) + 1;
		int column = (i % INTERPOLATED_ROWS) + 1;

		// Is AMG pixel inside in face area
		// Check Column First
		if(column >= scaledX && column <= (scaledX + scaledW)){
			if(row >= scaledY && row <= (scaledY + scaledH)){
				howManyPixelMatch++;

				// Find Pixel Index
				int pixelIndex = (row - 1) * 8 + (column - 1);
				
				tAverageTemp += dest_2d[pixelIndex];

				if(calculatedData[AMG::dataType::maxTemp] == -1.00){
					calculatedData[AMG::dataType::maxTemp] = dest_2d[pixelIndex];
				}

				if(calculatedData[AMG::dataType::minTemp] == -1){
					calculatedData[AMG::dataType::minTemp] = dest_2d[pixelIndex];
				}

				// Check highest temp
				if(calculatedData[AMG::dataType::maxTemp] < dest_2d[pixelIndex]){
					calculatedData[AMG::dataType::maxTemp] = dest_2d[pixelIndex];
				}

				// Check lowest temp
				if(calculatedData[AMG::dataType::minTemp] > dest_2d[pixelIndex]){
					calculatedData[AMG::dataType::minTemp] = dest_2d[pixelIndex];
				}
			}
		}
	}

	calculatedData[AMG::dataType::averageTemp] = tAverageTemp / howManyPixelMatch;

	if(false){
		Serial.print("SCALE FACTOR: ");
		Serial.print(scaleFactor);
		Serial.print("\n");
		Serial.print("INCOMING: ");
		Serial.print(x);
		Serial.print(" ");
		Serial.print(y);
		Serial.print(" ");
		Serial.print(w);
		Serial.print(" ");
		Serial.print(h);
		Serial.println(" ");

		Serial.print("SCALED: ");
		Serial.print(scaledX);
		Serial.print(" ");
		Serial.print(scaledY);
		Serial.print(" ");
		Serial.print(scaledW);
		Serial.print(" ");
		Serial.print(scaledH);
		Serial.println(" ");

		Serial.print("PIXEL MATCH: ");
		Serial.println(howManyPixelMatch);
		Serial.print("TOTAL PIXEL: ");
		Serial.println((INTERPOLATED_COLS * INTERPOLATED_ROWS));
		Serial.print("PIXEL MATCH PERCENT: ");
		Serial.println((100 * howManyPixelMatch) / (INTERPOLATED_COLS * INTERPOLATED_ROWS));
	}
	

}


// HELPER

float AMG::get_point(float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y) {
    if (x < 0)
    	x = 0;
    if (y < 0)
    	y = 0;
  	if (x >= cols)
    	x = cols - 1;
  	if (y >= rows)
    	y = rows - 1;
  	return p[y * cols + x];
}

void AMG::set_point(float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y, float f) {
  	if ((x < 0) || (x >= cols))
    	return;
  	if ((y < 0) || (y >= rows))
    	return;
  	p[y * cols + x] = f;
}

// src is a grid src_rows * src_cols
// dest is a pre-allocated grid, dest_rows*dest_cols
void AMG::interpolate_image(float *src, uint8_t src_rows, uint8_t src_cols, float *dest, uint8_t dest_rows, uint8_t dest_cols) {
	float mu_x = (src_cols - 1.0) / (dest_cols - 1.0);
	float mu_y = (src_rows - 1.0) / (dest_rows - 1.0);

  	float adj_2d[16]; // matrix for storing adjacents

  	for (uint8_t y_idx = 0; y_idx < dest_rows; y_idx++) {
    	for (uint8_t x_idx = 0; x_idx < dest_cols; x_idx++) {
      		float x = x_idx * mu_x;
      		float y = y_idx * mu_y;
			// Serial.print("("); Serial.print(y_idx); Serial.print(", ");
			// Serial.print(x_idx); Serial.print(") = "); Serial.print("(");
			// Serial.print(y); Serial.print(", "); Serial.print(x); Serial.print(") =
			// ");
      		get_adjacents_2d(src, adj_2d, src_rows, src_cols, x, y);
			/*
			Serial.print("[");
			for (uint8_t i=0; i<16; i++) {
				Serial.print(adj_2d[i]); Serial.print(", ");
			}
			Serial.println("]");
			*/
			float frac_x = x - (int)x; // we only need the ~delta~ between the points
			float frac_y = y - (int)y; // we only need the ~delta~ between the points
			float out = bicubicInterpolate(adj_2d, frac_x, frac_y);
			// Serial.print("\tInterp: "); Serial.println(out);
			set_point(dest, dest_rows, dest_cols, x_idx, y_idx, out);
    	}
  	}
}

// p is a list of 4 points, 2 to the left, 2 to the right
float AMG::cubicInterpolate(float p[], float x) {
  	float r = p[1] + (0.5 * x *
                    (p[2] - p[0] +
					x * (2.0 * p[0] - 5.0 * p[1] + 4.0 * p[2] - p[3] +
					x * (3.0 * (p[1] - p[2]) + p[3] - p[0]))));
	/*
	Serial.print("interpolating: [");
	Serial.print(p[0],2); Serial.print(", ");
	Serial.print(p[1],2); Serial.print(", ");
	Serial.print(p[2],2); Serial.print(", ");
	Serial.print(p[3],2); Serial.print("] w/"); Serial.print(x); Serial.print("
	= "); Serial.println(r);
	*/
  	return r;
}

// p is a 16-point 4x4 array of the 2 rows & columns left/right/above/below
float AMG::bicubicInterpolate(float p[], float x, float y) {
	float arr[4] = {0, 0, 0, 0};
	arr[0] = cubicInterpolate(p + 0, x);
	arr[1] = cubicInterpolate(p + 4, x);
	arr[2] = cubicInterpolate(p + 8, x);
	arr[3] = cubicInterpolate(p + 12, x);
	return cubicInterpolate(arr, y);
}

// src is rows*cols and dest is a 4-point array passed in already allocated!
void AMG::get_adjacents_1d(float *src, float *dest, uint8_t rows, uint8_t cols, int8_t x, int8_t y) {
	// Serial.print("("); Serial.print(x); Serial.print(", "); Serial.print(y);
	// Serial.println(")");
	// pick two items to the left
	dest[0] = get_point(src, rows, cols, x - 1, y);
	dest[1] = get_point(src, rows, cols, x, y);
	// pick two items to the right
	dest[2] = get_point(src, rows, cols, x + 1, y);
	dest[3] = get_point(src, rows, cols, x + 2, y);
}

// src is rows*cols and dest is a 16-point array passed in already allocated!
void AMG::get_adjacents_2d(float *src, float *dest, uint8_t rows, uint8_t cols, int8_t x, int8_t y) {
	// Serial.print("("); Serial.print(x); Serial.print(", "); Serial.print(y);
	// Serial.println(")");
	float arr[4];
  	for (int8_t delta_y = -1; delta_y < 3; delta_y++) { // -1, 0, 1, 2
    	float *row = dest + 4 * (delta_y + 1); // index into each chunk of 4
    	for (int8_t delta_x = -1; delta_x < 3; delta_x++) { // -1, 0, 1, 2
      		row[delta_x + 1] = get_point(src, rows, cols, x + delta_x, y + delta_y);
    	}
  	}
}