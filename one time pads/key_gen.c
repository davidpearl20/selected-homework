#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


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
			message_minus_key[i] += 26;

	for (int i = 0; i < length; i++)
		if (message_minus_key[i] % 27 == 26)
			decoded[i] = ' ';
		else
			decoded[i] = (char)(message_minus_key[i] % 26 + 65);

}
int main(int argc, char* argv[])
{
	srand(time(NULL));
	int len = atoi(argv[1]);
	char chars[27] = { 'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',' ' };
	char letter_key[len + 1];
	for (int i = 0; i < len; i++)
	{
		int rand_num = rand() % 27;
		letter_key[i] = chars[rand_num];
	}
	letter_key[len] = '\0';

	for (int i = 0; i < len; i++)
		printf("%c", letter_key[i]);
	
		
	return 0;
}

