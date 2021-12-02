#pragma once
#include <vector>

class SOLA {
private:
	//Variables that define how the SOLA works
	float timeScale = 1.0;
	int sequenceSize;	//Size of sequence to process
	int overlapSize;	//Overlap of sequence
	int seekWindow;		//How far the algorithm will search to find the best overlap

	//Calculated on startup
	int flatDuration;
	int processDistance;

	//Pointers to data
	int16_t* input;
	int inputLength;
	int16_t* output;

	//Sola can be implemented with ints, as it is simple addition for the most part, but our case uses floats as that's what wavReader outputs.
	void overlap(int16_t* output, int16_t* previous, int16_t* current);
	int seekOverlap(int16_t* previous, int16_t* current);
public:
	SOLA(float l_timeScale, int l_sequenceSize, int l_overlapSize, int l_seekWindow, int16_t* l_input, int16_t* l_output, int l_inputLength);
	void sola(void);
};