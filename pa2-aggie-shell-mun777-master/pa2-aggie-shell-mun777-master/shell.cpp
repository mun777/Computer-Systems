#include <iostream>
#include "shell.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>

#include <vector>
#include <string>
#include <fcntl.h>
#include "Tokenizer.h"
#include <linux/limits.h>
#include <cstring>

// all the basic colours for a shell prompt
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"

using namespace std;
void check_background()
{
    int status;
    for(__uint64_t i =0; i < PIDS.size(); i++)
    {
        if((status = waitpid(PIDS[i], 0, WNOHANG)) > 0)
        {
            std::cout << "PIDS killed " << *(PIDS.begin() + i) <<std::endl;
            PIDS.erase(PIDS.begin() +i);
            i--;
        }
    }
}
void spawn_processes(int in,int  out ,Command *cmd)
{
    pid_t pid = fork();
    if (pid ==0)
    {
        //child
        if(cmd->hasInput())
        {
            int rd = open(cmd->in_file.c_str(), O_RDONLY); //permissions for child stdin
            if(rd <0){
                perror("ouput");
                exit(2);
            }
            if(dup2(rd,0) < 0)
            {
                perror("dup");
                exit(2);
            }
        }
        else if (in != 0)
        {
            dup2(in,0);
            //close(out);
            //redir output of file to  to read end of pipe
        }

        if(cmd->hasOutput())
        {
            int wr = open(cmd->out_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR| S_IWUSR);
            if(wr < 0){
                perror("ouput");
                exit(2);
            }
            if(dup2(wr,1) < 0)
            {
                perror("dup");
                exit(2);
            }
        }
        else if (out!= 1)
        {
            dup2(out,1);
            //close(in);
            //reset redir output to write end of pipe
        }
        size_t size = cmd->args.size();
        char** args = new char*[size+1];
        for(size_t j=0; j< size; j++)
        {
            args[j] = (char*) cmd->args.at(j).c_str();
        }
        args[size] = NULL;

        if (execvp(args[0], args) < 0) {  // error check
            perror("execvp");
            exit(2);
        }
        delete[]args;
    }

        
    else{
        //waitpid(pid,nullptr,0);
        waitpid(pid,nullptr,0);

        }
    }     
    

void process_command(Tokenizer &tknr)
{
    //single
    auto cmd = tknr.commands.at(0);
    int fd[2];
    //int token_size = tknr.commands.size();
    size_t size = cmd->args.size();
    char** args = new char*[size+1];
    //bool bg = false;
    for(size_t j=0; j< size; j++)
    {
        args[j] = (char*) cmd->args.at(j).c_str();
    }
    args[size] = NULL;
    //cout<<args[0]<<endl;

    /*
    if(args[size-1] == "&")
    {
        bg = true;
        cmd.pop_back();
    }
    */
    //check if change directory

    if(!strcmp(args[0],"cd"))
    {
            //previous
            char mockdir[100];
            getcwd(mockdir,sizeof(mockdir));
            string currloc(mockdir);
            if(!strcmp(args[1], "../../"))
            {
                string prev = currloc.substr(0,currloc.find_last_of("/"));
                prev = prev.substr(0,prev.find_last_of("/"));
                chdir((char*) prev.c_str());
            }
            else if(!strcmp(args[1], "-"))
            
            {
                
                chdir((char*) prevfile.c_str());
            }
            else if(!strcmp(args[1], "..") || !strcmp(args[1], "../") )
            {
                string prev = currloc.substr(0,currloc.find_last_of("/"));
                chdir((char*) prev.c_str());

            }
            else{
                //specified directory
                chdir((args[1]));
            }
    }
    else if(!strcmp(args[0],"up"))
    {
        chdir((char*) commandhis.back().c_str());

    }
    else{
    //io redirection
    pipe(fd);
    pid_t pid = fork();
    if (pid ==0)
    {
        if(cmd->hasInput())
        {
            int rd = open(cmd->in_file.c_str(), O_RDONLY); //permissions for child stdin
            if(rd < 0)
            {
                perror("open");
                exit(2);

            }
            if(dup2(rd,0) < 0)
            {
                perror("dup2");
                exit(2);
            }

        }
        if(cmd->hasOutput())
        {
            int wr = open(cmd->out_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR| S_IWUSR);
            if(wr <0)
            {
            
                perror("open");
                exit(2);
            }
            if(dup2(wr,1)<0)
            {
                perror("dup2");
                exit(2);
            }
        }
        if (execvp(args[0], args) < 0) {  // error check
                perror("execvp");
                exit(2);
        }
    }
        
    
    else{
        if(cmd->isBackground())
        {
            PIDS.push_back(pid);
        }
        else{
            int stats;
            waitpid(pid, &stats,0);
        }
    }
    delete [] args;
    }
}

