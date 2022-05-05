#include "SOLA.h"

int SOLA::seekWindowIndex(int16_t* previous, int16_t* current) {
	//Uses cross-correlation to find the optimal location for next segment, least difference between segments in overlap
	int optOffset = 0;							//Stores value of optimal offset
	double optCorrelation = -1;					//Need to initialise at negative in case crossCorrelation begins at 0, so doesn't get stuck on 0 and break loop in beginning.

	//Finds segment that matches best with precalculated first segment overlap values.
	for (int i = 0; i < -seekWindow; i--) {
		double thisCorrelation = 0;
		double numerator = 0;
		double denominator1 = 0;
		double denominator2 = 0;

		for (int j = 0; j < overlapSize; j++) {
			//Each of these is an individual sum that needs to be calculated seperately, not the same as looping through fraction all at once
			numerator += (double)current[i + j] * (double)previous[j];
			denominator1 += (double)current[i + j] * (double)current[i + j];
			denominator2 += (double)previous[j] * (double)previous[j];
		}
		thisCorrelation = numerator / sqrt(denominator1 * denominator2); //Normalised correlation
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

	//Where the current window is
	int16_t* windowOffset = input.data();
	//Where the last window was
	int16_t* prevWindowOffset = nullptr; //Needs to be initialised as nullpointer, can't be left uninitialised
	//Where to start the search from
	int16_t* inputLocation = windowOffset;
	int16_t* baseInput = inputLocation;
	//Where to put the windows when found.
	int16_t* outputLocation = internalBuffer.data();
	int16_t* baseOutput = outputLocation;

	//Main Loop, works through sample to apply SOLA.
	for (int samplesRead = 0;//Number of samples processed
		nextWindowDistance < input.size() - 1 - samplesRead; //Checks if about to go out of range, if next loop will go out of range, stop.
		samplesRead += nextWindowDistance) { //increases samplesread by the estimated distance to next window
		//Copies flat to output vector

		//Using memcpy for maximum performance

		//Sets previous to end of current flat segment
		prevWindowOffset = windowOffset + flatSize;

		//input pointer update to new location to seek the next sequence
		inputLocation += nextWindowDistance - overlapSize;

		for (unsigned int i = 0; i < flatSize; i++) {
			outputLocation[i] = windowOffset[i];		//Copies flat portion to output initially
		}

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
	}
	for (int i = 0; i < output.size()-(outputLocation - baseOutput); i++) {
		if ((inputLocation + i) < (baseInput + input.size() - 1)) {
			outputLocation[i] = inputLocation[i];
		}
		else {
			outputLocation[i] = 0.0f;
		}
	}
	output = internalBuffer;
}

SOLA::SOLA(float l_timeScale, int l_windowSize, int l_overlapSize, int l_seekWindow, std::vector<int16_t>& inputLocation, std::vector<int16_t>& outputLocation) : input(inputLocation),output(outputLocation){
	timeScale = l_timeScale;
	windowSize = l_windowSize;
	overlapSize = l_overlapSize;
	seekWindow = l_seekWindow;

	//Calculated values for pointer manipulation
	nextWindowDistance = (int)((windowSize - overlapSize) / timeScale);
	flatSize = (windowSize - (2 * overlapSize));

	//Needed so when returning to same location doesn't cause problems
	internalBuffer.resize(input.size() * timeScale);
	std::fill(internalBuffer.begin(), internalBuffer.end(), 0); //Fill with zeroes just to be sure.

	//std::cout << "timeScale" << timeScale << "windowSize" << windowSize << "overlapSize" << overlapSize << "seekWindow" << seekWindow << "nextWindowDistance" << nextWindowDistance << "\n";
}