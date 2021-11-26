#include "SOLA.h"

void SOLA::overlap(float* output, float* previous, float* current) {
	for (int i = 0; i < overlapSize; i++) {
		//Computes overlap linearly between the two segments
		//previous starts at strength of 1, decays to 0
		//current starts at 0, increases to 1
		output[i] = (previous[i] * (overlapSize - i) + current[i] * i)/overlapSize; 
	}
}

int SOLA::seekOverlap(float* previous, float* current) {
	//Uses cross-correlation to find the optimal overlapping offset within the window, least differences between signals.
	int optOffset = 0;							//Stores value of optimal offset
	float optCorrelation = -1;					//Need to initialise at negative in case crossCorrelation begins at 0
	float* temp = new float[overlapSize] {};	//Initialises new array to store values of previous slope vals

	//Creates new values of slope only for previous vals
	for (int i = 0; i < overlapSize; i++) {
		temp[i] = previous[i] * i * (overlapSize - i);
	}

	//find optimal overlap offset within window
	//iterates through possible places for overlap
	for (int i = 0; i < overlapSize; i++) {
		float thisCorrelation = 0;

		for (int j = 0; j < overlapSize; j++) {
			//makes value for comparison, greater values show greater correlation
			thisCorrelation += current[i + j] * temp[j];
		}
		if (thisCorrelation > optCorrelation) {
			//if greater, new best correlation found. optimal offset update to new optimum
			optCorrelation = thisCorrelation;
			optOffset = i;
		}

	}
	//Deallocates memory, returns offset
	delete[] temp;
	return optOffset;
}

std::vector<float> SOLA::sola() {

}

SOLA::SOLA(float l_time_scale,
	int l_sequenceSize,
	int l_overlapSize,
	int l_seekWindow,
	float* l_input, int l_inputLength) {
	time_scale = l_time_scale;
	sequenceSize = l_sequenceSize;
	seekWindow = l_seekWindow;
	input = l_input;
	inputLength = l_inputLength;
}