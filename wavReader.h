//include guard to prevent wavReader.h being included twice
#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
class wavReader {
private:
    std::string fileName;

    std::ifstream fin;
    bool isOpen;

    char* buffer;
    int16_t* intReader;

    int fileLengthBytes;
    int sampleNum;
    std::int16_t channels;
    std::int32_t sampleRate;
    std::int16_t sampleBits;
    float int16toFloat(int16_t input);
public:
    wavReader(std::string filename);
    ~wavReader();
    bool open(); //best practice is to use an enum instead of a bool, but we'll just treat all successes as true
    float sequentialBitRead16();//scale shows how many to skip
    float skippingBitRead16(unsigned short offset);
    float bitRead16();
    void incrementReader16();
    void resetRead();
    double getAverage();
    std::vector<float> dataToVector();
    int getSampleNum();
    int32_t getSampleRate();
    int16_t getChannels();
};
