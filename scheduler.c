#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define min(a,b) (((a)<(b))?(a):(b))

// total jobs
int numofjobs = 0;

struct job {
    // job id is ordered by the arrival; jobs arrived first have smaller job id, always increment by 1
    int id;
    int arrival; // arrival time; safely assume the time unit has the minimal increment of 1
    int length;
    int tickets; // number of tickets for lottery scheduling
    // TODO: add any other metadata you need to track here
    struct job *next;
};

// the workload list
struct job *head = NULL;

void append_to(struct job **head_pointer, int arrival, int length, int tickets) {
    if (*head_pointer == NULL) {
        struct job *new_job = (struct job *)malloc(sizeof(struct job));
        if (new_job == NULL) {
            exit(-1);  // Exit if memory allocation fails
        }

        new_job->id = 0;  // Initialize the job ID to 0
        new_job->arrival = arrival;
        new_job->length = length;
        new_job->tickets = tickets;
        new_job->next = NULL;

        *head_pointer = new_job;  // Set the head pointer to the new job
        return;
    }
    
    struct job *p1 = *head_pointer;
    // Traverse to the end of the list
    while (p1->next != NULL) {
        p1 = p1->next;
    }

    // Append the new job at the end
    struct job *new_job = (struct job *)malloc(sizeof(struct job));
    if (new_job == NULL) {
        exit(-1);  // Exit if memory allocation fails
    }
    new_job->id = p1->id + 1;  // Increment job ID based on the last job
    new_job->arrival = arrival;
    new_job->length = length;
    new_job->tickets = tickets;
    new_job->next = NULL;

    p1->next = new_job;  // Link the new job at the end
    return;
}


void read_job_config(const char* filename) {
    FILE *fp;
    char line[1024];  // Using fgets instead of getline
    int tickets = 0;

    char* delim = ",";
    char *arrival = NULL;
    char *length = NULL;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        line[strcspn(line, "\n")] = 0; // Remove newline character
        arrival = strtok(line, delim);
        length = strtok(NULL, delim);
        if (arrival != NULL && length != NULL) {
            tickets += 100;
            append_to(&head, atoi(arrival), atoi(length), tickets);
        }
    }

    fclose(fp);
}

void policy_SJF(struct job* head) {
    sortLength(&head);
    printf("Execution trace with SJF:\n");
    struct job *current = head;
    int time = 0;

    while (current != NULL) {
        // If the job has not arrived yet, idle the CPU until it does
        if (time < current->arrival) {
            //printf("t=%d: CPU idle until Job %d arrives at [%d]\n", time, current->id, current->arrival);
            time = current->arrival;  // Move time forward to job arrival
        }

        // Print job execution trace
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", time, current->id, current->arrival, current->length);
        time += current->length;  // Increment time by the length of the current job
        current = current->next;   // Move to the next job
    }
    
    printf("End of execution with SJF.\n");
}

void sortLength(struct job** headRef) {
    struct job* newHead = NULL;
    struct job* p1 = *headRef;

    if (*headRef == NULL) {
        return;
    }

    while (p1 != NULL) {
        insertWLength(&newHead, p1);
        p1 = p1->next; // Move to the next job
    }

    // Deallocate old nodes
    struct job* current = *headRef; // Use headRef to point to the original list
    struct job* nextJob;

    while (current != NULL) {
        nextJob = current->next; // Store the next job
        free(current);           // Free the current job
        current = nextJob;       // Move to the next job
    }
    *headRef = newHead; // Update the head reference to point to the new list
}


