#include "WordFreqApp.hpp"

#include <exception>
#include <iostream>

int main(int Argc, char* Argv[])
{
    WordFreqApp Application;

    int ReturnCode = Application.Init(Argc, Argv);
    if (ReturnCode != 0)
    {
        return ReturnCode;
    }

    try
    {
        return Application.Run();
    }
    catch (const std::exception& Ex)
    {
        std::cerr << Ex.what() << "\n";
        return 1;
    }
    catch (...)
    {
        std::cerr << "Worker failed.\n";
        return 1;
    }
}
