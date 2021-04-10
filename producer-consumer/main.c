#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

/*This assignment was adapted from example code (6.5) and made to pass chars between buffers, instead of ints/doubles
The producer and consumer functions are mostly identical to the example, except where they needed to be adapted to change data
type, or in the case of one of the consumers (get_buff_2), do the plus sign manipulation. I know the prompt states that the 
thread is supposed to do this manipulation, but the thread is calling the function, so the action happens within the action of the thread
*/

#define STOP '\b' //stop signal

int plus_signs = 0;
pthread_mutex_t plus_signs_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t has_outputted = PTHREAD_COND_INITIALIZER;

int has_stopped = 0;

#define SIZE 50000

char buffer_1[SIZE];
int count_1 = 0;
int prod_idx_1 = 0;
int con_idx_1 = 0;
pthread_mutex_t mutex_1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full_1 = PTHREAD_COND_INITIALIZER;

char buffer_2[SIZE];
int count_2 = 0;
int prod_idx_2 = 0;
int con_idx_2 = 0;
pthread_mutex_t mutex_2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full_2 = PTHREAD_COND_INITIALIZER;

char buffer_3[SIZE];
int count_3 = 0;
int prod_idx_3 = 0;
int con_idx_3 = 0;
pthread_mutex_t mutex_3 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full_3 = PTHREAD_COND_INITIALIZER;

/*Consumers lock the appropropriate buffer's mutex, wait via while loop for the count to not be zero, find out about this change in count
via a conditional variable, take the copy the char, update the count and index, unlock the mutex, and return the char.

Producers lock the appropriate buffer's mutex, send a signal to any waiting thread that the buffer is no longer empty, put a passed in char into 
the appropriate buffer, update the count and index, and unlock the mutex*/

int get_buff_1() {

    pthread_mutex_lock(&mutex_1);
    while (count_1 == 0)
        // Buffer is empty. Wait for the producer to signal that the buffer has data
        pthread_cond_wait(&full_1, &mutex_1);
    char item = buffer_1[con_idx_1];
    con_idx_1 = con_idx_1 + 1;
    count_1--;
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_1);
    // Return the item
    return item;
}

char get_buff_2() {
    // Lock the mutex before checking if the buffer has data
    pthread_mutex_lock(&mutex_2);
    while (count_2 == 0)
        // Buffer is empty. Wait for the producer to signal that the buffer has data
        pthread_cond_wait(&full_2, &mutex_2);
    char item;
    if (buffer_2[con_idx_2] == '+' && buffer_2[con_idx_2] == '+')
    {
        item = '^';
        con_idx_2 = con_idx_2 + 2;
        count_2--;
        count_2--;
    }
    else
    {
        item = buffer_2[con_idx_2];
        // Increment the index from which the item will be picked up
        con_idx_2 = con_idx_2 + 1;
        count_2--;
    }
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_2);
     // Return the item
    return item;
}

char get_buff_3() {
    // Lock the mutex before checking if the buffer has data
    pthread_mutex_lock(&mutex_3);
    while (count_3 == 0)
        // Buffer is empty. Wait for the producer to signal that the buffer has data
        pthread_cond_wait(&full_3, &mutex_3);
    char item = buffer_3[con_idx_3];
    // Increment the index from which the item will be picked up
    con_idx_3 = con_idx_3 + 1;
    count_3--;
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_3);
    // Return the item
    return item;
}

void put_buff_1(int item) {
    // Lock the mutex before putting the item in the buffer
    pthread_mutex_lock(&mutex_1);
    // Put the item in the buffer
    buffer_1[prod_idx_1] = item;
    // Increment the index where the next item will be put.
    prod_idx_1 = prod_idx_1 + 1;
    count_1++;
    // Signal to the consumer that the buffer is no longer empty
    pthread_cond_signal(&full_1);
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_1);
}

void put_buff_2(char item) {
    // Lock the mutex before putting the item in the buffer
    pthread_mutex_lock(&mutex_2);
    // Put the item in the buffer
    buffer_2[prod_idx_2] = item;
    // Increment the index where the next item will be put.
    prod_idx_2 = prod_idx_2 + 1;
    count_2++;
    // Signal to the consumer that the buffer is no longer empty
    pthread_cond_signal(&full_2);
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_2);
}

void put_buff_3(char item) {
    // Lock the mutex before putting the item in the buffer
    pthread_mutex_lock(&mutex_3);
    // Put the item in the buffer
    buffer_3[prod_idx_3] = item;
    // Increment the index where the next item will be put.
    prod_idx_3 = prod_idx_3 + 1;
    count_3++;
    // Signal to the consumer that the buffer is no longer empty
    pthread_cond_signal(&full_3);
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_3);
}

