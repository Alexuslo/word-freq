#include "WordFreqApp.hpp"

int main(int Argc, char* Argv[])
{
    auto& Application = WordFreqApp::Get();

    int ReturnCode = Application.Init(Argc, Argv);
    if (ReturnCode != 0) return ReturnCode;

    ReturnCode = Application.Run();

    Application.Release();
    return ReturnCode;
}
