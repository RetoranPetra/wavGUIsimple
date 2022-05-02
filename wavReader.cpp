#include "wavReader.h"
using string = std::string;
using ifstream = std::ifstream;
using std::cout; //using cout=std::cout causes problems with << operator
using ios = std::ios;
using std::vector;
//Standard constructor
wavReader::wavReader() {
    isOpen = false;
}
//Deconstructor to destroy pointer array
wavReader::~wavReader() {
    delete[] buffer;
}
float wavReader::int16toFloat(int16_t input) {
    return (float)input / (float)(pow(2, 15) - 1);
}
void wavReader::pathToFileName(string path) {
    std::size_t start = path.find_last_of('\\') + 1;
    std::size_t finish = path.find_last_of('.');
    fileName = path.substr(start, finish - start);
}
//Opens file and initiates some things
bool wavReader::open(string filename) {//Returns true on success, false on failure.
    fileName = filename;
    fin.open(fileName + ".wav", ios::binary);
    if (fin.is_open()) {
        cout << fileName + ".wav has been opened successfully\n";
        //file length info
        fin.seekg(0, ios::end);
        fileLengthBytes = fin.tellg();
        sampleNum = (fileLengthBytes - 44) / 2;//In bytes
        fin.seekg(0, ios::beg);

        //buffer creation and filling
        buffer = new char[fileLengthBytes];
        cout << "Reading " << fileLengthBytes << " bytes...\n";
        fin.read(buffer, fileLengthBytes);
        if (fin) { cout << "Read Success!\n"; }
        else { cout << "Read Failure\n"; return false; }

        fin.close();

        //Extracting basic variables from wav
        intReader = reinterpret_cast<int16_t*>(buffer);
        channels = intReader[11];
        sampleRate = *reinterpret_cast<int32_t*>(&intReader[12]);
        sampleBits = intReader[17];
        intReader = &intReader[22];//Sets position to start of data
        //Output important info
        cout << "Channels: " << channels << "\nSamplerate: " << sampleRate << "\nSamplebits: " << sampleBits << "\nSamplenum: " << sampleNum << "\n";
        channelLength = sampleNum / channels;
        numSamples20ms = sampleRate/50;
        //Set read flags
        isOpen = true;
        return true;
    }
    else { cout << fileName + ".wav has failed to open, retry\n"; return false; }
}
bool wavReader::openSpecific(string FileAddress) {
    fin.open(FileAddress, ios::binary);
    if (fin.is_open()) {
        cout << fileName + ".wav has been opened successfully\n";
        pathToFileName(FileAddress);
        //file length info
        fin.seekg(0, ios::end);
        fileLengthBytes = fin.tellg();
        sampleNum = (fileLengthBytes - 44) / 2;
        fin.seekg(0, ios::beg);

        //buffer creation and filling
        buffer = new char[fileLengthBytes];
        cout << "Reading " << fileLengthBytes << " bytes...\n";
        fin.read(buffer, fileLengthBytes);
        if (fin) { cout << "Read Success!\n"; }
        else { cout << "Read Failure\n"; return false; }

        fin.close();

        //Extracting basic variables from wav
        intReader = reinterpret_cast<int16_t*>(buffer);
        channels = intReader[11];
        sampleRate = *reinterpret_cast<int32_t*>(&intReader[12]);
        sampleBits = intReader[17];
        intReader = &intReader[22];//Sets position to start of data
        //Output important info
        cout << "Channels: " << channels << "\nSamplerate: " << sampleRate << "\nSamplebits: " << sampleBits << "\nSamplenum: " << sampleNum << "\n";
        channelLength = sampleNum / channels;
        numSamples20ms = sampleRate / 50;
        //Set read flags
        isOpen = true;
        return true;
    }
    else { cout << fileName + ".wav has failed to open, retry\n"; return false; }
}

