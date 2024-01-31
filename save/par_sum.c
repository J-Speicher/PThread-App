/*
 * sum.c
 *
 * CS 470 Project 1 (Pthreads)
 * Serial version
 *
 * Compile with --std=c99
 */

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// aggregate variables
volatile long sum = 0;
volatile long odd = 0;
volatile long min = INT_MAX;
volatile long max = INT_MIN;
volatile bool done = false;

// mutexes to protect sum, odd, min, max, done, and the queue
pthread_mutex_t sum_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t odd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t min_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t max_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t done_mutex = PTHREAD_MUTEX_INITIALIZER;

// condition variable to signal when queue is not empty
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

// function prototypes
void update(long number);
struct node* dequeue(struct node** head);
void *thread_func(void *arg);

//create global head and back pointers
struct node* head = NULL;
struct node* back = NULL;

// link-list node to hold tasks
struct node {
    struct node* next;
    long data;
    char action;
};

// function to add tasks to queue using back pointer in a thread safe manner
void enqueue(struct node** head, struct node** back, long data, char action){
    // create new node
    struct node* new_node = malloc(sizeof(struct node));
    new_node->data = data;
    new_node->action = action;
    new_node->next = NULL;

    // if queue is empty, set head and back to new node
    if(*head == NULL){
        *head = new_node;
        *back = new_node;
    } else {
        // else add new node to back of queue
        (*back)->next = new_node;
        *back = new_node;
    }
}

// function to remove task from start of queue while returing a pointer to the node
struct node* dequeue(struct node** head) {
    // if queue is empty, return NULL
    if (*head == NULL) {
        return NULL;
    } else {
        // else remove node from front of queue
        struct node* temp = *head;
        *head = (*head)->next;

        // Detach the removed node from the queue
        temp->next = NULL;

        // return pointer to temp node
        return temp;
    }
}

/*
 * update global aggregate variables given a number
 */
void update(long number)
{
    // update sum
    pthread_mutex_lock(&sum_mutex);
    sum += number;
    pthread_mutex_unlock(&sum_mutex);

    // update odd variable if number is odd
    if (number % 2 == 1) {
        pthread_mutex_lock(&odd_mutex);
        odd++;
        pthread_mutex_unlock(&odd_mutex);
    }

    // find min
    pthread_mutex_lock(&min_mutex);
    if (number < min) {
        min = number;
    }
    pthread_mutex_unlock(&min_mutex);

    // find max
    pthread_mutex_lock(&max_mutex);
    if (number > max) {
        max = number;
    }
    pthread_mutex_unlock(&max_mutex);
}

// thread function
void *thread_func(void *arg)
{
    // create a temp node to hold dequeued node 
    struct node* temp = malloc(sizeof(struct node));

    pthread_mutex_lock(&done_mutex);
    while (!done || temp != NULL) {
        pthread_mutex_unlock(&done_mutex);
        
        // printf("-- thread DEBUG: Thread ID: %ld\n", pthread_self());  // DEBUGGING DELETE LATER ============================
        
        if (temp != NULL) {
            // if task is not NULL, update aggregate variables
            if (temp->action == 'p') {
                // simulate computation
                sleep(temp->data);
                // update aggregate variables
                update(temp->data);
            } 
            // if task is w, sleep for data seconds
            else { 
                sleep(temp->data);
            } 
        } 
        else {
            // condition variable to wait for queue to be signaled (queue not empty)
            pthread_mutex_lock(&queue_mutex);
            pthread_cond_wait(&queue_cond, &queue_mutex);
            pthread_mutex_unlock(&queue_mutex);
        }
        // dequeue task from queue
        pthread_mutex_lock(&queue_mutex);
        temp = dequeue(&head);
        pthread_mutex_unlock(&queue_mutex);

        // for checking while loop condition "done"
        pthread_mutex_lock(&done_mutex);
    }
    // unlock done_mutex when done is true
    pthread_mutex_unlock(&done_mutex);
    
    free(temp); //possibly put free inside loop <- CHECK BEFORE SUBMISSION ========================
    return NULL;
}

int main(int argc, char* argv[])
{
    // check and parse command line options
    if (argc < 3) {
        printf("Usage: sum <infile>\n");
        exit(EXIT_FAILURE);
    }
    char *fn = argv[1];
    int num_threads = atoi(argv[2]);

    //verify that num_threads is greater than 0
    if(num_threads < 1){
        printf("ERROR: Invalid number of threads: %d\n", num_threads);
        exit(EXIT_FAILURE);
    }

    // open input file
    FILE* fin = fopen(fn, "r");
    if (!fin) {
        printf("ERROR: Could not open %s\n", fn);
        exit(EXIT_FAILURE);
    }

    // initialize and create threads
    pthread_t threads[num_threads];
    for(int i = 0; i < num_threads; i++){
        pthread_create(&threads[i], NULL, thread_func, NULL);
    }

    // load numbers and add them to the queue
    char action;
    long num;
    
    // read file line by line by line, add data to queue
    while (fscanf(fin, "%c %ld\n", &action, &num) == 2) {

        // check for invalid action parameters
        if (num < 1) {
            printf("ERROR: Invalid action parameter: %ld\n", num);
            exit(EXIT_FAILURE);
        }

        // if p or w, create node and add to queue
        if (action == 'p' || action == 'w') {
            // printf("main DEBUG: Action: '%c'; number: %ld\n", action, num); // DEBUGGING DELETE LATER =====================
            pthread_mutex_lock(&queue_mutex);
            enqueue(&head, &back, num, action);
            pthread_cond_signal(&queue_cond);
            pthread_mutex_unlock(&queue_mutex);
        } 
        else {
            printf("ERROR: Unrecognized action: '%c'\n", action);
            exit(EXIT_FAILURE);
        }
    }
    fclose(fin);

    // signal threads that there are no more tasks
    pthread_mutex_lock(&done_mutex);
    done = true;
    pthread_mutex_unlock(&done_mutex);

    // broadcast queue_cond to ensure no threads are stuck in condition wait now that "done" is true
    pthread_mutex_lock(&queue_mutex);
    pthread_cond_broadcast(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);

    // wait for threads to finish
    for(int i = 0; i < num_threads; i++){
        pthread_join(threads[i], NULL);
    }
    
    // print results
    printf("%ld %ld %ld %ld\n", sum, odd, min, max);
    
    // clean up and return
    return (EXIT_SUCCESS);
}