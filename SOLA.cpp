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
	//Uses cross-correlation to find the optimal location for next segment, least difference between segments in overlap
	int optOffset = 0;							//Stores value of optimal offset
	double optCorrelation = -1;					//Need to initialise at negative in case crossCorrelation begins at 0, so doesn't get stuck on 0 and break loop in beginning.
	double* temp = new double[overlapSize] {};	//Initialises new array to store values of previous slope vals

	//Calculates what overlap would be from the first segment, same no matter what second segment is, just combines when mixed at end.
	for (int i = 0; i < overlapSize; i++) {
		temp[i] = previous[i] * i * (overlapSize - i);
	}

	//Finds segment that matches best with precalculated first segment overlap values.
	for (int i = 0; i < -seekWindow; i--) {
		double thisCorrelation = 0;

		for (int j = 0; j < overlapSize; j++) {
			//Makes sum of correlation of all points between 
			thisCorrelation += ((double)current[i + j] * temp[j])/sqrt((double)current[i + j] * temp[j] * (double)current[i + j] * temp[j]); //Normalised correlation, better than correlation
		}
		if (thisCorrelation > optCorrelation) {
			//if greater, new best correlation found. optimal offset update to new optimum
			optCorrelation = thisCorrelation;
			optOffset = i;
		}

	}
	//Deallocates memory, returns offset
	delete[] temp;
	return optOffset; //negative offset, not positive
}

void SOLA::sola() {

	//use local versions of variables so same SOLA object can be reused, otherwise would need to create new object for every SOLA operation.
	int numSamplesOut = 0;
	int16_t* seq_offset = input.data();
	int16_t* prev_offset = nullptr; //Needs to be initialised as nullpointer, can't be left uninitialised
	int16_t* l_input = input.data();
	int16_t* l_output = output.data();

	//Reduces over time, basically counts things left in input
	int numSamplesIn = inputLength-1; //Needs to be -1 to count 0

	//Main Loop, works through sample to apply SOLA.
	while (numSamplesIn > processDistance+seekWindow) {
		//Copies flat to output vector

		//Using memcpy for maximum performance
		memcpy(l_output, seq_offset, flatDuration * sizeof(int16_t)); //Need to use sizeof due to this being a legacy C function, doesn't do it automatically

		//Sets previous to end of current flat segment
		prev_offset = seq_offset + flatDuration;

		//input pointer update to new location to seek the next sequence
		l_input += processDistance - overlapSize;

		//Updates sequence offset to the optimal place using seekOverlap
		seq_offset = l_input + seekOverlap(prev_offset, l_input); //Seekoverlap just gives offset as number, need to add to current reader in input for location of sequence offset

		//Calls overlap to overlap new and previous sample
		overlap(l_output + flatDuration, //+flatDuration so inputs data at end of last flat in output
			prev_offset,
			seq_offset);

		//Add overlap to pointers now that overlap has been added
		seq_offset += overlapSize;
		l_input += overlapSize;

		//Update position in output to new location, 
		l_output += sequenceSize - overlapSize;

		//Update counters to match new values
		numSamplesOut += sequenceSize - overlapSize;
		//Processed this amount from input, now do output.
		numSamplesIn -= processDistance;
	}
	//output.resize((int)(l_input - &input[0]) - output.size());

	//std::cout << "Hello world!";

	std::cout << "Difference in addresses" << (long unsigned int)(l_input - &input[0]) << "\n";

	output.resize((int)(l_input - input.data()));
}

SOLA::SOLA(float l_timeScale, int l_sequenceSize, int l_overlapSize, int l_seekWindow, std::vector<int16_t>& l_input, std::vector<int16_t>& l_output) : input(l_input),output(l_output){
	timeScale = l_timeScale;
	sequenceSize = l_sequenceSize;
	overlapSize = l_overlapSize;
	seekWindow = l_seekWindow;
	inputLength = input.size();
	


	flatDuration = (sequenceSize - (2 * overlapSize));
	processDistance = (int)((sequenceSize - overlapSize) / timeScale);
}