bool wavReader::writeBuffer(std::vector<int16_t>& vectorIn) {
    fout.open("out.wav", ios::binary);
    if (fout.is_open()) {
        cout << fileName + ".wav has been opened successfully\n";
        //buffer creation and filling
        outBuffer = new char[fileLengthBytes];
        //std::copy(buffer, &buffer[fileLengthBytes], fileLengthBytes);
        memcpy(outBuffer, buffer, fileLengthBytes);
        //cout << "Vectorin Size: " << vectorIn.size() << "\n";
        //copys over values from vectorin, needs to seperate into 8 byte chunks to be used in the char input.
        //Cheating by not doing the header properly and instead copying it from read file. Means file can only be same length always.
        for (int i = 0; i < fileLengthBytes/2-44; i++) {
            int16_t copy;
            int8_t top;
            int8_t bottom;
            if (i >= vectorIn.size() - 1) { copy = 0; }
            else { copy = vectorIn[i]; }
            bottom = copy & 0x00FF;
            top = copy >> 8;
            outBuffer[44 + i * 2] = *reinterpret_cast<char*>(&bottom);
            outBuffer[45 + i * 2] = *reinterpret_cast<char*>(&top);
        }

        //Debug code
        /*
        for (int i = 3000; i < 3100; i++) {
            cout << *reinterpret_cast<int16_t*>(&outBuffer[44+i*2]) << " " << *reinterpret_cast<int16_t*>(&buffer[44+i * 2]) << " " << vectorIn[i] << "\n";
        }
        */

        cout << "Writing " << fileLengthBytes << " bytes...\n";
        fout.write(outBuffer, fileLengthBytes);
        fout.close();
        delete[] outBuffer;
        if (fout) { cout << "Write Success!\n"; }
        else { cout << "Write Failure!\n"; return false; }

        return true;
    }
    else { cout << fileName + ".wav has failed to open, retry\n"; return false; }
}
//reads current 2 bytes then moves 2 bytes forwards, also converts to float instead of int
int16_t wavReader::sequentialBitRead16() {
    int16_t temp = *intReader;
    intReader++;
    return temp;
}
//Used for getting less data for plots to reduce risk of memory problems
int16_t wavReader::skippingBitRead16() {
    int16_t temp = *intReader;
    intReader++;
    return temp;
}
//Reads without moving forward
int16_t wavReader::bitRead16() {
    return *intReader;
}
//Increments without reading
void wavReader::incrementReader16() {
    intReader++;
}
//Resets reader pointer back to start of data
void wavReader::resetRead() {
    intReader = reinterpret_cast<int16_t*>(buffer) + 22;
}
//Makes average, used for checking validity
int wavReader::getAverage() {
    int sum = 0;
    for (int i = 0; i < sampleNum; i++) {
        int16_t currentVal = *intReader;
        intReader++;
        sum += abs(currentVal);
    }
    return sum / sampleNum;
}
//Send values to vector
vector<int16_t> wavReader::dataToVector() {
    if (!isOpen) { return vector<int16_t>(); }//maybe replace with exception
    resetRead();
    vector<int16_t> temp;
    for (int i = 0; i < sampleNum; i++) {
        temp.push_back(sequentialBitRead16());
    }
    resetRead();
    return temp;
}
//Bunch of gets for private portions
int wavReader::getSampleNum() {
    return sampleNum;
}
int32_t wavReader::getSampleRate() {
    return sampleRate;
}
int16_t wavReader::getChannels()
{
    return channels;
}
string wavReader::getFileName() {
    return fileName;
}
int wavReader::getChannelLength() {
    return channelLength;
}
bool wavReader::is_open() {
    return isOpen;
}
int wavReader::getSampleNum_ms(int ms) {
    return (int)(sampleRate * ms/1000);
}

