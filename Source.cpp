#include "gnuPlotter.h"
#include "wavReader.h"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
//gui stuff
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <GLFW/glfw3.h>
#include <imgui_impl_opengl3.h>



#define cmd
int main()
{
    //Set namespace stuff
    using string = std::string;
    using std::cout;
    using std::cin;


    //Actual Program
#if defined cmd
    string fileName;
    cout << "Input filename of WAV (Excluding .wav): ";
    cin >> fileName;
    cout << "\n";

    wavReader wav(fileName);
    gnuPlotter gnu(fileName);
    if (!wav.open()) { return 0; }
    cout << "Average amplitude of wav: " << wav.getAverage() << "\n";
    
    unsigned short scale;
    cout << "Number of bits to skip per reading (Prevents crashes): ";
    cin >> scale;
    
    gnu.genDatFromWav(wav, scale);
    //gnu.genImgOutPlt(1024, 512);
    gnu.genDefaultPlt();
    gnu.openPltWin();
    system("pause");
    gnu.cleanup();
    /*
    std::vector<float> data = wav.dataToVector();
    int length = data.size();
    for (int i = 0; i < 1000; i++) {
        cout << data[i] << "\n";
    }*/
#else
    cout << "Not using cmd\n";
#endif
}
