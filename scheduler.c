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
    int hasRanFor; //amount of time that the job has executed for(used in RR)
    int response_time; //start time - arrival time
    int turnaround_time; //turnaround time
    int wait_time;  //first exec time - arrival
    struct job *next;
     int completion_time; 
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
        new_job->hasRanFor = 0;
        new_job->response_time = -1;
        new_job->turnaround_time = 0;
        new_job->completion_time = -1;
        new_job->wait_time = -1;

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
    new_job->hasRanFor = 0;
    new_job->response_time = -1;
    new_job->turnaround_time = 0;
    new_job->completion_time = -1;
    new_job->wait_time = -1;

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
        line[strcspn(line, "\n")] = 0; 
        arrival = strtok(line, delim);
        length = strtok(NULL, delim);
        if (arrival != NULL && length != NULL) {
            tickets += 100;
            append_to(&head, atoi(arrival), atoi(length), tickets);
        }
    }

    fclose(fp);
}


// Helper function to duplicate the job list
struct job* duplicate_job_list(struct job* original) {
    if (original == NULL) {
        return NULL;
    }

    struct job* copy_head = NULL;
    struct job* copy_tail = NULL;
    
    struct job* current = original;
    while (current != NULL) {
        // Create a new job node and copy data
        struct job* new_job = (struct job*) malloc(sizeof(struct job));
        memcpy(new_job, current, sizeof(struct job)); // Copy all fields
        new_job->next = NULL; // Initialize the next pointer to NULL
        
        // Append the new job to the copy list
        if (copy_head == NULL) {
            copy_head = new_job; // First node
        } else {
            copy_tail->next = new_job; // Append to the tail
        }
        copy_tail = new_job; // Update the tail
        
        current = current->next; // Move to the next job
    }

    return copy_head;
}

void policy_SJF() {
    // Make a copy of the original job list
    struct job* job_copy = duplicate_job_list(head);
    int current_time = 0;

    printf("Execution trace with SJF:\n");

    while (job_copy != NULL) {
        struct job *current = job_copy;
        struct job *shortest = NULL;

        // Find the job with the shortest length that has arrived
        while (current != NULL) {
            if (current->arrival <= current_time) {
                if (shortest == NULL || current->length < shortest->length ||
                    (current->length == shortest->length && current->arrival < shortest->arrival)) {
                    shortest = current;
                }
            }
            current = current->next;
        }

        // If no job is ready to execute, move time forward to the next job arrival
        if (shortest == NULL) {
            current = job_copy;
            int next_arrival = INT_MAX;
            while (current != NULL) {
                if (current->arrival > current_time && current->arrival < next_arrival) {
                    next_arrival = current->arrival;
                }
                current = current->next;
            }
            current_time = next_arrival;
        } else {
            // Print execution trace
            printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n",
                   current_time, shortest->id, shortest->arrival, shortest->length);
            
            // Calculate turnaround, wait, and response times
            shortest->turnaround_time = current_time + shortest->length - shortest->arrival;
            shortest->wait_time = current_time - shortest->arrival;
            shortest->response_time = shortest->wait_time; // Response time is equal to wait time in non-preemptive SJF

            // Update the original job list with calculated times
            struct job *orig = head;
            while (orig != NULL) {
                if (orig->id == shortest->id) {
                    orig->turnaround_time = shortest->turnaround_time;
                    orig->wait_time = shortest->wait_time;
                    orig->response_time = shortest->response_time;
                    break;
                }
                orig = orig->next;
            }

            current_time += shortest->length;

            // Remove the shortest job from the copy list
            struct job **ptr = &job_copy;
            while (*ptr != NULL) {
                if (*ptr == shortest) {
                    *ptr = shortest->next;
                    free(shortest);
                    break;
                }
                ptr = &(*ptr)->next;
            }
        }
    }

    printf("End of execution with SJF.\n");
}

