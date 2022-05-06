#pragma once
#include <vector>
#include <iostream>
#include <math.h>
class SOLA {
private:
	//Variables that define how the SOLA works
	float timeScale = 1.0;
	int windowSize;		//Size of window to process
	int overlapSize;	//Size of overlap between windows
	int seekSize;		//How far the algorithm will search to find the best overlap

	//Calculated on startup
	int flatSize;
	int nextWindowDistance;

	//Counters for stop conditions
	//Tried using iterator comparisons to control program, but resulted in problems due to undefined behaviour in some cases.
	//Stored as size_t to match bounds of regular vector, used to check if writes/reads are feasible.
	size_t inputSamplesRead = 0; //also counts samples skipped past
	size_t outputSamplesWritten = 0;



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
public:
	SOLA(double l_timeScale, int l_windowSize, int l_overlapSize, int l_seekWindow, std::vector<int16_t>& l_input, std::vector<int16_t>& l_output);
	void sola(void);
};