#ifndef _INPUT_PARSER_H
#define _INPUT_PARSER_H

class InputParser 
{
public:
  bool debug = false;
  int timeout = 500;
  int monitor = 1000;

  void parse(int argc, const char *argv[]);
};

#endif