void policy_STCF() {
    struct job* job_copy = duplicate_job_list(head);
    int current_time = 0; 
    struct job *current_job = NULL;  

    printf("Execution trace with STCF:\n");

    while (job_copy != NULL) {
        struct job *shortest = NULL;
        struct job *current = job_copy;

        // Find the job with the shortest remaining time that has arrived
        while (current != NULL) {
            if (current->arrival <= current_time) {
                if (shortest == NULL || (current->length < shortest->length) ||
                    (current->length == shortest->length && current->arrival < shortest->arrival)) {
                    shortest = current; 
                }
            }
            current = current->next;
        }

        // If no job is ready to execute, move time forward to the next job arrival
        if (shortest == NULL) {
            current = job_copy;
            int next_arrival = INT_MAX;
            while (current != NULL) {
                if (current->arrival > current_time && current->arrival < next_arrival) {
                    next_arrival = current->arrival; 
                }
                current = current->next;
            }
            current_time = next_arrival; 
        } else {
            // If there is a current job running, and the new job has a shorter length
            if (current_job != NULL) {
                if (shortest->length < current_job->length) {
                    // Accumulate wait time for the current job
                    current_job->wait_time += (current_time - current_job->completion_time);
                    current_job->completion_time = current_time;  // Update completion time
                }
            }

            // If this is the first time we are running the shortest job
            if (current_job == NULL || (shortest->length < current_job->length)) {
                current_job = shortest;
                // Print the execution trace
                printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n",
                       current_time, current_job->id, current_job->arrival, current_job->length);
            }

            // Increment the current time and decrease the job length
            current_time++;
            current_job->length--;

            // Update analysis parameters for the running job
            struct job *orig = head;
            while (orig != NULL) {
                if (orig->id == current_job->id) {
                    orig->completion_time = current_time; // Record completion time
                    orig->turnaround_time = orig->completion_time - orig->arrival; // Turnaround time
                    orig->wait_time = orig->turnaround_time - (orig->length); // Wait time
                    orig->response_time = orig->turnaround_time - orig->length; // Calculate response time
                    break;
                }
                orig = orig->next;
            }

            // If the current job has completed
            if (current_job->length == 0) {
                struct job **ptr = &job_copy;
                while (*ptr != NULL) {
                    if (*ptr == current_job) {
                        *ptr = current_job->next; 
                        free(current_job); 
                        break;
                    }
                    ptr = &(*ptr)->next;
                }
                current_job = NULL;  // Reset the running job
            }
        }
    }

    printf("End of execution with STCF.\n");
}
void policy_RR(int slice) {
    int current_time = 0;
    struct job* currentJob = head;
    int jobsLeft = 0;

    // Count how many jobs are left
    struct job* temp = head;
    while (temp != NULL) {
        jobsLeft++;
        temp = temp->next;
    }

    printf("Execution trace with RR:\n");
    currentJob = head;

    while (jobsLeft > 0) {
        int jobFound = 0;  // Flag to check if a job was executed in this cycle

        if (currentJob->arrival <= current_time && (currentJob->length - currentJob->hasRanFor) > 0) {
            int jobExecTime = (currentJob->length - currentJob->hasRanFor > slice) ? slice : (currentJob->length - currentJob->hasRanFor);
            
            // If this is the first time the job is running, set response time
            if (currentJob->hasRanFor == 0) {
                currentJob->response_time = current_time - currentJob->arrival;
            }

            printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", current_time, currentJob->id, currentJob->arrival, jobExecTime);
            current_time += jobExecTime;

            currentJob->hasRanFor += jobExecTime;

            // Check if the job is completed
            if (currentJob->hasRanFor == currentJob->length) {
                jobsLeft--;
                currentJob->completion_time = current_time;
                currentJob->turnaround_time = currentJob->completion_time - currentJob->arrival;
                currentJob->wait_time = currentJob->turnaround_time - currentJob->length; // Wait time calculation
            }

            jobFound = 1;  
        }

        // If no job was executed, find the next job that has arrived
        if (!jobFound) {
            struct job* temp = currentJob->next ? currentJob->next : head;
            while (temp != currentJob && !(temp->arrival <= current_time && (temp->length - temp->hasRanFor) > 0)) {
                temp = temp->next ? temp->next : head;
            }
            
            // If we did not find any job that can run, increment current_time
            if (temp == currentJob) {
                current_time++;
            }
        }

        // Move to the next job in the list
        currentJob = currentJob->next ? currentJob->next : head;
    }

    printf("End of execution with RR.\n");
}

