#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>


#define MAX_ARGUMENT_SIZE 2048
#define MAX_ARGUMENTS 512
char *arguments[MAX_ARGUMENTS];

int processes[200];
int process_count = 0;
int background = 0; 
int background_allowed = 1;

void print_status(int pid);


/*Function replaceWord
    THIS IS NOT MY CODE. I copied it from https://www.geeksforgeeks.org/c-program-replace-word-text-another-given-word/ because
    I needed a way to replace characters in a string as part of the variable expansion requirement. I searched a long time for
    how to implement this myself, and kept running into trouble. I eventually found strstr which returns a the first occurence of a 
    designated substring in the original string. This function uses strstr to 
    string with a designated set of char replaced in another designated set of chars.
    
    I sought to understand it as best i could and attempt to explain it so I'm not as blindly copying code*/
char* replaceWord(const char* s, const char* oldW,
    const char* newW)
{
    char* result;
    int i, cnt = 0;
    int newWlen = strlen(newW);
    int oldWlen = strlen(oldW);

    // Counting the number of times old word 
    // occur in the string 
    /*This part is straightforward enough. It counts the number of replacements
    by using strstr to find the first occurence of the word to be replaced, and since
    each time through it puts the iterator to the index after the found old word, it then
    calls strstr again, with the index passed in and I *think* it is calling strstr on the string comprising the rest
    of the string after that index. For each pass, i is incremented and then if there is a hit on the found word, then
    it is placed after the found word for the next go-round.*/
    for (i = 0; s[i] != '\0'; i++) {
        if (strstr(&s[i], oldW) == &s[i]) {
            cnt++;

            // Jumping to index after the old word. 
            i += oldWlen - 1;
        }
    }
    /*So the previous seems all about knowing how much memory to allocate, and this part I am having trouble getting
    I get that count * (new length - old length) is the amount of memory that needs to be allocated *in excess* of the previous
    word. But i ended up one after the last appearance of the word to be replaced. So I don't know why the new size wouldn't just be
    the old size plus the new size needed (count * (new length - old length)). i is an arbitrary index that is the index
    after the last appearance of the old word but I don't see how this factors into the size determination because there could be a long string
    with something replaced near the beginning, so I don't know how this would factor into the size for the end result. It seems like it should
    have iterated through s to find the size of the original, and then added the extra needed size. It's probable that it is doing this, but I don't
    see it.*/
    result = (char*)malloc(i + cnt * (newWlen - oldWlen) + 1);

    /*Here we actually build the new string. The iterating seems to be done with pointer/memory arithmatic
    It is iterating through the original string, s, making the comparisons with strstr, and either copying an entire
    word, aftr which addjustments are made to the iterators according to the size of the word, in order to skip past that
    word and continue copying char by char. The result string copies the new word, so it skips ahead by the length of the new word it just received,
    while the old string skips ahead by the length of the old word it just compared. If the current index of s doesn't hold the
    index of the next appearance of the word to be tested, it does char by char copying into the result. I don't really understand
    how s is being iterated through but I think it is happening with memory addresses*/
    i = 0;
    while (*s) {
        // compare the substring with the result 
        if (strstr(s, oldW) == s) {
            strcpy(&result[i], newW);
            i += newWlen;
            s += oldWlen;
        }
        else
            result[i++] = *s++;
    }

    result[i] = '\0'; //makes it null terminated
    return result;
}

//pasted from example
void handle_SIGINT(int signo){
	char* message = "Caught SIGINT, sleeping for 10 seconds\n";
	write(STDOUT_FILENO, message, 39);
  // Sleep for 10 seconds
	//sleep(10);
}

// Handler for SIGSTP
void SIGTSTP_handler(){

    /*program defaults to background being allowed, and this is tracked by a global variable
    this function catches SIGTSTP and delivers the following behavior depending on if the program
    is currently in foreground mode. The background_allowed int is treated like a boolean, switching
    to the other mode from which it is currently in*/
    if (background_allowed == 1)
    {
        char* informative_message = "\nEntering foreground - only mode(&is now ignored)\n";
        write(1, informative_message, 50);
        fflush(stdout);
        background_allowed = 0;
    }
    else if (background_allowed == 0)
    {
        char* informative_message = "\nExiting foreground-only mode\n";
        write(1, informative_message, 30);
        fflush(stdout);
        background_allowed = 1;
    }
}

