#include "SOLA.h"



//Return false when invalid.
//Checks validity of input/output location for a certain step.
bool SOLA::checkValidity(int inStep, int outStep) {

	if (inStep > 0) {
		if ((size_t)(inputLocation - baseInput) + (size_t)inStep >= input.size()) {
			std::cout << "Ran out of input\n";
			return false;
		}
	}
	else if ((inputLocation+inStep) <= baseInput) {
		std::cout << "Ran out of input\n";
		return false;
	}
	if (outStep > 0) {
		if ((size_t)(outputLocation - baseOutput) + (size_t)outStep >= internalBuffer.size()) {
			std::cout << "Ran out of output\n";
			return false;
		}
	}
	else if ((inputLocation + outStep) <= baseInput) {
		std::cout << "Ran out of output\n";
		return false;
	}
	
	return true;
}

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


	//Iterates through seekSize, finding best overlap.
	for (int i = -seekSize; i < seekSize; i++) {

		double thisCorrelation = 0;
		double numerator = 0;
		double denominator1 = 0;
		double denominator2 = 0;

		for (int j = 0; j < overlapSize; j++) {
			//Each of these is an individual sum that needs to be calculated seperately, not the same as looping through fraction all at once
			double x = (double)current[j + i];
			double y = (double)previous[j];

			numerator += x * y;
			denominator1 += x * x;
			denominator2 += y * y;
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
	//Loop stop is controlled by internal if statements.
	for (int j = 0;;j++) {
		std::cout << "Loop: " << j << "\n";
		std::cout << "OutputP remaining " << internalBuffer.size()-(outputLocation - baseOutput) << " InputP remaining: " << input.size()-(inputLocation - baseInput)<< "\n";

		//Start by writing flat to output.
		for (int i = 0; i < flatSize; i++) {
			if (checkValidity(1,1)) {
				*(outputLocation++) = *(inputLocation++);
			}
			else {
				std::cout << "Ended in flat write.\n";
				endSOLA();
				return;
			}
		}
		//Pointers moved along as needed, now in location of start of overlap for both windows.
		window = inputLocation;
		
		//Estimates position of next window
		//Need to account for size of flat and overlap, so only size of gap not distance between start/stop of windows
		nextWindow = window + nextWindowDistance - (flatSize + overlapSize);

		int foundOffset = 0;
		
		//If number of samples read is less than seek size, search could go out of vector range backwards, so check backwards validity.
		if (checkValidity(-seekSize, 0)) {
			//Checks if highset possible point valid.
			if (checkValidity(nextWindowDistance - flatSize + seekSize, 0)) {
				foundOffset = seekWindowIndex(window, nextWindow);
			}
			else {
				std::cout << "Ended in overlap seek";
				endSOLA();
				return;
			}
		}
		std::cout << "Offset Chosen: " << foundOffset << "\n";
		
		//InputSamplesread should be updated inside seekWindowIndex, but should be fine outside of it.
		nextWindow += foundOffset;


		//inputLocation updated to new position, nextwindow. Used so validity checks/datum still accurate.
		inputLocation = nextWindow;

		for (int i = 0; i < overlapSize; i++) {
			if (checkValidity(1,1)) {
				float weighting = (float)(overlapSize - i) / (float)overlapSize;
				*(outputLocation++) = *(window++) * weighting + *(inputLocation++) * (1.0f - weighting);
			}
			else {
				std::cout << "Ended in overlap write";
				endSOLA();
				return;
			}
		}
	}
}

void SOLA::endSOLA() {
	output = internalBuffer;
	internalData.gotOutLength = outputLocation - baseOutput;
	internalData.readInputSize = inputLocation - baseInput;

	internalData.solaInReadPercentage = (double)internalData.readInputSize / (double)internalData.inputSize;
	internalData.solaOutWritePercentage = (double)internalData.gotOutLength / (double)internalData.expectedOutLength;
}

//Percentages should be between 1 and 0.
SOLA::SOLA(double l_timeScale, int l_windowSize, double overLapPercentage, double seekPercentage, std::vector<int16_t>& l_input, std::vector<int16_t>& l_output) : input(l_input),output(l_output){
	internalData = SOLAdatum();
	
	
	timeScale = l_timeScale;
	windowSize = l_windowSize;

	//Calculated based on percentage so isn't out of range.
	overlapSize = (int)((double)windowSize/2.0*overLapPercentage);

	//Calculated values for pointer manipulation
	//Distance between start of one window and start of another window in input.
	nextWindowDistance = (int)((windowSize - overlapSize) / timeScale);
	flatSize = (windowSize - (2 * overlapSize));

	//Calculated based on percentage so doesn't go out of range.
	//Actual length seeked is double this, however due to old code easier to just change like this.
	seekSize = (int)((double)nextWindowDistance * seekPercentage);

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

	//Update datum values for data-out

	internalData.freqScale = timeScale;//Can say are same as they are most of the time
	internalData.solaWindow = windowSize;
	internalData.solaOverlapPercentage = overLapPercentage;
	internalData.solaOverlap = overlapSize;
	internalData.solaSeekPercentage = seekPercentage;
	internalData.solaSeek = seekSize;
	internalData.inputSize = input.size();
	internalData.expectedOutLength = internalBuffer.size();
}

void SOLA::datumPass(SOLAdatum& data) {
	data = internalData;
}