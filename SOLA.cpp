#include "SOLA.h"

int SOLA::seekWindowIndex(int16_t* previous, int16_t* current) {
	//Uses cross-correlation to find the optimal location for next segment, least difference between segments in overlap
	int optOffset = 0;							//Stores value of optimal offset
	double optCorrelation = -1;					//Need to initialise at negative in case crossCorrelation begins at 0, so doesn't get stuck on 0 and break loop in beginning.

	//Finds segment that matches best with precalculated first segment overlap values.
	for (int i = 0; i < -seekWindow; i--) {
		double thisCorrelation = 0;

		for (int j = 0; j < overlapSize; j++) {
			//Makes sum of correlation of all points between 
			thisCorrelation += ((double)current[i + j] * previous[j])/sqrt((double)current[i + j] * previous[j] * (double)current[i + j] * previous[j]); //Normalised correlation, better than correlation
		}
		if (thisCorrelation > optCorrelation) {
			//if greater, new best correlation found. optimal offset update to new optimum
			optCorrelation = thisCorrelation;
			optOffset = i;
		}

	}
	return optOffset; //negative offset, not positive
}

void SOLA::sola() {

	//use local versions of variables so same SOLA object can be reused, otherwise would need to create new object for every SOLA operation.
	int numSamplesOut = 0;
	int16_t* windowOffset = input.data();
	int16_t* prevWindowOffset = nullptr; //Needs to be initialised as nullpointer, can't be left uninitialised
	int16_t* inputLocation = input.data();
	int16_t* outputLocation = output.data();

	int samplesRead = 0;

	//Main Loop, works through sample to apply SOLA.
	
	for (int samplesRead = 0; //Number of samples processed
		nextWindowDistance + seekWindow < input.size() - 1 - samplesRead; //Checks if about to go out of range, if next loop will go out of range, stop.
		samplesRead += nextWindowDistance) { //increases samplesread by the estimated distance to next window
		//Copies flat to output vector

		//Using memcpy for maximum performance

		for (unsigned int i = 0; i < flatSize; i++) {
			outputLocation[i] = windowOffset[i];		//Copies flat portion to output initially
		}

		//Sets previous to end of current flat segment
		prevWindowOffset = windowOffset + flatSize;

		//input pointer update to new location to seek the next sequence
		inputLocation += nextWindowDistance - overlapSize;

		//Updates sequence offset to the optimal place using seekOverlap
		windowOffset = inputLocation + seekWindowIndex(prevWindowOffset, inputLocation); //Seekoverlap just gives offset as number, need to add to current reader in input for location of sequence offset


		//Overlaps previous sample with new found sample
		for (int i = 0; i < overlapSize; i++) {
			//Computes overlap linearly between the two segments
			//previous starts at strength of 1, decays to 0
			//current starts at 0, increases to 1
			(outputLocation+flatSize)[i] = ((prevWindowOffset)[i] * (overlapSize - i) + (windowOffset)[i] * i) / overlapSize;
		}


		//Add overlap to pointers now that overlap has been added
		windowOffset += overlapSize;
		inputLocation += overlapSize;

		//Update position in output to new location, 
		outputLocation += windowSize - overlapSize;

		//Update counters to match new values
		numSamplesOut += windowSize - overlapSize; //Adds new sequence, but haven't done overlap at end of sequence yet so need to take away that portion.
	}
}

SOLA::SOLA(float l_timeScale, int l_windowSize, int l_overlapSize, int l_seekWindow, std::vector<int16_t>& inputLocation, std::vector<int16_t>& outputLocation) : input(inputLocation),output(outputLocation){
	timeScale = l_timeScale;
	windowSize = l_windowSize;
	overlapSize = l_overlapSize;
	seekWindow = l_seekWindow;

	//Calculated values for pointer manipulation
	nextWindowDistance = (int)((windowSize - overlapSize) / timeScale);
	flatSize = (windowSize - (2 * overlapSize));

	//std::cout << "timeScale" << timeScale << "windowSize" << windowSize << "overlapSize" << overlapSize << "seekWindow" << seekWindow << "nextWindowDistance" << nextWindowDistance << "\n";
}