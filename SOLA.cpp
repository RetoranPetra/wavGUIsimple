#include "SOLA.h"

void SOLA::overlap(int16_t* output, int16_t* previous, int16_t* current) {
	for (int i = 0; i < overlapSize; i++) {
		//Computes overlap linearly between the two segments
		//previous starts at strength of 1, decays to 0
		//current starts at 0, increases to 1
		output[i] = (previous[i] * (overlapSize - i) + current[i] * i)/overlapSize; 
	}
}

int SOLA::seekOverlap(int16_t* previous, int16_t* current) {
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

void SOLA::sola() {
	int numSamplesOut = 0;
	int16_t* seq_offset = input;
	int16_t* prev_offset = nullptr;
	int16_t* l_input = input;
	int16_t* l_output = output;

	//Reduces over time, basically counts things left in input
	int numSamplesIn = inputLength;

	//Main loop, does SOLA stuff.
	while (numSamplesIn >processDistance+seekWindow) {
		//Copies flat to output vector
		memcpy(l_output, seq_offset, flatDuration * sizeof(int16_t)); //Need to use sizeof due to this being a legacy C function, doesn't do it automatically

		//Sets previous to end of current flat segment
		prev_offset = seq_offset + flatDuration;

		//input pointer update to new location of processing sequence
		l_input += (processDistance - overlapSize);

		//Updates sequence offset to the optimal place using seekOverlap
		seq_offset = l_input + seekOverlap(prev_offset, l_input);

		//Calls overlap to overlap new and previous sample
		overlap(l_output + flatDuration, prev_offset, seq_offset);

		//Update pointers by the total overlap value
		seq_offset += overlapSize;
		l_input += overlapSize;

		//Output pointer needs to be different, due to initial memcpy
		l_output += sequenceSize -overlapSize;

		//Update counters to match new values
		numSamplesOut += sequenceSize - overlapSize;
		numSamplesIn -= processDistance;
	}

}

SOLA::SOLA(float l_timeScale,
	int l_sequenceSize,
	int l_overlapSize,
	int l_seekWindow, 
	int16_t* l_input, int16_t* l_output,int l_inputLength) {
	timeScale = l_timeScale;
	sequenceSize = l_sequenceSize;
	overlapSize = l_overlapSize;
	seekWindow = l_seekWindow;
	input = l_input;
	inputLength = l_inputLength;
	output = l_output;
	


	flatDuration = (sequenceSize - 2 * overlapSize);
	processDistance = (int)((sequenceSize - overlapSize) * timeScale);
}