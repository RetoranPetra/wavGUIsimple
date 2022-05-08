#pragma once
#include <vector>
#include <iostream>
#include <math.h>

struct SOLAdatum {
	float freqScale;
	int solaWindow;
	float solaOverlapPercentage;
	int solaOverlap;
	float solaSeekPercentage;
	int solaSeek;
	float THD;
	float expectedFreq;
	float measuredFreq;
	unsigned long int expectedOutLength;
	unsigned long int gotOutLength;
	unsigned long int inputSize;
	unsigned long int readInputSize;
	double solaInReadPercentage;
	double solaOutWritePercentage;
};


class SOLA {
private:

	SOLAdatum internalData;

	//Variables that define how the SOLA works
	float timeScale = 1.0;
	int windowSize;		//Size of window to process
	int overlapSize;	//Size of overlap between windows
	int seekSize;		//How far the algorithm will search to find the best overlap

	//Calculated on startup
	int flatSize;
	int nextWindowDistance;

	//Using iterators to control program. Be wary of undefined behaviour.

	//Pointers to data
	std::vector<int16_t>& input;
	std::vector<int16_t>& output;
	//internal data holder
	std::vector<int16_t> internalBuffer;

	//Iterators

	//Where the current window is
	std::vector<int16_t>::iterator nextWindow;
	//Where the last window was
	std::vector<int16_t>::iterator window; //Needs to be initialised as nullpointer, can't be left uninitialised
	//Where to start the search from
	std::vector<int16_t>::iterator inputLocation;
	std::vector<int16_t>::iterator baseInput;
	//Where to put the windows when found.
	std::vector<int16_t>::iterator outputLocation;
	std::vector<int16_t>::iterator baseOutput;


	//Sola can be implemented with ints, as it is simple addition for the most part, but our case uses floats as that's what wavReader outputs.
	int seekWindowIndex(std::vector<int16_t>::iterator previous, std::vector<int16_t>::iterator current);
	void overlap(std::vector<int16_t>::iterator firstOverlap, std::vector<int16_t>::iterator secondOverlap, std::vector<int16_t>::iterator outputPosition);
	bool checkValidity(int inStep, int outStep);
	void datumPass(SOLAdatum &data);
	void endSOLA();
public:
	SOLA(double l_timeScale, int l_windowSize, double overLapPercentage, double seekPercentage, std::vector<int16_t>& l_input, std::vector<int16_t>& l_output);
	void sola(void);
};