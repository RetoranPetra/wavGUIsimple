#include "SOLA.h"

//Combines overlaps
void SOLA::overlap(std::vector<int16_t>::iterator firstOverlap, std::vector<int16_t>::iterator secondOverlap, std::vector<int16_t>::iterator outputPosition) {
	//Iterators needed are the positions of the start of each overlap.
	for (int i = 0; i < overlapSize; i++) {
		float weighting = (float)(overlapSize - i) / (float)overlapSize;
		*(outputPosition + i) = *(firstOverlap + i) * weighting
			+ *(secondOverlap + i) * (1.0f - weighting);
	}
}

//Finds next index
int SOLA::seekWindowIndex(std::vector<int16_t>::iterator previous, std::vector<int16_t>::iterator current) {
	//previous is the 
	//Uses cross-correlation to find the optimal location for next segment, least difference between segments in overlap
	int optOffset = 0;							//Stores value of optimal offset
	double optCorrelation = -1;					//Need to initialise at negative in case crossCorrelation begins at 0, so doesn't get stuck on 0 and break loop in beginning.


	for (int i = 0; i < seekSize; i++) {

		double thisCorrelation = 0;
		double numerator = 0;
		double denominator1 = 0;
		double denominator2 = 0;

		/*
		//only checks for positive out of range, not negative.
		if (previous + overlapSize - 1 > baseInput + input.size() - 1 || current + overlapSize -1 > baseInput + input.size() - 1) {
			std::cout << "Reaching out of range. If crashes all is well.\n";
			return 1; //Meant to return negative value, positive 1 error code.
		}
		if (current - i < baseInput) { //If not enough space to move backwards, just use no correlation.
			std::cout << "Reaching out of range backwards. If crashes all is well.\n";
			return 0;
		}
		*/

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
	/*
	//Begins by copying flat from input to output.
	for (int i = 0; i < flatSize; i++) {
		*(outputLocation + i) = *(inputLocation + i);
	}
	//Moves both pointers forward by flat size, as that's what been written.
	inputLocation += flatSize;
	outputLocation += flatSize;
	//Makes note of samples written/read
	outputSamplesWritten += flatSize;
	inputSamplesRead += flatSize;
	*/

	//Loop stop is controlled by internal if statements.
	for (int j = 0;;j++) {
		std::cout << "Loop: " << j << "\n";

		//Checks if flat would go out of range, if so shortens amount of flat copied.
		int flatToCopy = flatSize;
		/*
		long int expectedInputLeft =  baseInput + input.size() - (inputLocation + flatToCopy + 1);
		long int expectedOutputLeft = baseOutput + internalBuffer.size() - (outputLocation + flatToCopy + 1);
		std::cout << "OutputLeft: " << expectedOutputLeft << "InputLeft: " << expectedInputLeft << "\n";
		if (expectedInputLeft <= 0 || expectedOutputLeft <= 0) {

		}*/

		if (outputSamplesWritten + flatSize > internalBuffer.size() || inputSamplesRead + flatSize > input.size()) {
			std::cout << "Writing flat would go over.\n";
			break;
		}
		if (inputSamplesRead + flatSize > input.size()) {
			std::cout << "Reading flat would go over.\n";
			break;
		}

		//Copies flat section
		for (int i = 0; i < flatToCopy; i++) {
			*(outputLocation + i) = *(inputLocation + i);
		}
		//Updates iterators to end of flat
		inputLocation += flatSize;
		outputLocation += flatSize;
		//Makes note of number of writes/reads
		inputSamplesRead += flatSize;
		outputSamplesWritten += flatSize;

		
		//Check if seekSize could check behind the start of the vector, if it is overlap without checking for better index.

		//Update previouswindow position to current inputlocation, and guess at where next window's position is.
		window = inputLocation; //Positioned at end of flat of first window in input
		nextWindow = inputLocation + nextWindowDistance - (windowSize - 2*overlapSize);


		if (outputSamplesWritten + overlapSize > internalBuffer.size() || inputSamplesRead + nextWindowDistance - (windowSize - 2 * overlapSize) > input.size()) {
			std::cout << "Writing/Reading overlap would go over.\n";
			break;
		}

		int foundOffset = 0;
		//Won't check for better window if better window search could search out of range backwards.
		if (inputSamplesRead - seekSize) {
			std::cout << "Actually searching for window.\n";
			foundOffset = seekWindowIndex(window, nextWindow);
		}
		nextWindow += foundOffset;

		//Maybe should increment by one more so overlaps don't share points with flatsize? not sure if correct.

		overlap(window, nextWindow, outputLocation);

		inputSamplesRead += (size_t)nextWindowDistance + (size_t)overlapSize + foundOffset;
		outputSamplesWritten += overlapSize;

		//Updates location in input to start of next flat.
		inputLocation = nextWindow + overlapSize;
		//Updates location in output to location of next flat
		outputLocation += overlapSize;
	}



	output = internalBuffer;
}

SOLA::SOLA(double l_timeScale, int l_windowSize, int l_overlapSize, int l_seekSize, std::vector<int16_t>& l_input, std::vector<int16_t>& l_output) : input(l_input),output(l_output){
	timeScale = l_timeScale;
	windowSize = l_windowSize;
	overlapSize = l_overlapSize;
	seekSize = l_seekSize;

	//Calculated values for pointer manipulation
	nextWindowDistance = (int)((windowSize - overlapSize) / timeScale);
	flatSize = (windowSize - (2 * overlapSize));

	//Needed so when returning to same location doesn't cause problems
	internalBuffer.resize((double)input.size() * timeScale);

	std::cout << "internalbuffer size" << internalBuffer.size();

	std::fill(internalBuffer.begin(), internalBuffer.end(), 0); //Fill with zeroes just to be sure.

	//Where the current window is
	nextWindow = input.begin();
	//Where the last window was
	window = input.begin(); //Needs to be initialised as nullpointer, can't be left uninitialised
	//Where to start the search from
	inputLocation = input.begin();
	baseInput = input.begin();
	//Where to put the windows when found.
	outputLocation = internalBuffer.begin();
	baseOutput = internalBuffer.begin();
	//std::cout << "timeScale" << timeScale << "windowSize" << windowSize << "overlapSize" << overlapSize << "seekSize" << seekSize << "nextWindowDistance" << nextWindowDistance << "\n";
}