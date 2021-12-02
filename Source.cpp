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

#define GUI
#define fileBuffer 100

using std::vector;

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
    bool plotWindow = false;
    bool plotWindow20ms = false;

    bool plotWindowSOLA = false;

    bool plotFreq = false;


    int scale = 1;
    float solaTimeScale = 2.0;

    //Max number of samples allowed in plots
    int sampleLimit = 1e4;

    //Stores wavdata
    vector<int16_t> wavData;
    //Stores wavdata in float format for reading by 20ms window
    float* wavTime = nullptr;
    float* trueValsFloat = nullptr;

    //Stores values to be displayed by entire plot
    float* yVals = nullptr;//Stores wavfile info
    float* xVals = nullptr;
    size_t valLength = 0;

    //Stores values directly gotten from applying sola to the data
    float* solaBuffer = nullptr;
    float* solaTime = nullptr;

    //Stores values used in display of sola data
    float* dSolaBuffer = nullptr;
    float* dSolaTime = nullptr;


    //Buffers for fourier values for original 20ms
    FFT::CArray fourierBuffer;
    double* fftYVals = nullptr; //if not initialised to nullptr, program wont compile as pointer points to literally nothing, not even the nullptr
    double* fftXVals = nullptr;

    //Buffers for fourier values for the SOLA'd 20ms
    double* fftYVals2 = nullptr;//Used for fft of sola algorithm'd portion
    double* fftXVals2 = nullptr;

    //Pointers for controlling 20ms sample location
    int sampleOffset20ms = 0;
    int sampleNum20ms = 0;

    //Stores samplerate so doesn't have to be called multiple times
    int sampleRate = 0;

    //Bools that store state of whether windows have been updated, and therefore need recentering again.
    bool offsetUpdated = false;
    bool offsetUpdatedFreq = false;

    //Bool that makes the fourier values update
    bool updateFourier = false;

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

        if (wavWindow) {
            ImGui::Begin("WavReader", &wavWindow);
            std::stringstream ss;
            ss << "Current WAV: " << wav.getFileName();
            ImGui::Text(ss.str().c_str());
            ss.str(std::string()); ss <<"Channels: " << wav.getChannels();
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
            /*
            if (ImGui::Button("Plot in GNUPLOT") && wav.is_open()) {
                gnu.changeFile(wav.getFileName());
                gnu.genDatFromWavAuto(wav);
                //gnu.genImgOutPlt(1024, 512);
                gnu.genDefaultPlt();
                gnu.openPltWin();
            }
            
            ImGui::SameLine();
            ss.str(std::string()); ss << "Previous Gnuplot accuracy: 1/" << gnu.getLastAutoScale();
            ImGui::Text(ss.str().c_str());
            if (ImGui::Button("Clean up GNUPLOT files")) {
                gnu.cleanup();
            }
            */
            if (ImGui::Button("Plot in IMPLOT") && wav.is_open()) {
                vector<int16_t> shrunken = vectorStuff::shrinkData(wavData, sampleLimit);
                valLength = shrunken.size();

                if (yVals != nullptr) {
                    delete[] yVals;
                }
                if (xVals != nullptr) {
                    delete[] xVals;
                }

                yVals = new float[valLength];
                xVals = new float[valLength];

                vectorStuff::floatData(&shrunken[0], yVals, valLength);
                float ratio = (float)wavData.size() / (float)valLength;
                for (int i = 0; i < valLength; i++) {
                    xVals[i] = (float)(i+1) * ratio / (float)sampleRate;
                }
                plotWindow = true;
            }
            
            if (ImGui::Button("Plot in IMPLOT 20ms") && wav.is_open()) {
                sampleOffset20ms = 0;
                plotWindow20ms = true;
            }
            
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::InputInt("Max Samples on Plot", &sampleLimit, 1e4, 1e6)) {
                if (sampleLimit < 1e3) { sampleLimit = 1e3; }//can't be close to 0
            }
            if (ImGui::Button("Create FFT of 20ms") && plotWindow20ms) {
                updateFourier = true;
            }
            
            if (ImGui::Button("Apply Sola With Default Vals")) {
                //Memory cleanup
                if (solaTime != nullptr) {
                    delete[] solaTime;
                }
                if (solaBuffer != nullptr) {
                    delete[] solaBuffer;
                }

                int size = (int)(wavData.size() / solaTimeScale * 1.1f); //1.1f than it needs to be to account for errors in algorithm
                plotWindowSOLA = true;
                //Expands solaBuffer
                int16_t*temp = new int16_t[size];
                //Sola with default values
                SOLA newCalc(solaTimeScale, //Time scale to acheive, 2 = double speed, 1/2 = half speed
                    wav.getSampleNum_ms(100),//Processing sequence size
                    wav.getSampleNum_ms(20),//Overlap size
                    wav.getSampleNum_ms(15),//Seek for best overlap size
                    &wavData[0],//Data to read from
                    temp,//Data to send to
                    wavData.size());//Length of data in

                newCalc.sola();
                solaTime = new float[(int)(wavData.size() / solaTimeScale * 1.1f)];


                //Reconstructs time portion
                for (int i = 0; i < size; i++) {
                    solaTime[i] = (float)i / wav.getSampleRate();
                }

                //Converts to float for display
                solaBuffer = new float[size];
                vectorStuff::floatData(temp, solaBuffer, size);
                
                //Memory Cleanup
                delete[] temp;
            }
            
            ImGui::End();
        }

        if (plotWindow) {
            ImGui::Begin("Plotting Window");
            if (ImPlot::BeginPlot(("Plot of " + wav.getFileName()).c_str(), "Time (s)", "Amplitude")) {
                ImPlot::PlotStairs(wav.getFileName().c_str(), xVals, yVals, valLength);
                ImPlot::EndPlot();
            }
            ImGui::End();
        }
        
        if (plotWindow20ms) {
            ImGui::Begin("Plotting Window 20ms Samples");
            if (offsetUpdated) { ImPlot::FitNextPlotAxes(); offsetUpdated = false; }//recentres plot
            if (ImPlot::BeginPlot(("Plot of " + wav.getFileName() + " over 20ms").c_str(), "Time (s)", "Amplitude")) {
                //Grabs small segment of window
                ImPlot::PlotLine(wav.getFileName().c_str(), &wavTime[sampleNum20ms * sampleOffset20ms], &trueValsFloat[sampleNum20ms * sampleOffset20ms], sampleNum20ms);
                ImPlot::EndPlot();
            }
            if (ImGui::InputInt("20ms sampleNumber", &sampleOffset20ms, 1, 100)) {
                offsetUpdated = true;
                updateFourier = true;
                int max = wav.getChannelLength() / sampleNum20ms;
                if (sampleOffset20ms > max-1) {
                    sampleOffset20ms = max-1;
                }
                else if (sampleOffset20ms < 0) {
                    sampleOffset20ms = 0;
                }
            }
            ImGui::End();
        }
        
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
                ImPlot::PlotLine(wav.getFileName().c_str(), fftXVals, fftYVals, sampleNum20ms/scale/2);//divided by two to only show positive freq values
                ImPlot::EndPlot();
            }
            ImGui::End();
        }
        
        if (plotWindowSOLA) {
            ImGui::Begin("SOLA");
            if (ImPlot::BeginPlot(("Plot of " + wav.getFileName()+"SOLA'd").c_str(), "Time (s)", "Amplitude")) {
                //Converts float to new array as data is stored continguously in vectors, same as arrays.
                ImPlot::PlotStairs(wav.getFileName().c_str(), solaTime, solaBuffer, (int)(wavData.size() / solaTimeScale));
                ImPlot::EndPlot();
            }
            ImGui::End();
        }
        
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
            plotWindow = false;
            plotWindow20ms = false;
            plotFreq = false;

            std::string buffer = charNullEnderToString(fileSelectionBuffer, fileBuffer);
            cout << buffer << " has been auto-opened\n";
            wav.openSpecific(buffer);
            fileSelectionBuffer = new char[fileBuffer]();
            sampleNum20ms = wav.getSampleNum_ms(20);
            wavData = wav.dataToVector();
            sampleRate = wav.getSampleRate();

            //Initialisation/cleanup of pointers
            if (wavTime != nullptr) {
                delete[] wavTime;
            }
            if (trueValsFloat != nullptr) {
                delete[] trueValsFloat;
            }
            wavTime = new float[wavData.size()];
            trueValsFloat = new float[wavData.size()];

            vectorStuff::floatData(&wavData[0], trueValsFloat, wavData.size());
            //Makes time for plotting/calculations

            for (int i = 0; i < wavData.size(); i++) {
                wavTime[i] = (float)(i)/(float)sampleRate;
            }
        }


        //moved from previous button, so multiple things can access it.

        
        if (updateFourier) {
            fourierBuffer.resize(sampleNum20ms);
            for (int i = 0; i < sampleNum20ms; i++) {
                fourierBuffer[i].real(trueValsFloat[i + sampleOffset20ms * sampleNum20ms]);
            }
            //Applies fourier transform to buffer
            FFT::fft(fourierBuffer);

            //initialises arrays for xvals
            fftXVals = new double[sampleNum20ms];
            fftYVals = new double[sampleNum20ms];

            int sampleFreq = wav.getSampleRate();

            
            //Non Log
            //convert to x values
            for (int i = 0; i < sampleNum20ms; i++) {
                fftXVals[i] = (double)(i * sampleFreq) / (double)sampleNum20ms;
            }

            //convert to y values
            for (int i = 0; i < sampleNum20ms; i++) {
                fftYVals[i] = abs(fourierBuffer[i]);
            }
            //cleans up buffer when done
            fourierBuffer = FFT::CArray();
            updateFourier = false;
            offsetUpdatedFreq = true;
            plotFreq = true;
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