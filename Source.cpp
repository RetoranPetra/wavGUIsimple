#include "gnuPlotter.h"
#include "wavReader.h"
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


    //Actual Program
#if defined cmd
    string fileName;
    cout << "Input filename of WAV (Excluding .wav): ";
    cin >> fileName;
    cout << "\n";

    wavReader wav;
    gnuPlotter gnu;
    if (!wav.open(fileName)) { return 0; }
    cout << "Average amplitude of wav: " << wav.getAverage() << "\n";
    /*
    unsigned short scale;
    cout << "Number of bits to skip per reading (Prevents crashes): ";
    cin >> scale;
    
    gnu.genDatFromWav(wav, scale);
    //gnu.genImgOutPlt(1024, 512);
    gnu.genDefaultPlt();
    gnu.openPltWin();
    system("pause");
    gnu.cleanup();
    */
    
    std::vector<float> data = wav.dataToVector(1);
    double sum = 0;
    int length = data.size();
    for (int i = 0; i < length; i++) {
        sum += fabs(data[i]);
    }
    cout << "Avg " << sum / (double)length;

#elif defined GUI
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Caleb Evans - DSP Data Processing", NULL, NULL);
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
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);


    //My Variables
    char* fileSelectionBuffer = new char[fileBuffer](); //Allows filenames of up to 100 bytes
    std::string currentFilePath;
    std::string drive = "C";

    wavReader wav;
    gnuPlotter gnu;

    bool wavWindow = false;
    bool plotWindow = false;

    int scale = 1;

    vector<float> yVector;
    vector<float> xVector;
    int sampleLimit = 1e6;

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
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            if (ImGui::Button("Hello Button")) {
                cout << "Hello to you!\n";
            }
            ImGui::End();
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
            if (ImGui::Button("Press For Files!")) {
                FileDialog::fileDialogOpen = true;
            }
            if (ImGui::Button("Plot in GNUPLOT")) {
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
            if (ImGui::Button("Plot in IMPLOT")) {
                int channelLength = wav.getChannelLength();
                scale = 1;
                for (int x = channelLength; x > sampleLimit; x = channelLength / scale) {
                    scale++;
                }
                cout << "Using scale: " << scale << "\n";

                yVector = wav.dataToVector(scale);
                xVector = wav.timeToVector(scale);
                cout << "y size: " << yVector.size() << "\nx size: " << xVector.size() << "\nChannel Size: " << wav.getChannelLength() << "\n";
                cout << "y\n";
                for (int i = 0; i < 1000; i++) {
                    cout << yVector[i];
                }
                cout << "x\n";
                for (int i = 0; i < 1000; i++) {
                    cout << xVector[i] << "\n";
                }
                plotWindow = true;
            }
            ImGui::SameLine();
            ImGui::InputInt("Max Samples on Plot", &sampleLimit, 1e5, 1e6);
            ImGui::End();
        }

        if (plotWindow) {
            ImGui::Begin("Plotting Window");
            if (ImPlot::BeginPlot(("Plot of " + wav.getFileName()).c_str())) {
                //Converts float to new array as data is stored continguously in vectors, same as arrays.
                cout << "inside implot if\n";
                float* yVals = &yVector[0];
                float* xVals = &xVector[0];
                ImPlot::PlotStairs("Dataset", xVals, yVals, wav.getChannelLength()/scale);
                ImPlot::EndPlot();
            }
            cout << "Outside implot if\n";
            ImGui::End();
        }


        //===========
        //Loop Checks
        //===========

        //Opens file dialog lib
        if (FileDialog::fileDialogOpen == true) {
            FileDialog::ShowFileDialog(&FileDialog::fileDialogOpen,fileSelectionBuffer,100,std::string(drive));
        }

        //Checks if file selection buffer has anything in it
        if (!(*fileSelectionBuffer == NULL) /*&& !wavOpened*/) {
            //wavOpened = true;
            std::string buffer = charNullEnderToString(fileSelectionBuffer, 100);
            cout << buffer << " has been auto-opened\n";
            wav.openSpecific(buffer);
            fileSelectionBuffer = new char[fileBuffer]();
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
#endif
}
