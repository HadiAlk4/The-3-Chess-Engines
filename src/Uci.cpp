#include "Uci.h"
#include <iostream>
#include <sstream>
#include <string>

void uciLoop()
{
    // the GUI reads our output through a pipe, so every reply must be flushed right away
    // or it sits in the buffer and the GUI thinks the engine has hung
    std::cout << std::unitbuf;

    std::string line;

    // getline fails when the GUI closes our input, which should end the engine just like "quit"
    while (std::getline(std::cin, line))
    {
        // GUIs on Windows can send \r\n line endings
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        // the first word is the command, the rest of the stream holds its arguments
        std::istringstream tokens(line);
        std::string command;
        tokens >> command;

        // blank lines are allowed and ignored
        if (command.empty())
        {
            continue;
        }

        if (command == "quit")
        {
            break;
        }

        // uci, isready, position and go come with the next milestone.
        // the UCI spec says unknown commands are ignored silently, so the engine
        // keeps running when a GUI sends something it doesn't support
    }
}
