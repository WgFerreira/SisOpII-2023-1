#include <string>
#include <iostream>

#include "include/input_parser.h"

#define TKN_DEBUG "--debug"
#define TKN_TIMEOUT "--timeout"
#define TKN_MONITOR "--monitor"

void InputParser::parse(int argc, const char *argv[])
{
  for (int i = 1; i < argc; i++)
  {
    std::string arg = argv[i];
    if (arg == TKN_DEBUG)
      this->debug = true;

    else if (arg == TKN_TIMEOUT && i+1 < argc)
    {
      i++;
      this->timeout = std::stoi(argv[i]);
    }
    
    else if (arg == TKN_MONITOR && i+1 < argc)
    {
      i++;
      this->monitor = std::stoi(argv[i]);
    }
  }
}

