#include "SOLA.h"

int SOLA::seekWindowIndex(std::vector<int16_t>::iterator previous, std::vector<int16_t>::iterator current) {
	//Uses cross-correlation to find the optimal location for next segment, least difference between segments in overlap
	int optOffset = 0;							//Stores value of optimal offset
	double optCorrelation = -1;					//Need to initialise at negative in case crossCorrelation begins at 0, so doesn't get stuck on 0 and break loop in beginning.


	for (int i = 0; i < seekWindow; i++) {

		double thisCorrelation = 0;
		double numerator = 0;
		double denominator1 = 0;
		double denominator2 = 0;


		//only checks for positive out of range, not negative.
		if (previous + overlapSize - 1 > baseInput + input.size() - 1 || current + overlapSize -1 > baseInput + input.size() - 1) {
			std::cout << "Reaching out of range. If crashes all is well.\n";
			return 1; //Meant to return negative value, positive 1 error code.
		}
		if (current - i < baseInput) { //If not enough space to move backwards, just use no correlation.
			std::cout << "Reaching out of range backwards. If crashes all is well.\n";
			return 0;
		}

		for (int j = 0; j < overlapSize; j++) {
			//Each of these is an individual sum that needs to be calculated seperately, not the same as looping through fraction all at once
			double x = (double)current[j - i];
			double y = (double)previous[j];

			numerator += x * y;
			denominator1 += x * x;
			denominator2 += y * y;
		}
		thisCorrelation = numerator / sqrt(denominator1 * denominator2); //Normalised correlation
		if (thisCorrelation > optCorrelation) {
			//if greater, new best correlation found. optimal offset update to new optimum
			optCorrelation = thisCorrelation;
			optOffset = -i;
		}

	}
	return optOffset; //negative offset, not positive
}

void SOLA::sola() {
	//Validity checks
	if (windowSize <= overlapSize * 2) {
		std::cout << "Error in setup, overlap too high\n";
		return;
	}

	
	if (nextWindowDistance - seekWindow < 1) {
		seekWindow -= nextWindowDistance - seekWindow; //Estimation is DEFINITELY wrong. just returns old seekWindow size.
		std::cout << "Seek window too large, progression could come to a halt. Try " << (double)seekWindow/48000.0*1000.0 << "ms as seekWindow at 48kHz\n";
		return;
	}
	

	int loopCounter = 0;

	//Main Loop, works through sample to apply SOLA.
	while ((inputLocation+nextWindowDistance) < (baseInput + input.size() - 1)//Checks if input has room for more reads
		&& (outputLocation+windowSize) < (baseOutput+internalBuffer.size()-1)) {//Check if output has room for more writes
		//Copies flat to output vector

		std::cout << "Loop: " << loopCounter << "\n";
		loopCounter++;

		//Using memcpy for maximum performance

		//Sets previous to end of current flat segment
		prevWindowOffset = windowOffset + flatSize;

		//input pointer update to new location to seek the next sequence
		inputLocation += nextWindowDistance - overlapSize;

		for (unsigned int i = 0; i < flatSize; i++) {
			outputLocation[i] = windowOffset[i];		//Copies flat portion to output initially
		}

		//Updates sequence offset to the optimal place using seekOverlap
		int thisOffset = seekWindowIndex(prevWindowOffset, inputLocation);
		if (thisOffset == 1) {
			std::cout << "Just missed memory exception.\n";
			break;
		}
		windowOffset = inputLocation + thisOffset; //Seekoverlap just gives offset as number, need to add to current reader in input for location of sequence offset


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
	//Fills in remaining signal
	/*
	for (int i = 0; i < output.size()-(outputLocation - baseOutput); i++) {
		if ((inputLocation + i) < (baseInput + input.size() - 1)) {
			outputLocation[i] = inputLocation[i];
		}
		else {
			outputLocation[i] = 0.0f;
		}
	}
	*/
	//Instead chops off remaining signal
	internalBuffer.resize(outputLocation - baseOutput + 1);

	output = internalBuffer;
}

SOLA::SOLA(float l_timeScale, int l_windowSize, int l_overlapSize, int l_seekWindow, std::vector<int16_t>& l_input, std::vector<int16_t>& l_output) : input(l_input),output(l_output){
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

	//Where the current window is
	windowOffset = input.begin();
	//Where the last window was
	prevWindowOffset = input.begin(); //Needs to be initialised as nullpointer, can't be left uninitialised
	//Where to start the search from
	inputLocation = input.begin();
	baseInput = input.begin();
	//Where to put the windows when found.
	outputLocation = internalBuffer.begin();
	baseOutput = internalBuffer.begin();
	//std::cout << "timeScale" << timeScale << "windowSize" << windowSize << "overlapSize" << overlapSize << "seekWindow" << seekWindow << "nextWindowDistance" << nextWindowDistance << "\n";
}