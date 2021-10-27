#include "gnuPlotter.h"
using string = std::string;
using std::cout;
using std::to_string;
//Constructor
gnuPlotter::gnuPlotter(string name) : fileName(name),img(false){}
//Private, opens a dat file to write to.
bool gnuPlotter::openDat() {//Returns true on success, false on failure.
    if (plotGen.is_open()) { cout << "ofstream in use by .plt.\n"; return false; }
    plotGen.open(fileName + ".dat");
    if (plotGen) { cout << fileName + ".dat created!\n"; return true; }
    else { cout << fileName + ".dat creation failure.\n"; return false; }
}
//Generates dat file from wav, takes in wavReader class.
bool gnuPlotter::genDatFromWav(wavReader& wav, unsigned short scale) {//Wav must be open beforehand
    if (!openDat()) { return false; }
    if (scale < 1) { return false; }
    wav.resetRead();
    int sampleNum = wav.getSampleNum() / wav.getChannels();
    if (scale == 1) {
        for (int i = 0; i < sampleNum; i++) {
            float currentVal = wav.sequentialBitRead16();//only works for 16 bit, use 2^x-1 for x bits
            plotGen << to_string((double)i / (double)wav.getSampleRate()) + " " + to_string(currentVal) + "\n";
        }
    }
    else {
        sampleNum = sampleNum / scale;
        for (int i = 0; i < sampleNum; i++) {
            float currentVal = wav.skippingBitRead16(scale);//only works for 16 bit, use 2^x-1 for x bits
            plotGen << to_string((double)scale * (double)i / (double)wav.getSampleRate()) + " " + to_string(currentVal) + "\n"; //includes scale to keep time accurate
        }
    }
    plotGen.close();
    cout << ".dat generation success\n";
    return true;
}
//Swap file so class instance can be reused.
void gnuPlotter::changeFile(string newFileName) {//avoid using midoperation
    fileName = newFileName;
}
//Sets up default plt file that reads .dat
bool gnuPlotter::genDefaultPlt() {
    if (plotGen.is_open()) { cout << "ofstream in use by .dat.\n"; return false; }
    plotGen.open(fileName + ".plt");
    plotGen << "set title \"" + fileName + ".wav plot\"\nset xlabel \"Time (Seconds)\"\nset ylabel \"Amplitude\"\nset autoscale\n";
    plotGen << "plot \"" + fileName + ".dat\" with fillsteps";//steps reduce computation time
    plotGen.close();
    cout << ".plt generation success\n";
    return true;
}
//Sets up a plot file that generates a .png, for use in GUI later.
bool gnuPlotter::genImgOutPlt(int width, int height) {
    if (plotGen.is_open()) { cout << "ofstream in use by .dat.\n"; return false; }
    plotGen.open(fileName + ".plt");
    plotGen << "set terminal pngcairo size " << width << "," << height << " enhanced font 'Verdana, 10'\nset title \"" + fileName + ".wav plot\"\nset xlabel \"Time (Seconds)\"\nset ylabel \"Amplitude\"\nset autoscale\nset output \"" + fileName + ".png\"\n";
    plotGen << "plot \"" + fileName + ".dat\" with fillsteps";//steps reduce computation time, curve looks prettier
    plotGen.close();
    cout << ".plt generation success\n";
    img = true;
    return true;
}
//Deletes all files related to current filename
bool gnuPlotter::cleanup() {
    bool success = true;
    if (remove((fileName + ".dat").c_str()) == 0) {
        cout << ".dat deletion success\n";
    }
    else {
        cout << ".dat deletion failure\n";
        success = false;
    }
    if (remove((fileName + ".plt").c_str()) == 0) {
        cout << ".plt deletion success\n";
    }
    else {
        cout << ".plt deletion failure\n";
        success = false;
    }
    if (img) {
        if (remove((fileName + ".png").c_str()) == 0) {
            cout << ".png deletion success\n";
            img = false;
        }
        else {
            cout << ".png deletion failure\n";
            success = false;
        }
    }
    return success;
}
//Uses command line to open up .plt file, works if .plt are handled by gnuplot by default.
void gnuPlotter::openPltWin() {
    system((fileName + ".plt").c_str());
}