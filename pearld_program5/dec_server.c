#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>


#define SIZE 500000

void reject_bad_chars(char* buffer)
{
    int len = strlen(buffer);
    char chars[27] = { 'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',' ' };
    for (int i = 0; i < len; i++)
    {
        for (int j = 0; j < 27; j++)
        {
            if (buffer[i] == chars[j])
                break;
            if (j == 26)
                error("at least one invalid char");
        }
    }
}
// Error function used for reporting issues
void error(const char* msg) {
    perror(msg);
    exit(1);
}
void decode(char* message, char* key, char* decoded, int length)
{
    //create an integer representation for message
    int message_ints[length];
    //get the integer representation of the message
    for (int i = 0; i < length; i++)
        if (message[i] == ' ')
            message_ints[i] = 26;
        else
            message_ints[i] = (int)message[i] - 65; //convert ascii to A = 0, B = 1 etc.

    //create an integer representation of the key
    int key_ints[length];

    //get the integer representation of the key
    for (int i = 0; i < length; i++)
        if (key[i] == ' ')
            key_ints[i] = 26;
        else
            key_ints[i] = (int)key[i] - 65;

    //get the integer representation of message - key
    int message_minus_key[length];
    for (int i = 0; i < length; i++)
        message_minus_key[i] = message_ints[i] - key_ints[i];
    for (int i = 0; i < length; i++)
        if (message_minus_key[i] < 0)
            message_minus_key[i] += 27;

    for (int i = 0; i < length; i++)
    {
        decoded[i] = (char)(message_minus_key[i] % 27 + 65);
        if (decoded[i] == '[') //manually change this character to space
            decoded[i] = ' ';
    }
}
// Set up the address struct for the server socket
// Set up the address struct for the server socket


// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address,
    int portNumber) {

    // Clear out the address struct
    memset((char*)address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);
    // Allow a client at any address to connect to this server
    address->sin_addr.s_addr = INADDR_ANY;
}

int main(int argc, char* argv[]) {
    int connectionSocket, charsRead;
    char buffer[SIZE];
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);

    // Check usage & args
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

    // Create the socket that will listen for connections
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        error("ERROR opening socket");
    }

    // Set up the address struct for the server socket
    setupAddressStruct(&serverAddress, atoi(argv[1]));

    // Associate the socket to the port
    if (bind(listenSocket,
        (struct sockaddr*)&serverAddress,
        sizeof(serverAddress)) < 0) {
        error("ERROR on binding");
    }

    // Start listening for connetions. Allow up to 5 connections to queue up
    listen(listenSocket, 5);
    int status;
    // Accept a connection, blocking if one is not available until one connects
    while (1) {
        // Accept the connection request which creates a connection socket
        connectionSocket = accept(listenSocket,
            (struct sockaddr*)&clientAddress,
            &sizeOfClientInfo);
        if (connectionSocket < 0) {
            error("ERROR on accept");
        }

        pid_t pid = fork();
        if (pid < 0)
            error("you have a forking error");
        if (pid == 0)
        {
            printf("SERVER: Connected to client running at host %d port %d\n",
                ntohs(clientAddress.sin_addr.s_addr),
                ntohs(clientAddress.sin_port));

            // Get the message from the client and display it
            memset(buffer, '\0', SIZE);
            // Read the client's message from the socket
            charsRead = recv(connectionSocket, buffer, SIZE - 1, MSG_WAITALL);//waits for a huge buffer
            if (charsRead < 0) {
                error("ERROR reading from socket");
            }
            //printf("SERVER: I received this from the client: \"%s\"\n", buffer);

            int total = 0;

            //see how many meaningful characters there are
            for (int i = 0; i < SIZE; i++)
            {
                if (buffer[i] == '\b')
                {
                    total = i;
                    break;
                }

            }
            //clean the rest of the buffer
            for (int i = total; i < SIZE; i++)
                buffer[i] = '\0';

            char key[SIZE];
            char message[SIZE];
            char password[SIZE];
            char decoded[SIZE];

            const char delim[2] = "\n";

            char* token = strtok(buffer, delim);
            strcpy(password, token);

            if (strcmp(password, "dec_client") != 0)
            {
                fprintf(stderr, "attempting to make an unauthorized connection, will only accept dec_client");
                exit(2);
            }

            token = strtok(NULL, delim);
            strcpy(message, token);

            token = strtok(NULL, delim);
            strcpy(key, token);

            int encoded_length = 0;

            reject_bad_chars(message);
            reject_bad_chars(key);

            decode(message, key, decoded, strlen(message));

            charsRead = send(connectionSocket,
                decoded, strlen(decoded) + 1, 0); 
            if (charsRead < 0) {
                error("ERROR writing to socket");
            }
            // Close the connection socket for this client
            exit(0);
        }
        close(connectionSocket);

        wait(NULL);//I got this on stackoverflow somewhere, not sure where, it is I think equivalent to waitpaid(-1, pid, &status, 0)
        //necessary to wait for child processes to finish, otherwise this parent process would exit the program before anything happened.
        connectionSocket = -1;/*not really sure about this one...I tried many different things to prevent the second client run on the same port
                              from freezing, something that only started happening after I put in process forking. I thought the connection
                              was remaining somehow so future runs couldn't connect. I don't know why connectionSocket doesn't reset
                              when accept is called again if the while loop continues. I basically just thought am I doing everything I can
                              to make sure the connection is invalid after the program runs...This seemed to be what did the trick. I'd be interested
                              to find out more about why this works. I know the socket doesn't close in the child process but I don't
                              think trying that worked...it was only this. Shouldn't closing the socket be adequate? I guess not*/
    }
    // Close the listening socket
    close(listenSocket);
    return 0;
}