void insertWLength(struct job** newHead, struct job* p){
    if (*newHead == NULL) {
        struct job *new_job = (struct job *)malloc(sizeof(struct job));
        if (new_job == NULL) {
            exit(-1);  // Exit if memory allocation fails
        }

        new_job->id = p->id;  
        new_job->arrival = p->arrival;
        new_job->length = p->length;
        new_job->tickets = p->tickets;
        new_job->next = NULL;

        *newHead = new_job;  // Set the head pointer to the new job
        return;
    }
    int pArrival = p->arrival;
    int pLength = p->length;

    struct job *p1 = *newHead;
    // Traverse to the end of the list
   
    while (1) { 
        if (pArrival > p1->arrival && p1->next != NULL)
        {
            p1 = p1->next;
        }else if (pArrival >= p1->arrival && pLength > p1->length && p1->next != NULL && p1->next->length < pLength )
        {                                                                                                              
            p1 = p1->next;
        }else{
            break;
        }
                                                                         
    }
    
    

    
    struct job *new_job = (struct job *)malloc(sizeof(struct job));
    if (new_job == NULL) {
        exit(-1);  // Exit if memory allocation fails
    }
    new_job->id = p->id;  
    new_job->arrival = p->arrival;
    new_job->length = p->length;
    new_job->tickets = p->tickets;
    new_job->next = NULL;
    if(p1 == *newHead){
        struct job *temp = (*newHead)->next;
        *newHead = new_job; 
        new_job->next = temp;
        
    } else{
        struct job *temp = p1->next;
        p1->next = new_job;  
        new_job->next = temp;
    }
    
    return;
}

void policy_STCF()
{
    printf("Execution trace with STCF:\n");

    // TODO: implement STCF policy

    printf("End of execution with STCF.\n");
}


void policy_RR(int slice)
{
    printf("Execution trace with RR:\n");

    // TODO: implement RR policy

    printf("End of execution with RR.\n");
}


void policy_LT(int slice)
{
    printf("Execution trace with LT:\n");

    // Leave this here, it will ensure the scheduling behavior remains deterministic
    srand(42);

    // In the following, you'll need to:
    // Figure out which active job to run first
    // Pick the job with the shortest remaining time
    // Considers jobs in order of arrival, so implicitly breaks ties by choosing the job with the lowest ID

    // To achieve consistency with the tests, you are encouraged to choose the winning ticket as follows:
    // int winning_ticket = rand() % total_tickets;
    // And pick the winning job using the linked list approach discussed in class, or equivalent

    printf("End of execution with LT.\n");

}

void policy_FIFO() {
    printf("Execution trace with FIFO:\n");

    struct job *current = head;
    int time = 0;

    while (current != NULL) {
        // If the job has not arrived yet, idle the CPU until it does
        if (time < current->arrival) {
            //printf("t=%d: CPU idle until Job %d arrives at [%d]\n", time, current->id, current->arrival);
            time = current->arrival;  // Move time forward to job arrival
        }

        // Print job execution trace
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", time, current->id, current->arrival, current->length);
        time += current->length;  // Increment time by the length of the current job
        current = current->next;   // Move to the next job
    }
    printf("End of execution with FIFO.\n");
}



int main(int argc, char **argv){

    static char usage[] = "usage: %s analysis policy slice trace\n";

    int analysis;
    char *pname;
    char *tname;
    int slice;


    if (argc < 5)
    {
        fprintf(stderr, "missing variables\n");
        fprintf(stderr, usage, argv[0]);
		exit(1);
    }

    // if 0, we don't analysis the performance
    analysis = atoi(argv[1]);

    // policy name
    pname = argv[2];

    // time slice, only valid for RR
    slice = atoi(argv[3]);

    // workload trace
    tname = argv[4];

    read_job_config(tname);


    if (strcmp(pname, "FIFO") == 0){
        policy_FIFO();
        if (analysis == 1){
            // TODO: perform analysis
        }
    }
    else if (strcmp(pname, "SJF") == 0)
    {
        // TODO
    }
    else if (strcmp(pname, "STCF") == 0)
    {
        // TODO
    }
    else if (strcmp(pname, "RR") == 0)
    {
        // TODO
    }
    else if (strcmp(pname, "LT") == 0)
    {
        // TODO
    }

	exit(0);
}