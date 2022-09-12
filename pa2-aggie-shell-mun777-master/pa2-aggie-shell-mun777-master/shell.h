#include <iostream>
#include "Tokenizer.h"
#define MAX_PATH 256

char CURRENT_PATH[MAX_PATH];
std::string prevfile;
std::vector<pid_t> PIDS;
std::vector<std::string> commandhis;
char hostname[1024];

void process_pipe(Tokenizer &);
void process_command(Tokenizer &);
void check_background();

void change_dir();
