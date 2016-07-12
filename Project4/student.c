/*
 * student.c
 * Multithreaded OS Simulation for CS 2200, Project 4
 * Fall 2014
 *
 * This file contains the CPU scheduler for the simulation.
 * Name: Shen Yang
 * GTID: 902912328
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os-sim.h"

/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 */
static pcb_t **current;
static pthread_mutex_t current_mutex;

static pcb_t *head;
static pthread_mutex_t ready_mutex;
static pthread_cond_t is_empty;
static int schedulerType;
static int timeSlice;
static int cpu_count;
static int inIdle;

void add(pcb_t* q) {
	pcb_t* tmp;
	q -> next = NULL;
	if(head == NULL) {
		head = q;
	} else {
		tmp = head;
		while (tmp -> next != NULL) {
			tmp = tmp -> next;
		}
		tmp -> next = q;
	}
}

pcb_t* removeFront() {
	pcb_t * tmp;
	
	if (head != NULL) {
		tmp = head;
		head = head -> next;
	} else {
		return NULL;
	}
	return tmp;
}

pcb_t* getHigh() {
	pcb_t *tmp, *max;
	
	if (head != NULL) {
		max = head;
		tmp = head;
		while (tmp -> next != NULL) {
			if (max -> static_priority < tmp -> static_priority) {
				max = tmp;
			}
			tmp = tmp -> next;
		}
	} else {
		return NULL;
	}
	return max;
}

int getSize() {
	int i = 0;
	pcb_t *tmp;
	if (head != NULL) {
		tmp = head;
		while (tmp -> next != NULL) {
			i++;
			tmp = tmp -> next;
		}
	}
	return i;
}

/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which 
 *	you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *	The current array (see above) is how you access the currently running process indexed by the cpu id. 
 *	See above for full description.
 *	context_switch() is prototyped in os-sim.h. Look there for more information 
 *	about it and its parameters.
 */
static void schedule(unsigned int cpu_id)
{
	pthread_mutex_lock(&ready_mutex);
	pcb_t* q = removeFront();
	pthread_mutex_unlock(&ready_mutex);
	if (q != NULL) {
		pthread_mutex_lock(&ready_mutex);
		q->state = PROCESS_RUNNING;
		pthread_mutex_unlock(&ready_mutex);
		pthread_mutex_lock(&current_mutex);
		current[cpu_id] = q;
		pthread_mutex_unlock(&current_mutex);
		context_switch(cpu_id, q, timeSlice); 
	} else {
		context_switch(cpu_id, NULL, timeSlice);
	} 
}


/*
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * This function should block until a process is added to your ready queue.
 * It should then call schedule() to select the process to run on the CPU.
 */
extern void idle(unsigned int cpu_id)
{
	pthread_mutex_lock(&ready_mutex);
	inIdle = inIdle+1;
	while (head == NULL) {
		pthread_cond_wait(&is_empty, &ready_mutex);
	}
	inIdle = inIdle-1;
	pthread_mutex_unlock(&ready_mutex);
	schedule(cpu_id);
}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 */
extern void preempt(unsigned int cpu_id)
{
	pcb_t *pcb;
	pcb = current[cpu_id];
	add(pcb);
	schedule(cpu_id);
}


/*
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * It should mark the process as WAITING, then call schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id)
{
	pthread_mutex_lock(&current_mutex);
	current[cpu_id]->state = PROCESS_WAITING;
	pthread_mutex_unlock(&current_mutex);
	schedule(cpu_id);	

}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id)
{
	pthread_mutex_lock(&current_mutex);
	current[cpu_id]->state = PROCESS_TERMINATED;
	pthread_mutex_unlock(&current_mutex);
	schedule(cpu_id);
}


/*
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is static priority, wake_up() may need
 *      to preempt the CPU with the lowest priority process to allow it to
 *      execute the process which just woke up.  However, if any CPU is
 *      currently running idle, or all of the CPUs are running processes
 *      with a higher priority than the one which just woke up, wake_up()
 *      should not preempt any CPUs.
 *	To preempt a process, use force_preempt(). Look in os-sim.h for 
 * 	its prototype and the parameters it takes in.
 */
extern void wake_up(pcb_t *process)
{
	int v = -1;
	pthread_mutex_lock(&ready_mutex);
	pthread_mutex_lock(&current_mutex);
	if (schedulerType == 3) {
		int i = 0;
		while (i < cpu_count) {
			if(current[i] != NULL) {
				if((current[i] -> static_priority) < process -> static_priority) {
					v = i;
				}
			}
			i++;
		}
		
		if(v != -1) {
			pthread_mutex_unlock(&ready_mutex);
			pthread_mutex_unlock(&current_mutex);
			if (head != NULL) {
				force_preempt(v);
			}
			pthread_mutex_lock(&current_mutex);
			pthread_mutex_lock(&ready_mutex);
		}
	}
	process->state = PROCESS_READY;
	add(process);
	pthread_cond_signal(&is_empty);
	pthread_mutex_unlock(&current_mutex);
	pthread_mutex_unlock(&ready_mutex);
}


/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -r and -p command-line parameters.
 */
int main(int argc, char *argv[])
{
	inIdle = 0;
	cpu_count = 0;
    cpu_count = atoi(argv[1]);
	schedulerType = 0;
	if (argc != 2) {
		if(argv[2][1] == 'r') {
			schedulerType = 2;
			timeSlice = atoi(argv[3]);
		}
		if(argv[2][1] == 'p') {
			schedulerType = 3;
		}
	} else {
		schedulerType = 1;
		timeSlice = -1;
	}

	head = NULL;
    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);
	pthread_mutex_init(&ready_mutex, NULL);
	pthread_cond_init(&is_empty,NULL);
    start_simulator(cpu_count);

    return 0;
}