void policy_LT(int slice) {
    printf("Execution trace with LT:\n");
    srand(42);

    int currentTime = 0;
    int ticketCount = 0;
    int jobsToDo = 0;

    // Give Tickets
    struct job* current = head;
    while (current != NULL) {
        ticketCount += current->tickets;
        jobsToDo++;
        current->wait_time = -1;  // initialize wait time to -1 to detect first run
        current = current->next;
    }

    while (jobsToDo > 0) {
    int winner = (rand() % ticketCount) + 1; // Randomly select a ticket

    // Find the job corresponding to the winning ticket
    current = head;
    int ticketIndex = current->tickets;
    
    // Traverse the linked list to find the winning job
    while (winner > ticketIndex) {
        current = current->next;
        ticketIndex += current->tickets;

        // If we reach the end of the list and still haven't found an eligible job, reset to head
        if (current == NULL) {
            current = head;
            ticketIndex = current->tickets; // Reset ticket index for head job
        }
    }
    
    
    

    // Check if the current job has arrived
    struct job* temp = head;

// If the current job has not arrived yet, look for the first available job
    if (current->arrival > currentTime) {
        // Iterate through the job list to find the first job that has arrived
        temp = head;
        while (temp != NULL) {
            // Check if the job has arrived and is not yet completed
            if (temp->arrival <= currentTime && (temp->length > temp->hasRanFor)) {
                // If we found a valid job that has arrived and isn't completed
                current = temp;  // Set current to this job
                break;  // Exit the loop
            }
            temp = temp->next;  // Move to the next job
        }
        
        // If no job was found, increment currentTime
        if (temp == NULL || current->arrival > currentTime) {
            currentTime++;  // Increment time if no valid job found
        }

        // Optionally, you can print or log that the scheduler is idle
        continue;  // Skip this iteration and check for another job
    }


    // Job has arrived and is eligible for execution
    // Set Start Timer only for the first execution
    if (current->hasRanFor == 0) {
        current->response_time = currentTime - current->arrival;
    }

    // If remaining time is more than slice, run for slice
    int jobLengthLeft = current->length - current->hasRanFor;
    if (jobLengthLeft > slice) {
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", 
                currentTime, current->id, current->arrival, slice);
        currentTime += slice;
        current->hasRanFor += slice;

    // If remaining time is not 0, but not more than slice (job is not completed), then do final run
    } else if (jobLengthLeft > 0) {
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", 
                currentTime, current->id, current->arrival, jobLengthLeft);
        currentTime += jobLengthLeft;
        current->hasRanFor += jobLengthLeft;

        // Final job completion and statistics update
        current->completion_time = currentTime;
        current->turnaround_time = currentTime - current->arrival;
        current->wait_time = current->turnaround_time - current->length;

        // Remove tickets of completed job and decrement jobsToDo count
        ticketCount -= current->tickets;
        current->tickets = 0;
        jobsToDo--;
    }
}

    printf("End of execution with LT.\n");
    return;
}




void policy_FIFO() {
    printf("Execution trace with FIFO:\n");

    struct job *current = head;
    int time = 0;

    while (current != NULL) {
        // If the time is less than the job's arrival, move time forward to job arrival
        if (time < current->arrival) {
            time = current->arrival;  
        }

        // Print the execution trace
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", time, current->id, current->arrival, current->length);
        
        // Simulate running the job
        time += current->length;  // Increment time by the length of the current job
        
        // Calculate turnaround time and wait time
        current->turnaround_time = time - current->arrival; // Calculate turnaround time
        current->wait_time = current->turnaround_time - current->length; // Calculate wait time
        current->response_time = current->turnaround_time - current->length; // Calculate wait time

        // Move to the next job
        current = current->next;   
    }
    printf("End of execution with FIFO.\n");
}




void analyze_jobs() {
    struct job *current = head;
    int total_response_time = 0, total_turnaround_time = 0, total_wait_time = 0;
    int job_count = 0;

    while (current != NULL) {
        printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n",
               current->id, current->response_time, current->turnaround_time, current->wait_time);
        total_response_time += current->response_time;
        total_turnaround_time += current->turnaround_time;
        total_wait_time += current->wait_time;
        job_count++;
        current = current->next;
    }

    printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\n",
           (double)total_response_time / job_count, 
           (double)total_turnaround_time / job_count, 
           (double)total_wait_time / job_count);
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
            printf("Begin analyzing FIFO:\n");
            analyze_jobs();
            printf("End analyzing FIFO.\n");
        }
    }
    else if (strcmp(pname, "SJF") == 0)
    {
        policy_SJF();
        if (analysis == 1){
            printf("Begin analyzing SJF:\n");
            analyze_jobs();
            printf("End analyzing SJF.\n");
        }
    }
    else if (strcmp(pname, "STCF") == 0)
    {
        policy_STCF();
        if (analysis == 1){
            printf("Begin analyzing STCF:\n");
            analyze_jobs();
            printf("End analyzing STCF.\n");
        }
    }
    else if (strcmp(pname, "RR") == 0)
    {
        policy_RR(slice);
        if (analysis == 1){
            printf("Begin analyzing RR:\n");
            analyze_jobs();
            printf("End analyzing RR.\n");
        }
    }
    else if (strcmp(pname, "LT") == 0)
    {
        policy_LT(slice);
        if (analysis == 1){
            printf("Begin analyzing LT:\n");
            analyze_jobs();
            printf("End analyzing LT.\n");
        }
    }

	exit(0);
}