//#include "gnuPlotter.h"
#include "wavReader.h"
//FFT Functions
#include "FFT.h"
//Sola Functions
#include "SOLA.h"

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdio.h>

//gui stuff
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h> 
#include "L2DFileDialog.h"
#include <implot.h>

//Audio Playback
#include <Windows.h>
#include <thread>

#define GUI
#define fileBuffer 100

using std::vector;


int intLog2(int value) {
    int bitNumber = 0;
    for (; value >> bitNumber != 1; bitNumber++);
    return bitNumber;
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

//used to convert cstring to string, only works for 100 max size
std::string charNullEnderToString(char* charPointer, unsigned int length) {
    for (int i = 0; i < 100; i++) {
        if (*(charPointer + i) == NULL) {
            length = i;
            break;
        }
    }
    return std::string(charPointer, charPointer + length);
}

void playOut() {
    std::fstream fp;
    fp.open("out.wav", std::ios::in);
    if (fp.is_open()) {
        PlaySound(L"out.wav", NULL, SND_SYNC);
    }
    else {
        std::cout << "Failure to open out.wav\n";
    }
    fp.close();
}

void applySola(float freqScale, int processingSamples, int overlapSamples, int overlapSeekSamples, vector<int16_t> sampleIn, vector<int16_t> &sampleOut) { //Input in number of samples for sizes
    int size = (int)(sampleIn.size() * freqScale); //1.1f than it needs to be to account for errors in algorithm

    vector<int16_t> temp;
    temp.resize(size);

    //Sola with default values
    SOLA newCalc(1.0 / freqScale,
        processingSamples,//Processing sequence size
        overlapSamples,//Overlap size
        overlapSeekSamples,//Seek for best overlap size
        sampleIn,//Data to read from
        temp//Data to send to
    );//Length of data in
    newCalc.sola();
    sampleOut = temp;
}

void applyFourier(vector<int16_t> dataIn, vector<float>& magnitude, vector<float>& freq, int sampleRate) {
    FFT::CArray temp;
    temp.resize(dataIn.size());
    for (int i = 0; i < dataIn.size(); i++) {
        temp[i].real(dataIn[i]);
    }
    //Applies fourier transform to buffer
    FFT::fft(temp);

    //initialises arrays for xvals
    freq.clear();
    magnitude.clear();
    freq.resize(dataIn.size());
    magnitude.resize(dataIn.size());

    //convert to x values
    for (int i = 0; i < dataIn.size(); i++) {
        freq[i] = (double)(i * sampleRate) / (double)dataIn.size();
    }

    //convert to y values
    for (int i = 0; i < dataIn.size(); i++) {
        magnitude[i] = abs(temp[i]);
    }
}

void applyFourierWindowedSimple(vector<int16_t> dataIn, vector<float>& magnitude, vector<float>& freq, int sampleRate, int windowSize) {
    int windowNum = dataIn.size() / windowSize;
    magnitude.resize(windowSize);
    freq.resize(windowSize);
    for (int j = 0; j < windowNum; j++) {
        
        FFT::CArray temp;
        temp.resize(windowSize);
        for (int i = 0; i < windowSize; i++) {
            int index = i + windowNum * j; //If overshoots, just replaces rest of window with 0s
            if (index >= dataIn.size()) {
                temp[i].real(0);
            }
            else {
                temp[i].real(dataIn[i + windowNum * j]);
            }
            //std::cout << temp[i] << "\n";
        }

        

        //Applies fourier transform to buffer
        FFT::fft(temp);
        //convert to y values
        for (int i = 0; i < windowSize; i++) {
            magnitude[i]+= abs(temp[i]);
        }
    }
    for (int i = 0; i < windowSize; i++) {
        magnitude[i] = magnitude[i] / (float)windowNum / (float)pow(2, 15);
    }


    //convert to x values
    for (int i = 0; i < windowSize; i++) {
        freq[i] = (double)(i * sampleRate) / (double)windowSize;
    }
}

//Windowsize should always be a power of 2
void applyFourierWindowed(vector<int16_t> dataIn, vector<float>& magnitude, vector<float>& freq, int sampleRate, int waveSize, int windowSize) {


    int startingWaveSize = waveSize;

    magnitude.resize(windowSize);
    freq.resize(windowSize);

    unsigned int numberOfWindows = 0; //Counter for number of windows so can calculate average at end for magnitude
    unsigned int dataCounter = 1; //Place in data
    unsigned int previousDataCounter = 0; //Stores previous place in data before last wave end point found.
    unsigned int previousFourierDataCounter = 0; //Stores last place fourier was applied up to.
    unsigned int numberOfWaves = 0; //Number of full waves passed in total
    
    unsigned int previousValues[5] = {waveSize,waveSize,waveSize,waveSize,waveSize};

    //Write to file for debug
    std::ofstream fileOut;
    fileOut.open("debug.txt");


    while ((previousFourierDataCounter + windowSize) < dataIn.size()) {//When more windows added would overshoot data size, stop loop.
        while (((dataCounter+waveSize) - previousFourierDataCounter) < windowSize) {//When more waves discovered would overshoot windowsize, stop loop.
            dataCounter = dataCounter + waveSize;


            //Loop setup
            unsigned int checkPoint = dataCounter; //Marks centre point of search


            //Debug stuff
            int sampleOffBy = 0;

            for (int i = 0;; i++) {
                //std::cout << "Seek " << i << "\n";

                //In addition to checking if 0,
                //which only works when a wave coincidentally is sampled at that exact moment, check if points to the left and right are of opposite signs.
                //If so, there is an unsampled crossing point marking a turning point in the waveform.

                dataCounter = checkPoint + i;
                if (dataIn[dataCounter] == 0 || ((int)dataIn[dataCounter+1]* (int)dataIn[dataCounter]) < 0 || ((int)dataIn[dataCounter-1] * (int)dataIn[dataCounter]) < 0) { sampleOffBy = i; break; }
                dataCounter = checkPoint - i;
                if (dataIn[dataCounter] == 0 || ((int)dataIn[dataCounter + 1] * (int)dataIn[dataCounter]) < 0 || ((int)dataIn[dataCounter - 1] * (int)dataIn[dataCounter]) < 0) { sampleOffBy = i; break; }
            }
        
            //Found crossing point at datacounter, update number of waves passed.
            numberOfWaves++;
            //Need to readjust estimated wavesize.
            int thisWaveSize = dataCounter - previousDataCounter;
            previousValues[numberOfWaves % 5] = thisWaveSize;

            //Recalculating running average
            {
                int sum = 0;
                for (int i = 0; i < 5; i++) {
                    sum += previousValues[i];
                }
                waveSize = (int)round((double)sum / 5.0);
            }

            //Ignore wavesum stuff, just make it same.
            //waveSize = startingWaveSize;

            previousDataCounter = dataCounter; //Updates previousdata counter to current before next loop, needs to be negative one to prevent 0s from not being repeated. When not repeated, transform starts with non-zero causing lobes
            std::cout << "WaveSize" << thisWaveSize << "Avg " << waveSize << "Deviation" << sampleOffBy << "\n";
        }
        //Creates temporary CArray to perform the fourier transform upon.
        FFT::CArray temp;
        temp.resize(windowSize);

        //Takes found series of waveforms under the buffer size, places in complex array. 0 pads rest.

        int offsetToCentre = (windowSize - (dataCounter - previousFourierDataCounter) - 1)/2; //-1 needed due to final 0 to end waveform not being included in count otherwise.

        fileOut << "Offset to centre: " << offsetToCentre << "\n";
        
        fileOut << "Window: " << numberOfWindows << "\n";
        for (int i = 0; i < windowSize;i++) {
            if (i < (dataCounter - previousFourierDataCounter)) {
                temp[i].real(dataIn[previousFourierDataCounter + i]);
            }
            else {
                temp[i].real(0);
            }
            fileOut << temp[i] << "\n";
        }

        

        //Applies fourier transform to buffer
        FFT::fft(temp);
        //convert to y values
        for (int i = 0; i < windowSize; i++) {
            magnitude[i] += abs(temp[i]);
        }
        //Increment number of windows for magnitude average calculation
        numberOfWindows++;
        //Update Fourier counter, so know where to start off from for next loop
        previousFourierDataCounter = dataCounter;
    }

    fileOut.close();

    std::cout << "Number of waves" << numberOfWaves << "\n";
    std::cout << "Number of windows" << numberOfWindows << "\n";


    for (int i = 0; i < windowSize; i++) {
        magnitude[i] = magnitude[i] / (float)numberOfWindows / (float)pow(2, 15);
    }


    //convert to x values
    for (int i = 0; i < windowSize; i++) {
        freq[i] = (double)(i * sampleRate) / (double)windowSize;
    }
}


int main(int, char**)
{
    //Set namespace stuff
    using string = std::string;
    using std::cout;
    using std::cin;

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "DSP Data Processing", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    //make implot context as well
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
    

    // Our state
    bool showDemo = false;
    bool showPlotDemo = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);


    //My Variables
    char* fileSelectionBuffer = new char[fileBuffer](); //Allows filenames of up to 100 bytes
    char* driveSelector = new char[2]();
    std::string currentFilePath;

    wavReader wav;
    //gnuPlotter gnu;

    bool wavWindow = false;
    bool plotWindow20ms = false;

    bool plotWindowSOLA = false;

    bool plotFreq = false;

    bool dataWindowTable = false;


    int scale = 1;
    float solaTimeScale = 0.5;
    float oldSolaTimeScale = 0.0;

    float resampleTimeScale = 2.0;
    float frequencyScale = 1.1;

    //Max number of samples allowed in plots
    int sampleLimit = 3e3;

    //Stores wavdata
    vector<int16_t> wavData;
    //Stores wavdata in float format for reading by 20ms window
    vector<float> wavTime;
    vector<float> trueValsFloat;

    //Stores values directly gotten from applying sola to the data
    vector<float> solaBuffer;
    vector<int16_t> rawSolaBuffer;
    vector<float> solaTime;


    //Databuffer system saved
    struct dataWindow{
        //Data display stuff
        vector<float> dataDisplay;
        vector<float> dataTimeDisplay;
        vector<int16_t> dataBuffer;

        //Fourier buffer stuff
        vector<float> fourierMag;
        vector<float> fourierFreq;


        //Currently unimplemented outside of here
        //Display vectors of fourier stuff
        vector<float> magDisplay;
        vector<float> freqDisplay;


        //need a way to prevent problems with databuffer changing from window to window, each window should have its own databuffer logically.


        //Flags
        bool dataUpdated = false;
        bool showBuffer = false;
        //millisec mode flags
        bool millisecMode = false;
        int millisecCurrent = 0;
        bool millisecUpdated = false;
        //Fourier flags
        bool showFourier = false;

        int fourierSize = 1024;
        int fourierSizePower = 10;
        float waveFreq = 1000;
    };
    
    vector<dataWindow> windows;

    int currentBuffer = 0;


    //Stores values used in display of sola data
    vector<float> dSolaBuffer;
    vector<float> dSolaTime;


    //Buffers for fourier values for original 20ms
    FFT::CArray fourierBuffer;
    vector<double> fftYVals; //if not initialised to nullptr, program wont compile as pointer points to literally nothing, not even the nullptr
    vector<double> fftXVals;

    //Buffers for fourier values for the SOLA'd 20ms
    vector<double> fftYVals2;//Used for fft of sola algorithm'd portion
    vector<double> fftXVals2;

    //Pointers for controlling 20ms sample location
    int sampleOffset20ms = 0;
    int sampleNum20ms = 0;

    //Stores samplerate so doesn't have to be called multiple times
    int sampleRate = 0;

    //Bools that store state of whether windows have been updated, and therefore need recentering again.
    bool offsetUpdated = false;
    bool offsetUpdatedFreq = false;
    bool offsetUpdatedSola = false;
    bool offsetUpdatedFreqSola = false;

    //Bool that makes the fourier values update
    bool updateFourier = false;

    //Bool that tracks whether SOLA has been called
    bool flagSola = false;
    bool flagRecalculateSola = false;
    bool flag20msSola = false;

    //Flags for frequency shift
    bool flagFreqShift = false;

    windows.push_back(dataWindow());

    //Start of code that matters
    while (!glfwWindowShouldClose(window)) {
        //Essential Rubbish
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        //==================
        //Start of "my code"
        //==================

        if (showDemo) {
            ImGui::ShowDemoWindow(&showDemo);//show demo is the bool that decides when this closes
        }

        {
            ImGui::Begin("Base Window");
            ImGui::Checkbox("Show Demo Window", &showDemo);
            ImGui::Checkbox("Show WavReader", &wavWindow);
            ImGui::Checkbox("Show Plot Demo", &showPlotDemo);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            if (ImGui::Button("Hello Button")) {
                cout << "Hello to you!\n";
            }
            ImGui::End();
        }

        if (showPlotDemo) {
            ImPlot::ShowDemoWindow(&showPlotDemo);
        }

        if (dataWindowTable) {
            std::stringstream ss;
            ImGui::Begin("Buffer Selection", &dataWindowTable);
            if (ImGui::Button("Add buffer")) {
                windows.push_back(dataWindow());
            }
            ss << "Total Number of Buffers: " << windows.size();
            ImGui::Text(ss.str().c_str());
            ss.str(std::string());

            ss << "Current Selected Buffer: " << currentBuffer;
            ImGui::Text(ss.str().c_str());
            ss.str(std::string());

            for (int i = 0; i < windows.size(); i++) {
                ss << "Buffer " << i;
                if (ImGui::Button(ss.str().c_str())) {
                    currentBuffer = i;
                }
                ss.str(std::string());
            }
            ImGui::End();
        }

        if (wavWindow) {
            dataWindowTable = true;

            ImGui::Begin("WavReader", &wavWindow);
            std::stringstream ss;
            ss << "Current WAV: " << wav.getFileName();
            ImGui::Text(ss.str().c_str());
            ss.str(std::string()); ss << "Channels: " << wav.getChannels();
            ImGui::Text(ss.str().c_str());
            ss.str(std::string()); ss << "Sample Rate: " << wav.getSampleRate();
            ImGui::Text(ss.str().c_str());
            ss.str(std::string()); ss << "Sample Number: " << wav.getSampleNum();
            ImGui::Text(ss.str().c_str());
            if (ImGui::Button("Locate .wav")) {
                FileDialog::fileDialogOpen = true;
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(20.0f);
            if (ImGui::InputText("Drive", driveSelector, 2)) { //for some reason needs to be 2 long to store one character, might be end of string signifier
            }
            ImGui::SameLine();
            if (ImGui::Button("Import to Buffer") && wav.is_open()) {
                windows[currentBuffer].dataBuffer = wavData;
                windows[currentBuffer].dataUpdated = true;
            }
            /*
            if (ImGui::Button("Plot wav in 20ms") && wav.is_open()) {
                sampleOffset20ms = 0;
                plotWindow20ms = true;
            }
            */
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::InputInt("Max Samples on Plot", &sampleLimit, 1e4, 1e6)) {
                if (sampleLimit < 1e3) { sampleLimit = 1e3; }//can't be close to 0
            }
            /*
            if (ImGui::Button("Create FFT of 20ms") && plotWindow20ms) {
                updateFourier = true;
            }
            */
            if (ImGui::Button("Apply Sola to Buffer")) {
                flagRecalculateSola = true;
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::InputFloat("TimeScale (Sola)", &solaTimeScale, 0.05f, 0.2f)) {
                if (solaTimeScale <= 0.0f) {
                    solaTimeScale = 0.05f;
                }
            }

            if (ImGui::Button("Apply Resampling")) {
                windows[currentBuffer].dataBuffer = vectorStuff::resampleToSize(windows[currentBuffer].dataBuffer, windows[currentBuffer].dataBuffer.size() * resampleTimeScale);
                windows[currentBuffer].dataUpdated = true;
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::InputFloat("TimeScale (Resample)", &resampleTimeScale, 0.05f, 0.2f)) {
                if (resampleTimeScale <= 0.0f) {
                    resampleTimeScale = 0.05f;
                }
            }

            if (ImGui::Button("Frequency Shift by factor")) {
                flagFreqShift = true;
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::InputFloat("Frequency Scale", &frequencyScale, 0.05f, 0.2f)) {
                if (frequencyScale <= 0.0f) {
                    frequencyScale = 0.05f;
                }
            }

            if (ImGui::Button("Write to out.wav")) {
                //Writes to disk
                wav.writeBuffer(windows[currentBuffer].dataBuffer);
            }

            if (ImGui::Button("Play out.wav")) {
                //Removes from main thread to prevent freezing
                std::thread audio(playOut);
                audio.detach();
            }
            ImGui::End();
        }
        /*
        if (plotWindow20ms) {
            ImGui::Begin("Plotting Window 20ms Samples");
            if (offsetUpdated) { ImPlot::FitNextPlotAxes(); offsetUpdated = false; }//recentres plot
            if (ImPlot::BeginPlot(("Plot of " + wav.getFileName() + " over 20ms").c_str(), "Time (s)", "Amplitude")) {
                ImPlot::PlotLine(wav.getFileName().c_str(), &wavTime[sampleNum20ms * sampleOffset20ms], &trueValsFloat[sampleNum20ms * sampleOffset20ms], sampleNum20ms);
                ImPlot::EndPlot();
            }

            if (ImGui::InputInt("20ms sampleNumber", &sampleOffset20ms, 1, 100)) {
                offsetUpdated = true;
                updateFourier = true;
                offsetUpdatedSola = true;
                int max = wav.getChannelLength() / sampleNum20ms;
                if (sampleOffset20ms > max - 1) {
                    sampleOffset20ms = max - 1;
                }
                else if (sampleOffset20ms < 0) {
                    sampleOffset20ms = 0;
                }
            }

            ImGui::End();
        }
        */
        {
            std::stringstream ss;
            for (int i = 0; i < windows.size(); i++) {
                if (windows[i].showBuffer) {
                    ss << "Buffer Window " << i;
                    ImGui::Begin(ss.str().c_str());
                    ss.str(std::string());

                    if (ImGui::Button("Toggle 20ms mode")) {
                        windows[i].millisecMode = !windows[i].millisecMode;
                    }

                    ss << "Plot of buffer " << i;
                    if (!windows[i].millisecMode) {
                        if (ImPlot::BeginPlot(ss.str().c_str(), "Time (s)", "Amplitude")) {
                            ImPlot::PlotStairs(wav.getFileName().c_str(), &windows[i].dataTimeDisplay[0], &windows[i].dataDisplay[0], windows[i].dataDisplay.size());
                            ImPlot::EndPlot();
                        }
                    }
                    else {
                        if (windows[i].millisecUpdated) { ImPlot::FitNextPlotAxes(); windows[i].millisecUpdated = false; }//recentres plot
                        int samples = windows[i].dataBuffer.size() / wav.getSampleNum_ms(20);
                        if (ImPlot::BeginPlot((ss.str() + " over 20ms").c_str(), "Time (s)", "Amplitude")) {
                            ImPlot::PlotLine(wav.getFileName().c_str(), &windows[i].dataBuffer[samples * windows[i].millisecCurrent], samples); //need timescale
                            ImPlot::EndPlot();
                        }
                    }
                    ss.str(std::string());
                    if (windows[i].millisecMode) {
                        if (ImGui::InputInt("20ms sampleNumber", &windows[i].millisecCurrent, 1, 100)) {
                            windows[i].millisecUpdated = true;
                            int max = windows[i].dataBuffer.size() / wav.getSampleNum_ms(20);
                            if (windows[i].millisecCurrent > max - 1) {
                                windows[i].millisecCurrent = max - 1;
                            }
                            else if (windows[i].millisecCurrent < 0) {
                                windows[i].millisecCurrent = 0;
                            }
                        }
                    }

                    ss << "Samples: " << windows[i].dataBuffer.size();
                    ImGui::Text(ss.str().c_str());


                    if (ImGui::Button("Fourier!")) {
                        updateFourier = true;
                    }
                    ss.str(std::string());

                    ss << "Size " << windows[i].fourierSize;


                    ImGui::Text(ss.str().c_str());
                    ss.str(std::string());
                    ImGui::SameLine();

                    ImGui::SetNextItemWidth(20.0f);
                    if (ImGui::InputInt("Buffer Size", &windows[i].fourierSizePower, 1, 10)) {
                        if (windows[i].fourierSizePower < 7) {
                            windows[i].fourierSizePower = 7;
                        }
                        windows[i].fourierSize = (1 << windows[i].fourierSizePower);//Returns power of 2
                    }
                    if (ImGui::InputFloat("Estimated frequency", &windows[i].waveFreq, 100, 1000)) {
                        if (windows[i].waveFreq == 0 || windows[i].waveFreq < 0) {
                            windows[i].waveFreq = 1;
                        }
                    }


                    ss.str(std::string());

                    ss << "Plot of buffer " << i << "'s frequencies";

                    if (windows[i].showFourier) {
                        if (ImPlot::BeginPlot(
                            ss.str().c_str(),
                            "Freq (Hz)",
                            "Magnitude"
                            , ImVec2(-1, 0), ImPlotFlags_None, ImPlotAxisFlags_LogScale, ImPlotAxisFlags_LogScale //Logscale doesn't work, no idea why.
                        )) {
                            //Converts float to new array as data is stored continguously in vectors, same as arrays.
                            //cout << "Size of 20ms is: " << size << "\n";
                            ImPlot::PlotLine(ss.str().c_str(), &windows[i].fourierFreq[0], &windows[i].fourierMag[0], windows[i].fourierMag.size()/2);//divided by two to only show positive freq values
                            ImPlot::EndPlot();
                        }
                    }
                    ss.str(std::string());

                    ss << "Total Harmonic Distortion" << vectorStuff::totalHarmonicDistortion(windows[i].fourierMag);

                    ImGui::Text(ss.str().c_str());
                    ss.str(std::string());
                    ImGui::SameLine();

                    ImGui::End();
                }
            }
        }
        
        /*
        if (plotFreq) {
            ImGui::Begin("Plotting Window Freq");
            if (offsetUpdatedFreq) { ImPlot::FitNextPlotAxes(); offsetUpdatedFreq= false; }//recentres plot
            if (ImPlot::BeginPlot(
                ("Plot of " + wav.getFileName()+"'s frequencies").c_str(), 
                "Freq (Hz)", 
                "Magnitude"
                , ImVec2(-1, 0), ImPlotFlags_None, ImPlotAxisFlags_LogScale, ImPlotAxisFlags_LogScale //Logscale doesn't work, no idea why.
                )) {
                //Converts float to new array as data is stored continguously in vectors, same as arrays.
                //cout << "Size of 20ms is: " << size << "\n";
                ImPlot::PlotLine(wav.getFileName().c_str(), &fftXVals[0], &fftYVals[0], sampleNum20ms/2);//divided by two to only show positive freq values
                ImPlot::EndPlot();
            }
            ImGui::End();
        }
        */
        //===========
        //Loop Checks
        //===========

        //Opens file dialog lib
        if (FileDialog::fileDialogOpen == true) {
            FileDialog::ShowFileDialog(&FileDialog::fileDialogOpen,//Hands access to window creation to filedialog
                fileSelectionBuffer,            //Gives a file selection buffer to edit
                fileBuffer,                     //Size of buffer
                std::string(&driveSelector[0])  //Makes drive selector into regular string to pass through to file dialog
            );
        }
        //Checks if file selection buffer has anything in it, if it does, opens up the wav.
        if (!(*fileSelectionBuffer == NULL)) {
            //Destroys plot windows to prevent problems
            plotWindow20ms = false;
            plotFreq = false;

            std::string buffer = charNullEnderToString(fileSelectionBuffer, fileBuffer);
            wav.openSpecific(buffer);
            cout << buffer << " has been auto-opened\n";
            fileSelectionBuffer = new char[fileBuffer]();
            sampleNum20ms = wav.getSampleNum_ms(20);
            wavData = wav.dataToVector();
            sampleRate = wav.getSampleRate();

            //Initialisation/cleanup of pointers
            wavTime.clear(); trueValsFloat.clear();
            wavTime.resize(wavData.size());
            trueValsFloat.resize(wavData.size());

            vectorStuff::floatData(&wavData[0], &trueValsFloat[0], wavData.size());
            //Makes time for plotting/calculations

            for (int i = 0; i < wavData.size(); i++) {
                wavTime[i] = (float)(i)/(float)sampleRate;
            }
        }

        if (flagRecalculateSola) {
            applySola(frequencyScale, wav.getSampleNum_ms(100), wav.getSampleNum_ms(20), wav.getSampleNum_ms(15), windows[currentBuffer].dataBuffer, windows[currentBuffer].dataBuffer);
            //Flag Reseting
            windows[currentBuffer].dataUpdated = true;
            flagRecalculateSola = false;
            flagSola = true;
            oldSolaTimeScale = solaTimeScale;
        }

        if (flagFreqShift) {
            if (frequencyScale > 1.0) {

                applySola(frequencyScale, wav.getSampleNum_ms(100), wav.getSampleNum_ms(20), wav.getSampleNum_ms(15), windows[currentBuffer].dataBuffer, windows[currentBuffer].dataBuffer);
                //Resample
                windows[currentBuffer].dataBuffer = vectorStuff::resampleToSize(windows[currentBuffer].dataBuffer, windows[currentBuffer].dataBuffer.size() / frequencyScale);
            }
            else if (frequencyScale < 1.0) {
                //Resample
                windows[currentBuffer].dataBuffer = vectorStuff::resampleToSize(windows[currentBuffer].dataBuffer, windows[currentBuffer].dataBuffer.size() / frequencyScale);
                //Sola
                applySola(frequencyScale, wav.getSampleNum_ms(100), wav.getSampleNum_ms(20), wav.getSampleNum_ms(15), windows[currentBuffer].dataBuffer, windows[currentBuffer].dataBuffer);
            }
            windows[currentBuffer].dataUpdated = true;
            flagFreqShift = false;
        }


        //moved from previous button, so multiple things can access it.

        
        if (updateFourier) {
            //int fourierWindowSize = wav.getSampleNum_ms(20);

            //applyFourierWindowed2(windows[currentBuffer].dataBuffer, windows[currentBuffer].fourierMag, windows[currentBuffer].fourierFreq, wav.getSampleRate(), windows[currentBuffer].fourierSize);

            cout << 1.0 / 1000.0 * wav.getSampleRate() << "\n";

            applyFourierWindowed(windows[currentBuffer].dataBuffer, windows[currentBuffer].fourierMag, windows[currentBuffer].fourierFreq, wav.getSampleRate(), 1.0/windows[currentBuffer].waveFreq*wav.getSampleRate(), windows[currentBuffer].fourierSize);

            

            updateFourier = false;
            windows[currentBuffer].showFourier = true;
        }

        //Updating display values of the buffer

        for (int j = 0; j < windows.size(); j++) {
            if (windows[j].dataUpdated) {
                vector<int16_t> temp = vectorStuff::resampleToSize(windows[j].dataBuffer, sampleLimit);
                windows[j].dataDisplay.resize(temp.size());
                vectorStuff::floatData(&temp[0], &windows[j].dataDisplay[0], temp.size());
                windows[j].dataTimeDisplay.resize(temp.size());
                for (int i = 0; i < temp.size(); i++) {
                    windows[j].dataTimeDisplay[i] = (float)i / wav.getSampleRate() * (float)windows[j].dataBuffer.size()/(float)windows[j].dataDisplay.size();//should probably store the sample rate of the buffer currently.
                }
                windows[j].dataUpdated = false;
                windows[j].showBuffer = true;
            }
        }
        
        // Rendering, must take place at the end of every loop
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
};