#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()

#define SIZE 500000

/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// Error function used for reporting issues
void error(const char* msg) {
    perror(msg);
    exit(0);
}
/*This function takes a char buffer and exits the program if it contains invalid characters. It creates an array of
approved characters, and uses nested for loops to compare each character in the buffer to the approved array. If the buffer character
matches a character in the approved array, the loop breaks and the next character is compared. If any character
reaches the end of the approved array (see below, j == 26) that means the character was not one of the approved characters, and
the program exits. Placing this after the break statement ensures that had the char equalled the last approved character, the loop
would have broke before the exit is triggered.*/
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
// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address,
    int portNumber,
    char* hostname) {

    // Clear out the address struct
    memset((char*)address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);

    // Get the DNS entry for this host name
    struct hostent* hostInfo = gethostbyname(hostname);
    if (hostInfo == NULL) {
        fprintf(stderr, "CLIENT: ERROR, no such host\n");
        exit(0);
    }
    // Copy the first IP address from the DNS entry to sin_addr.s_addr
    memcpy((char*)&address->sin_addr.s_addr,
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}
/*the sample code was altered to take in the proper arguments from the command line.
A password to ensure against an improper connection is placed in the buffer, along with a message and a key, separated by newlines
for delimiting when received by server. The entire buffer is sent. Junk (the char '\b') is placed into the buffer in order to fill it
to its max size so we can know how many characters we are sending and receiving. Terribly inefficient, but it works. I tried various
methods like sending a size, reading the index of a termination character to break a receive loop, couldn't get anything to work except this.*/
int main(int argc, char* argv[]) {
    int socketFD, portNumber, charsWritten, charsRead;
    struct sockaddr_in serverAddress;
    char buffer[SIZE];
    // Check usage & args
    if (argc < 3) {
        fprintf(stderr, "USAGE: %s hostname port\n", argv[0]);
        exit(0);
    }

    // Create a socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0) {
        error("CLIENT: ERROR opening socket");
    }

    // Set up the server address struct
    setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

    // Connect to server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        error("CLIENT: ERROR connecting");
    }
    // Get input message from user
    // Clear out the buffer array
    // Get input from the user, trunc to buffer - 1 chars, leaving \0
    // Remove the trailing \n that fgets adds
    char message[SIZE];
    FILE* message_file = fopen(argv[1], "r");
    memset(message, '\0', sizeof(message));
    fgets(message, sizeof(message) - 1, message_file);
    int message_length = strlen(message);
    message[strcspn(message, "\n")] = '\0';


    char key[SIZE];
    FILE* key_file = fopen(argv[2], "r");
    memset(key, '\0', sizeof(key));
    fgets(key, sizeof(key) - 1, key_file);
    int key_length = strlen(key);
    key[strcspn(key, "\n")] = '\0';

    char password[200000] = "enc_client";
    password[strlen(password)] = '\0';
    char password_length = strlen(password);

    if (key_length < message_length)
    {
        fprintf(stderr, "need a longer key");
        exit(1);
    }
 //   reject_bad_chars(key);
  //  reject_bad_chars(message);

    sprintf(buffer, "%s\n%s\n%s", password, message, key);

    int buffer_length = message_length + key_length + password_length;

    

    for (int i = buffer_length + 1; i < SIZE; i++)
        buffer[i] = '\b';

    // Send message to server
    // Write to the server
    charsWritten = send(socketFD, buffer, SIZE, 0);
    if (charsWritten < 0) {
        error("CLIENT: ERROR writing to socket");
    }
    /*if (charsWritten < strlen(buffer)) {
        printf("CLIENT: WARNING: Not all data written to socket!\n");
        printf("chars written %d\n", charsWritten);
    }*/

    // Get return message from server
    // Clear out the buffer again for reuse
    memset(buffer, '\0', sizeof(buffer));
    // Read data from the socket, leaving \0 at end
    charsRead = recv(socketFD, buffer, strlen(message), MSG_WAITALL);
    if (charsRead < 0) {
        error("CLIENT: ERROR reading from socket");
    }
    printf("%s\n", buffer);

    // Close the socket
    close(socketFD);
    return 0;
}