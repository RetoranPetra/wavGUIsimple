#include "wavReader.h"
using string = std::string;
using ifstream = std::ifstream;
using std::cout; //using cout=std::cout causes problems with << operator
using ios = std::ios;
using std::vector;
//Standard constructor
wavReader::wavReader(string filename) {
    fileName = filename;
    isOpen = false;
}
//Deconstructor to destroy pointer array
wavReader::~wavReader() {
    delete[] buffer;
}
float wavReader::int16toFloat(int16_t input) {
    return (float)input / (float)(pow(2, 15) - 1);
}
//Opens file and initiates some things
bool wavReader::open() {//Returns true on success, false on failure.
    fin.open(fileName + ".wav", ios::binary);
    if (fin.is_open()) {
        cout << fileName + ".wav has been opened successfully\n";
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
        intReader += 11;
        channels = *intReader;
        intReader++;
        sampleRate = *reinterpret_cast<int32_t*>(intReader);
        intReader += 5;
        sampleBits = *intReader;
        intReader += 5;//Sets position to start of data
        //Output important info
        cout << "Channels: " << channels << "\nSamplerate: " << sampleRate << "\nSamplebits: " << sampleBits << "\n";

        //Set read flags
        isOpen = true;
        return true;
    }
    else { cout << fileName + ".wav has failed to open, retry\n"; return false; }
}
//reads current 2 bytes then moves 2 bytes forwards, also converts to float instead of int
float wavReader::sequentialBitRead16() {
    int16_t temp = *intReader;
    intReader++;
    return int16toFloat(temp);
}
//Used for getting less data for plots to reduce risk of memory problems
float wavReader::skippingBitRead16(unsigned short offset) {
    int16_t temp = *intReader;
    intReader += offset;
    return int16toFloat(temp);
}
//Reads without moving forward
float wavReader::bitRead16() {
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
double wavReader::getAverage() {
    double sum = 0.0;
    for (int i = 0; i < sampleNum; i++) {
        float currentVal = int16toFloat(*intReader);
        intReader++;
        sum += fabs(currentVal);
    }
    return sum / (double)sampleNum;
}
//Send values to vector
vector<float> wavReader::dataToVector() {
    resetRead();
    vector<float> temp;
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