//
void print_status(int status)
{
    if (WIFEXITED(status)) {
        printf("exited normally with status %d\n", WEXITSTATUS(status));
    }
    else {
        printf("exited abnormally due to signal %d\n", WTERMSIG(status));
    }
}

int main(int argc, char **argv)
{

    struct sigaction SIGINT_action = {0}, ignore_action = {0}, SIGTSTP_action = {0};

  // Fill out the SIGINT_action struct
  // Register handle_SIGINT as the signal handler
	SIGINT_action.sa_handler = SIG_IGN; //sets the SIGINT to ignore
    SIGTSTP_action.sa_handler = SIGTSTP_handler; //registers the handler for SIGTSTP
  // Block all catchable signals while handle_SIGINT is running
	sigfillset(&SIGINT_action.sa_mask);
    sigfillset(&SIGTSTP_action.sa_mask);


  // Block all catchable signals while handle_SIGUSR2 is running
  // No flags set
     SIGTSTP_action.sa_flags = SA_RESTART; //added to address segfault issue, was suggested on Piazza
     SIGINT_action.sa_flags = 0;


  // The ignore_action struct as SIG_IGN as its signal handler
	ignore_action.sa_handler = SIG_IGN;

  // Register the ignore_action as the handler for SIGTERM, SIGHUP, SIGQUIT. So all three of these signals will be ignored.

 	sigaction(SIGINT, &ignore_action, NULL); //sets the default action for SIGINT to the ignore action
 	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    char *input_file, *output_file; //for storing filenames
    int input_redirect, output_redirect = 0; //flags for whether redirection is specified

    static int status = 0;

    do
    {
        // Child process
        struct sigaction sigint_action = { 0 }; //making sure signal handler is setup, probably redundant, but it doesn't break, afraid to delete
        sigint_action.sa_handler = SIG_IGN;
        sigaction(SIGINT, &sigint_action, 0); 

        printf(": "); //shell prompt
        char *line_buffer = NULL;
        ssize_t size = MAX_ARGUMENT_SIZE; 
        getline(&line_buffer, &size, stdin);
    
        line_buffer[size-1] = '\0'; //get rid of trailing newline
  
        char** args = malloc(MAX_ARGUMENTS * sizeof(char*)); //arg buffer
  
        const char delim[3] = " \n";//separate by spaces and newline
        char *token;
        int pid_read = getpid(); //we're gonna convert the pid to a string
        char pid_string[1000]; //and this is part of that conversion
        token = strtok(line_buffer, delim);  //standard strtok tokening
       
        /* get the first token */
   
        /* walk through other tokens */
        int count = 0; 
        while( token != NULL ) {
            snprintf(pid_string, 1000, "%d", pid_read); //found this somewhere, can't find at the moment

            token = replaceWord(token, "$$", pid_string); //this is a function I found that does what the name says, explained above

            if (strcmp(token, "<") == 0) //if input redirection is sepcified
            {
                token = strtok(NULL, delim); //we want the *next* token for the filename
                if (token != NULL)
                {
                    input_redirect = 1; //set the flag
                    input_file = token; //set the filename
                   // printf("input file %s", input_file);
                    token = strtok(NULL, delim); //very important to send it to the next token so the filename doesn't fall into args
                    //next time through the loop
                    continue;//because we are continuing out of it, also to prevent this token from falling into args
                    //actually maybe I don't need continue then, because it is already on the next token, and it won't specify redirection,
                    //hopefully...anyway I'm not gonna change it at this late hour, just in case.
                    //took me awhile to realizing 
                }
            }
            else if (strcmp(token, ">") == 0) //see above
            {
                token = strtok(NULL, delim);
                if (token != NULL)
                {
                    output_redirect = 1;
                    output_file = token;
                    printf("output file %s ", output_file);
                    token = strtok(NULL, delim);
                    continue;
                }
            }

          
            args[count] = token; //put the token in args
            count++;
            token = strtok(NULL, delim);
        }
     
        if(count == 0) //empty command
        {
        status = 1;
        continue;
        }
   
   //built in commands
        char *temp = args[0];
        if(strcmp(args[0],"#") == 0 || (temp[0] == '#')) //if it's a comment
        {
            for(int i = 0; i < count; i++)
            printf("%s", args[i]);
            continue; //continue through the rest of the giant while loop
        }
   
        if(strcmp(args[0],"cd") == 0)
            if(args[1] == NULL)
            {
                char *home = getenv("HOME"); //standard built in cd
                chdir(home);
                status = 1;
                continue;
            } 
            else
            {
                chdir(args[1]);
                status = 1;
                continue;
            }
	
	        if(strcmp(args[0], "exit") == 0)
	        {
	        	status = 0;
                for (int i = 0; i < 200; i++)
                {
                    kill(processes[i], SIGKILL); //goes through a global array of process, kills them
                    printf("killed process %d\n", processes[i]);
                }
		        exit(0);
	        }
 
            if(strcmp(args[0], "status") == 0)
            {
                print_status(status); //passes in the status of current proccess
                continue;
            }
      
            if (strcmp(args[count - 1], "&") == 0)
            {
                background = 1;
                args[count - 1] = NULL; //required to get the & out of args
                count--;
            }
     
        pid_t pid;    
        int pid_status; //don't think this is used, afraid to delete
    
   
        pid = fork();
    
        processes[process_count] = pid;
        process_count++;
    
        if (pid == 0) {
            // Child process
            if (background == 0)
            {
                sigint_action.sa_handler = SIG_DFL; //if a background child proccess, set SIGINT to work normally
                sigaction(SIGINT, &sigint_action, NULL);
            }

      //copied from redirect IO example

            int fd0, fd1;

            if (background == 1 && input_redirect == 0) //handle dev/null according to conditions
            {
                fd0 = open("/dev/null", O_RDONLY);
                dup2(fd0, STDIN_FILENO);
            }
            if (background == 1 && output_redirect == 0)
            {
                fd1 = creat("/dev/null", 0644);
                dup2(fd1, STDOUT_FILENO);
            }


            if (input_redirect == 1)
            {
                int sourceFD = open(input_file, O_RDONLY);
                if (sourceFD == -1) {
                    perror("source open()");
                    exit(1);
                }

                // Redirect stdin to source file
                int result = dup2(sourceFD, 0);
                if (result == -1) {
                    perror("source dup2()");
                    exit(2);
                }
            }

            if (output_redirect == 1)
            {
                // Open target file
                int targetFD = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (targetFD == -1) {
                    perror("target open()");
                    exit(1);
                }

                // Redirect stdout to target file
                int result = dup2(targetFD, 1);
                if (result == -1) {
                    perror("target dup2()");
                    exit(2);
                }
            }
            if (execvp(args[0], args) == -1) {
                perror("execvp error");
            }
            exit(EXIT_FAILURE);
       }else if (pid < 0) {
    // Error forking
            perror("forking error");
            exit(1);
      }else{
    // Parent process
           do {
               if (background == 1 && background_allowed == 1)
               {
                   pid = waitpid(-1, &status, WNOHANG); //wait for child proccesses
                   printf("background process %d started\n", pid);
                   fflush(stdout);
               }
               else
               {
  
                    waitpid(pid, &status, WUNTRACED); //not sure what WUNTRACED is, got it from
                    //a shell tutorial //https://brennan.io/2015/01/16/write-a-shell-in-c/
                    //and my program seems to break without it

                    //should note that I learned a lot from the above tutorial, intially copied fork()/execvp code
                    //as a skeleton to then alter if/as needed. 
      
                    if (WTERMSIG(status) != 0) //if terminated, report it
                    {
                        printf("terminated by signal %d", WTERMSIG(status));
                    }
                    do
                    {
                        pid = waitpid(-1, &status, WNOHANG); //wait for children
                        if (pid > 0)
                        {
                            if (WIFEXITED(status) != 0) //report if exiting normally or terminated by signal
                            {
                                printf("background process %d exiting with value %d\n", pid, WEXITSTATUS(status));
                            }
                            else if (WIFSIGNALED(status) != 0)
                            {
                                printf("background process %d terminated by signal %d\n", pid, WTERMSIG(status));
                            }
                        }
                    } while (pid > 0);

      
      }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

    
    
  //check for redirection
  
  input_redirect = 0;
  output_redirect = 0;
  background = 0; //reset flags
  
  }while(1); //loop forever because exit breaks it
  
  
	
  return EXIT_SUCCESS;
}