void process_pipe(Tokenizer &tknr){
    // process piped commands

    int fd[2];
    int in = dup(0);
    int out = dup(1);
    size_t token_size = tknr.commands.size();
    for( size_t i = 0; i < token_size; i++)
    {
        //ru
        pipe(fd);
        auto cmd = tknr.commands.at(i);
        if(i == token_size-1)
        {
            if(!(tknr.commands.back()->hasOutput()))
            {
                out = 1;
            }
            // last one does not have output
            // pipeline output to stdout
            //write to file
        }
        else
        {
            //output to file write end
            out = fd[1];
        }
        spawn_processes(in,out,cmd);
        // done with write end of pipe, child wrote so we close
        if(in !=0)
        {
            close(in);
        }
        if(out != 1)
        {
            
            close(out);
        }
        // next process get output from read end of pipe
        
        in = fd[0];
    }

}

int main () {
    //void process_pipe(Tokenizer &);
    //void change_dir();
    

    //vector<int> PIDS;
    char bsstring[100];
    getcwd(bsstring,sizeof(bsstring));
    string prefile(bsstring);
    prevfile = prefile;
    commandhis.push_back(prevfile);
    //cout<< " this is"<< prevfile <<endl;
    //prevfile used for tracking the location

    /*
    char filterfile[100];
    getcwd(filterfile,sizeof(filterfile));
    string prevfile(filterfile);
    */

    //user prompt stuff
    /*
    char readingfirst[100];
    getcwd(readingfirst,sizeof(readingfirst));
    string prev (readingfirst);
    time_t clock;
    time(&clock);
    //@33:30: getlogin() should be getenv("USER")
    string user  = getenv("USER");
   
    */
    // create copies of stin/stdout;
        for (;;) {
        /*

        hostname[1023] = '\0';
        gethostname(hostname,1023);
        // date/time, username , path, etc
        cout << YELLOW << getpwuid(geteuid()) -> pw_name << '@' << hostname << ":" << getcwd(CURRENT_PATH, MAX_PATH) << NC << " ";
        */
        
        // implement iteration over vector of bg pid (vector also declared outside loop)
        // waitpid()- using flag for non-blocking
        check_background();
        cout << YELLOW << "Shell$" << NC << " ";
        // implement date/time with TODO
        // implement username with getlogin()
        // implement curdir with getcwd()
        // need date/time, username, and absolute path to current dir
        char readingfirst[100];
        getcwd(readingfirst,sizeof(readingfirst));
        string currdir (readingfirst);
        time_t clock;
        clock = time(NULL);
        //char *tm = ctime(&clock);
        char *tm = strtok(ctime(&clock), "\n");
        //tm.erase(tm.end()-1);
        //@33:30: getlogin() should be getenv("USER")
        //tm[16] = ' ';
        string user  = getenv("USER");
        cout << YELLOW << tm << " " << user <<":"<< currdir <<"$";
        


        
        // get user inputted command
        string input;
        getline(cin, input);

        if (input == "exit") {  // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }
        // chdir()
        // if dir (cd <dir>) is "-", then go to previous working directory
        // variable storing previous working directory (it needs to be declared outside loop)

        // get tokenized commands from user input
        Tokenizer tknr(input);
        if (tknr.hasError()) {  // continue to next prompt if input had an error
            continue;
        }
        // print out every command token-by-token on individual lines
        // prints to cerr to avoid influencing autograder
        // for (auto cmd : tknr.commands) {
        //     for (auto str : cmd->args) {
        //         cerr << "|" << str << "| ";
        //     }
        //     if (cmd->hasInput()) {
        //         cerr << "in< " << cmd->in_file << " ";
        //     }
        //     if (cmd->hasOutput()) {
        //         cerr << "out> " << cmd->out_file << " ";
        //     }
        //     cerr << endl;
        // }
        // add check for bg process - add pid to vector if bg and dont waitpid() in parent

        // for piping

        //for cmd : commands
 
        // call pipe() to make pipe
        // fork() - in child, redirect stdout, in par, redirect stdin
        // ^ is already written (:))
        // add checks for first/last command

        // fork to create child
        if(tknr.commands.size()< 2){
            //ls runs
            process_command(tknr);
        }
        else
        {
            process_pipe(tknr);

        }
        /*

        pid_t pid = fork();
        if (pid < 0) {  // error check
            perror("fork");
            exit(2);
        }
        // add check for bg process - add pid to vector if bg and don't waitpid() in par

        if (pid == 0) {  // if child, exec to run command
            // run single commands with no arguments
            // implement multiple arguements - iterate over args of current command to make
            // char * array into execvp
            char* args[] = {(char*) tknr.commands.at(0)->args.at(0).c_str(), nullptr};
            // if current command is redirected, then open file and dup2 std(in/out)* thats being redirected
            //implement it safey for both at same time

            if (execvp(args[0], args) < 0) {  // error check
                perror("execvp");
                exit(2);
            }
        }
        else {  // if parent, wait for child to finish
            int status = 0;
            waitpid(pid, &status, 0);
            if (status > 1) {  // exit if child didn't exec properly
                exit(status);
            }
        }
        */

        // restore stin.stdout (varable would be outside the loop)
        

    }
}

