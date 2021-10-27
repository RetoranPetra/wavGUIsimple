#pragma once
#include "wavReader.h"
#include <string>
#include <iostream>
#include <fstream>
class gnuPlotter {
private:
    std::string fileName;
    //vector<int> xData;
    //vector<double> yData;
    std::ofstream plotGen;
    bool img;
    bool openDat();
public:
    gnuPlotter(std::string name);
    bool genDatFromWav(wavReader& wav, unsigned short scale);//scale used to reduce number of data points to stop gnuplot from crashing
    void changeFile(std::string newFileName);
    bool genDefaultPlt();
    bool genImgOutPlt(int width, int height);
    bool cleanup();
    void openPltWin();
};