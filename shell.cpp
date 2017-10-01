/**
  * Shell
  * Operating Systems
  */

/**
  Hint: F2 (or Control-click) on a functionname to go to the definition
  Hint: F1 while cursor is on a system call to lookup the manual page (if it does not work, open a shell and type "man [systemcall]")
  Hint: Ctrl-space to auto complete functions and variables
  */

// function/class definitions you are going to use
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/param.h>
#include <signal.h>
#include <string.h>

#include <vector>

// although it is good habit, you don't have to type 'std::' before many objects by including this line
using namespace std;

int status;

// Parses a string to form a vector of arguments. The seperator is a space char (' ').
std::vector<std::vector<std::string>> parseArguments(const std::vector<std::string>& pipes) {
    std::vector<std::vector<std::string>> retval;
    for (const std::string& str : pipes){
        std::vector<std::string> tempval;
        for (int pos = 0; pos < str.length(); ) {
            // look for the next space
            int found = str.find(' ', pos);
            // if no space was found, this is the last word
            if (found == std::string::npos) {
                if (str.substr(pos).compare("exit")==0) { exit(0); }
                tempval.push_back(str.substr(pos));
                break;
            }
            // filter out consequetive spaces
            if (found != pos)
                tempval.push_back(str.substr(pos, found-pos));
                if (str.substr(pos, found-pos).compare("exit")==0) { exit(0); }
            pos = found+1;
        }
        retval.push_back(tempval);
    }
    return retval;
}

// Parses a string to form a vector of programs that will be piped. The seperator is a pipe char ('|').
std::vector<std::string> parsePipes(const std::string& str) {
    std::vector<std::string> retval;
    for (int pos = 0; pos < str.length(); ) {
        // look for the next space
        int found = str.find('|', pos);
        // if no space was found, this is the last word
        if (found == std::string::npos) {
            retval.push_back(str.substr(pos));
            break;
        }
        // filter out consequetive spaces
        if (found != pos)
            retval.push_back(str.substr(pos, found-pos));
        pos = found+1;
    }
    return retval;
}

// Executes a command with arguments. In case of failure, returns error code.
int executeCommand(const std::vector<std::string>& args) {
    if (args.size() == 0)
        return EINVAL;
    // build argument list
    // always start with the command itself
    // always terminate with a NULL pointer
    char** c_args = new char*[args.size()+1];
    for (int i = 0; i < args.size(); ++i)
        c_args[i] = strdup(args[i].c_str());
    c_args[args.size()] = NULL;

    // replace current process with new process as specified
    execvp(c_args[0], c_args);
    // if we got this far, there must be an error
    int retval = errno;

    // in case of failure, clean up memory
    for (int i = 0; i < args.size(); ++i)
        free(c_args[i]);
    delete c_args;

    return retval;
    exit(0);
}

// Executes multiple piped commands with arguments. In case of failure, returns error code.
int executePipedCommand(const std::vector<std::vector<std::string>>& args, int l) {

    int     fd[2];
    pid_t   childpid;

    pipe(fd);

    if((childpid = fork()) == -1)
    {
            perror("fork");
            exit(1);
    }

    if(childpid == 0)
    {
            /* Child process closes up input side of pipe */
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            /* Send "string" through the output side of pipe */
            if(l!=0){
                 executePipedCommand(args,l-1);
            }else{
                executeCommand(args[l]);
            }
            exit(0);
    }
    else
    {
            /* Parent process closes up output side of pipe */
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);
            executeCommand(args[l]);
            std::cout << "error\n";
    }

    exit(0);

}


int handleCommand(const std::vector<std::vector<std::string>>& args){


    pid_t   childpid;

    if((childpid = fork()) == -1)
    {
            perror("fork");
            exit(1);
    }

    if(childpid == 0)
    {
        if(args.size()>1){
            executePipedCommand(args, (args.size()-1));
            exit(0);
        }else{
            executeCommand(args[0]);
            exit(0);
        }
    }
    return(0);
}

void displayPrompt() {
    char buffer[512];
    char* dir = getcwd(buffer, sizeof(buffer));
    if (dir)
        std::cout << "\e[32m" << dir << "\e[39m"; // the strings starting with '\e' are escape codes, that the terminal application interpets in this case as "set color to green"/"set color to default"
    std::cout << "$ ";
    std::flush(std::cout);
}

std::string requestCommandLine(bool showPrompt) {
    if (showPrompt)
        displayPrompt();
    std::string retval;
    getline(std::cin, retval);
    return retval;
}

int shell(bool showPrompt) {
    while(cin.good()){//cin.good()
        int rc = 0;
        std::string commandLine = requestCommandLine(showPrompt);
        std::vector<std::string> pipes = parsePipes(commandLine);
        std::vector<std::vector<std::string>> argList = parseArguments(pipes);

        rc = handleCommand(argList);
        waitpid(-1, &status, false);

        if (rc != 0){
            std::cout << strerror(rc) << std::endl;
        }
    }
    return 0;
}