namespace vectorStuff {

//#define average

//Averaging version of function evens out outliers, at expense of computing time. Looks odd though and not representative of the wave, even if more accurate at a small scale.
    void floatData(int16_t* start, float* fstart, int length) {
        for (int i = 0; i < length; i++) {
            fstart[i] = (float)start[i] / pow(2, 15);
        }
    }

    std::vector<std::int16_t> resampleToSize(std::vector<std::int16_t> vectorIn, int sizeAim) {
        std::vector<std::int16_t> vectorOut(sizeAim);
        vectorIn.push_back(0);//Adds extra bit at end to copy to prevent checking out of vector bounds
        float ratio = (float)vectorIn.size() / (float)sizeAim;
        cout << "Ratio: " << ratio << "\n";
        double sum = 0.0;
        for (int i = 0; i < sizeAim; i++) {
            sum += ratio;
            //cout << "sum: " << sum << "\n";
            int temp = (int)sum;
            vectorOut[i] = (int)((double)(vectorIn[temp+1]-vectorIn[temp])*(sum-(double)temp))+vectorIn[temp];
        }
        cout << "Finished loop without crash\n";
        return vectorOut;
    }

    std::vector<float> resampleFloat(std::vector<float> vectorIn, int sizeAim) {
        std::vector<float> vectorOut(sizeAim);
        vectorIn.push_back(0);//Adds extra bit at end to copy to prevent checking out of vector bounds
        float ratio = (float)vectorIn.size() / (float)sizeAim;
        cout << "Ratio: " << ratio << "\n";
        double sum = 0.0;
        for (int i = 0; i < sizeAim; i++) {
            sum += ratio;
            //cout << "sum: " << sum << "\n";
            int temp = (int)sum;
            vectorOut[i] = (float)((double)(vectorIn[temp + 1] - vectorIn[temp]) * (sum - (double)temp)) + vectorIn[temp];
        }
        cout << "Finished loop without crash\n";
        return vectorOut;
    }

    float totalHarmonicDistortion(std::vector<float> vectorIn) {
        //Using squares, aka power as is traditional. Amplifies magnitude difference between noise and fundamental.


        double highest = 0;
        int highestIndex = 0;
        double sum = 0;
        for (int i = 0; i < vectorIn.size()/2; i++) {
            double current = (double)vectorIn[i] * (double)vectorIn[i];
            if (current > highest) {
                highest = current;
                highestIndex = i;
            }
            sum += current;
        }
        sum -= highest; //Remove highest from summation

        return sum/highest; //As it's sampling both sides of fourier domain, there are two maximums for one frequency. 
    }


    //Depreciated functions

/*
#ifdef average
    std::vector<std::int16_t> shrinkData(std::vector<std::int16_t> vectorIn, int maxSize) {
        int sum = 0;
        int absSum = 0;
        int size = vectorIn.size();
        int skip = 1;
        for (int x = size; x > maxSize; x = size / skip) {
            skip++;
        }
        //To be removed
        cout << "Skip decided as" << skip << "\n";
        std::vector<std::int16_t> vectorOut;
        for (int i = 0; i < size; i++) {
            sum += vectorIn[i];
            absSum += abs(vectorIn[i]);
            if (i % (skip) == 0) {
                if (sum > 0) {
                    vectorOut.push_back((int16_t)((absSum) / (skip)));
                }
                else {
                    vectorOut.push_back((int16_t)((-absSum) / (skip)));
                }
                sum = 0;
                absSum = 0;
            }
        }
        return vectorOut;
}
#else
    std::vector<std::int16_t> shrinkData(std::vector<std::int16_t> vectorIn, int maxSize) {
        int size = vectorIn.size();
        int skip = 1;
        for (int x = size; x > maxSize; x = size / skip) {
            skip++;
        }
        std::vector<std::int16_t> vectorOut;
        for (int i = 0; i < size; i++) {
            if (i % (skip) == 0) {
                vectorOut.push_back(vectorIn[i]);
            }
        }
        return vectorOut;
}
#endif
*/
}