/*initializes a char buffer to hold an input of up to 1000 characters, enters a sentinel controlled while loop, gets chars with a while loop
until a newline or the char limit is detected. Then it tests whether the first 5 characters are the stop signal. The previous character before the start
of the input buffer will always have been a newline, so the requirement that the stop signal be met with a newline is met. If the stop signal
is detected, the sentinel is set to break the loop so another line of input will not be read. If STOP\n was detected, a '\b' character is sent down the pipe
that other threads will pick up and end their while loops. The pipeline after input is read operates one character at a time, so any input before the stop
character will make it to the next stage before a thread terminates, and any input after the stop character is irrelevant. This type of stop signal
was not my idea but rather one of the TA's, including the idea to have it be a character outside of the normal ASCII range of input. For the every character in the input thread's
buffer, as determined by a count set by the initial loop reading input, that character is sent to the first buffer.*/
void* input_thread_function(void* args)
{   
    int count = 0;
    char input[1000];
    int flag = 1;
    while (flag == 1)
    {
        char c = '0';
        while (c != '\n' && count < 1000)
        {
            c = getchar();
            input[count] = c;
            count++;
        }

        if(count == 5)
            if (input[0] == 'S' &&
                input[1] == 'T' &&
                input[2] == 'O' &&
                input[3] == 'P' &&
                input[4] == '\n')
                flag = 0;

        for (int i = 0; i < count; i++)
        {
            char item = input[i];
            if (flag == 0)
                put_buff_1(STOP);
            else
                put_buff_1(item);
            fflush(stdin); //putting this here as a guess fixed a probably with inconsistent output and I have no idea why
        }
        for (int i = 0; i < 1000; i++)
            input[i] = NULL;

        count = 0;
    }
    return NULL;
}
/*extremely simple operation to get a character via a consumer function from the first buffer, replace a newline with a space as necessary,
and pass it to the 2nd buffer via a producer*/
void* line_separator_thread_function(void* args)
{
    int flag = 1;
    char item;
    while (flag == 1)
    {
        item = get_buff_1();

        if (item == '\n')
            item = ' ';

        put_buff_2(item);

        if (item == STOP)
        {
            flag = 0;
        }
    }
    return NULL;
}
/*extremely simple thread that gets a character from the second buffer and passes it to the third. Technically, this thread, by calling the consumer function
that does the actual work, replaces a ++ with a ^*/
void* plus_sign_thread_function(void* args)
{
    char item;
    int flag = 1;
    while (flag == 1)
    { 
        item = get_buff_2();
        put_buff_3(item);
        if (item == STOP)
        {
            flag = 0;
        }
    }
    return NULL;
}
/*this thread initializes a buffer for output. It enters a sentinel controlled while loop. It gets a char via a consumer from buffer 3, and puts it into its output buffer.
Each time it does this it increments a counter, and when the counter hits 80, it enters a loop that outputs each character, clears the buffer, resets the count. In this
manner only 80 char lines can be outputted, so anything less won't be, as the assginment requires.*/
void* output_thread_function(void* args)
{
    int output_count = 0;
    char output_buffer[SIZE];
    char line_buffer[81];
    char item;
    int flag = 1;
    while (flag == 1)
    {
        item = get_buff_3();

        line_buffer[output_count] = item;
        output_count++;

        if (output_count == 80)
        {
            line_buffer[80] = '\n';
            for (int j = 0; j < 81; j++)
                if (line_buffer[j] != NULL)
                    printf("%c", line_buffer[j]);
            for (int j = 0; j < 81; j++)
                line_buffer[j] = NULL;
            output_count = 0;
        }

        if (item == STOP)
        {
            flag = 0;
        }
    }
    return NULL;
}
//copied from example 6.5, with added thread and changed names as necessary
int main()
{
    srand(time(0));
    pthread_t input_thread, line_separator_thread, plus_sign_thread, output_thread;
    // Create the threads
    pthread_create(&input_thread, NULL, input_thread_function, NULL);
    pthread_create(&line_separator_thread, NULL, line_separator_thread_function, NULL);
    pthread_create(&plus_sign_thread, NULL, plus_sign_thread_function, NULL);
    pthread_create(&output_thread, NULL, output_thread_function, NULL);
    // Wait for the threads to terminate
    pthread_join(input_thread, NULL);
    pthread_join(line_separator_thread, NULL);
    pthread_join(plus_sign_thread, NULL);
    pthread_join(output_thread, NULL);

    return EXIT_SUCCESS;
}
