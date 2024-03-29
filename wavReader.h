//include guard to prevent wavReader.h being included twice
#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
class wavReader {
private:
    std::string fileName;

    std::ifstream fin;
    std::ofstream fout;
    bool isOpen;

    char* buffer;
    char* outBuffer;
    int16_t* intReader;

    int numSamples20ms;

    int fileLengthBytes;
    int sampleNum;
    int channelLength;
    std::int16_t channels;
    std::int32_t sampleRate;
    std::int16_t sampleBits;
    float int16toFloat(int16_t input);
    void pathToFileName(std::string path);
public:
    wavReader();
    ~wavReader();
    bool open(std::string filename); //best practice is to use an enum instead of a bool, but we'll just treat all successes as true
    bool openSpecific(std::string FileAddress);
    std::int16_t sequentialBitRead16();//scale shows how many to skip
    std::int16_t skippingBitRead16();
    std::int16_t bitRead16();
    void incrementReader16();
    void resetRead();
    int getAverage();
    std::vector<std::int16_t> dataToVector();
    bool writeBuffer(std::vector<int16_t>&vectorIn);

    int getSampleNum();
    int32_t getSampleRate();
    int16_t getChannels();
    std::string getFileName();
    int getChannelLength();
    bool is_open();
    int getSampleNum_ms(int ms);
};
namespace vectorStuff {
    void floatData(int16_t* start,float* fstart, long unsigned int length);
    std::vector<std::int16_t> resampleToSize(std::vector<std::int16_t> vectorIn, long unsigned int sizeAim);
    std::vector<float> resampleFloat(std::vector<float> vectorIn, long unsigned int sizeAim);

    float totalHarmonicDistortion(std::vector<float> vectorIn);
};
