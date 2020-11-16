/* CS 361 project5.c

Team: 
Names: 
Honor code statement: 

*/

#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <mqueue.h>
#include <assert.h>
#include "common.h"
#include "classify.h"
#include "intqueue.h" 
#ifdef GRADING // do not delete this or the next two lines
#include "gradelib.h"
#endif



int main(int argc, char *argv[])
{
    int input_fd;
    pid_t pid;
    off_t file_size;
    mqd_t tasks_mqd, results_mqd; // decsciptors
								  // The tasks queue will be populated by the supervisor process (parent), 
								  // which will add various types of tasks to the queue
								  // The results queue will be populated by the worker processes (running concurrently) 
								  // and contain the result of a classification task
    char tasks_mq_name[16]; // name of the tasks queue
    char results_mq_name[18];   // name of the results queue
    int num_clusters;
	
	struct mq_attr attr;
	attr.mq_flags = 0;
	attr.mq_maxmsg = 1000;
	attr.mq_msgsize = MESSAGE_SIZE_MAX;
	attr.mq_curmsgs = 0;

    if (argc != 2) {
        printf("Usage: %s data_file\n", argv[0]);
        return 1;
    }
	

    input_fd = open(argv[1], O_RDONLY);
    if (input_fd < 0) {
        printf("Error opening file \"%s\" for reading: %s\n", argv[1], strerror(errno));
        return 1;
    }

    // Determine the file size of the input file
    file_size = lseek(input_fd, 0, SEEK_END);
    close(input_fd);
    // Calculate how many clusters are present
    num_clusters = file_size / CLUSTER_SIZE;

    // Generate the names for the tasks and results queue
    snprintf(tasks_mq_name, 16, "/tasks_%s", getlogin()); //generating queuenames, gets loginID
    tasks_mq_name[15] = '\0';
    snprintf(results_mq_name, 18, "/results_%s", getlogin());
    results_mq_name[17] = '\0';

	//printf("%d\n", num_clusters);
    // Create the child processes
    for (int i = 0; i < NUM_PROCESSES; i++) {
        pid = fork();
        if (pid == -1)
            exit(1);
        else if (pid == 0) {

            // All the worker code must go here

            char cluster_data[CLUSTER_SIZE];
            char recv_buffer[MESSAGE_SIZE_MAX];
            struct task *my_task;
            unsigned char classification;

            // Here you need to create/open the two message queues for the
            // worker process. You must use the tasks_mqd and results_mqd
            // variables for this purpose
			tasks_mqd = mq_open(tasks_mq_name, O_CREAT | O_RDWR, 0600, &attr);
			results_mqd = mq_open(results_mq_name, O_CREAT | O_RDWR, 0600, &attr);

            // At this point, the queues must be open
#ifdef GRADING // do not delete this or you will lose points
            test_mqdes(tasks_mqd, "Tasks", getpid());
            test_mqdes(results_mqd, "Results", getpid());
#endif

			
            // A worker process must endlessly receive new tasks
            // until instructed to terminate
            while(1) {


                // receive the next task message here; you must read into recv_buffer
				if(mq_receive(tasks_mqd, recv_buffer, MESSAGE_SIZE_MAX, NULL) == -1) {
					printf("Error in worker receiving msg: %s\n", strerror(errno));
					exit(1);
				}

                // cast the received message to a struct task
                my_task = (struct task *)recv_buffer;
				int cluster_num = my_task->task_cluster;
				
				//map
				unsigned char init_clust_type;
				bool getFirst = true;
				bool finished = false;
				int clust_count = 0;
				int type;
				int classification_fd;
				int map_fd;
                switch (my_task->task_type) {
					
                    case TASK_CLASSIFY:
						//printf("number of task msgs: %ld \n", task.mq_curmsgs);
#ifdef GRADING // do not delete this or you will lose points
                        printf("(%d): received TASK_CLASSIFY\n", getpid());
#endif
                        // you must retrieve the data for the specified cluster
                        // and store it in cluster_data before executing the
                        // code below
						input_fd = open(argv[1], O_RDONLY);
						lseek(input_fd, cluster_num * CLUSTER_SIZE, SEEK_SET);
						read(input_fd, &cluster_data, CLUSTER_SIZE);

                        // Classification code
                        classification = TYPE_UNCLASSIFIED;
                        if (has_jpg_body(cluster_data))
                            classification |= TYPE_IS_JPG;
                        if (has_jpg_header(cluster_data))
                            classification |= TYPE_JPG_HEADER | TYPE_IS_JPG;
							//has_header = true;
                        if (has_jpg_footer(cluster_data))
                            classification |= TYPE_JPG_FOOTER | TYPE_IS_JPG;
                        // In the below statement, none of the JPG types were set, so
                        // the cluster must be HTML
                        if (classification == TYPE_UNCLASSIFIED) {
                            if (has_html_body(cluster_data))
                                classification |= TYPE_IS_HTML;
                            if (has_html_header(cluster_data))
                                classification |= TYPE_HTML_HEADER | TYPE_IS_HTML;
								//has_header = true;
                            if (has_html_footer(cluster_data))
                                classification |= TYPE_HTML_FOOTER | TYPE_IS_HTML;
                        }
                        if (classification == TYPE_UNCLASSIFIED)
                            classification = TYPE_UNKNOWN;
						
						struct result r;
						r.res_cluster_number = cluster_num;
						r.res_cluster_type = classification;
						
						//printf("received task CLASSIFY. cluster num: %d\n", cluster_num);
						
						if( mq_send(results_mqd, (const char*)&r, MESSAGE_SIZE_MAX, 0) < 0 ) {
							printf("Issue sending in worker: %s\n", strerror(errno));
						} 
						

                        // prepare a results message and send it to results. Send into the results queue
                        // queue here
						
                        break;

                    case TASK_MAP: //@8:45 Practice Session vid
						//seek to specified cluster in classification file
						// determine (take note of) whether its jpg or html
						// iterate over all clusters' entries in the classification file, starting with task_cluster, until a footer of the right type is found
						// if the current cluster is of the right type, generate a map entry and write it to the appropriate position to the map file
						classification_fd = open(CLASSIFICATION_FILE, O_RDONLY, 0600);
						if (classification_fd < 0) {
							printf("Error creating file \"%s\": %s\n", CLASSIFICATION_FILE, strerror(errno));
							exit(1);
						}
						
						map_fd = open(MAP_FILE, O_WRONLY | O_CREAT, 0600);
						if (map_fd < 0) {
							printf("Error creating file \"%s\": %s\n", CLASSIFICATION_FILE, strerror(errno));
							exit(1);
						}
						lseek(classification_fd, my_task->task_cluster, SEEK_SET);
						while(read(classification_fd, &type, sizeof(unsigned char)) != -1) {
							if(getFirst) {
								init_clust_type = type;
								getFirst = false;
							}
							if(init_clust_type < TYPE_IS_HTML) {
								if(type < TYPE_IS_HTML) {
									finished = (type >= TYPE_JPG_FOOTER);
									assert(lseek(map_fd, (clust_count + my_task->task_cluster) * 16, SEEK_SET) != -1);
									if (write(map_fd, &my_task->task_filename, sizeof(my_task->task_filename)) == -1) {
										printf("Error writing to map: %s\n", strerror(errno));
									}
									if(write(map_fd, &clust_count, sizeof(my_task->task_cluster))== -1) {
										printf("Error writing to map: %s\n", strerror(errno));
									}
								}
							} else {
								if(type >= TYPE_IS_HTML) {
									finished = (type >= TYPE_HTML_FOOTER);
									assert(lseek(map_fd, (clust_count + my_task->task_cluster) * 16, SEEK_SET) != -1);
									if (write(map_fd, &my_task->task_filename, sizeof(my_task->task_filename)) == -1) {
										printf("Error writing to map: %s\n", strerror(errno));
									}
									if(write(map_fd, &clust_count, sizeof(my_task->task_cluster))== -1) {
										printf("Error writing to map: %s\n", strerror(errno));
									}								
								}
							}
							if(finished) {
								break;
							}
							clust_count++;
						}
#ifdef GRADING // do not delete this or you will lose points
                        printf("(%d): received TASK_MAP\n", getpid());
#endif

                        break;

                    default:
						// close any open file or message queue descriptor and exit.
						// cleanup
						// implement the terminate task logic here
						printf("received task TERMINATE\n");
						close(tasks_mqd);
						close(results_mqd);
						close(classification_fd);
						close(map_fd);
						close(input_fd);
						exit(0);

#ifdef GRADING // do not delete this or you will lose points
                        printf("(%d): received TASK_TERMINATE or invalid task\n", getpid());
#endif

                }
            }
        }
    }

    // All the supervisor code needs to go here
    
    struct intqueue headerq; //in phase 1, if recieve classification type that is header, need to remember cluster number. 
							 // From this we will generate the map tasks

    // Initialize an empty queue to store the clusters that have file headers.
    // This queue needs to be populated in Phase 1 and worked off in Phase 2.
    initqueue(&headerq);

    
    // Here you need to create/open the two message queues for the
    // supervisor process. You must use the tasks_mqd and results_mqd
    // variables for this purpose

	if( (tasks_mqd = mq_open(tasks_mq_name, O_RDWR | O_CREAT | O_NONBLOCK, 0600, &attr)) < 0) { //O_NONBLOCK
		printf("Error opening task message queue: %s\n", strerror(errno));
		exit(1);
	}
	
	
	if( (results_mqd = mq_open(results_mq_name, O_RDWR | O_CREAT, 0600, &attr)) < 0) {
		printf("Error opening result message queue: %s\n", strerror(errno));
		exit(1);
	}
	
	
    // At this point, the queues must be open   
#ifdef GRADING // do not delete this or you will lose points
    test_mqdes(tasks_mqd, "Tasks", getpid());
    test_mqdes(results_mqd, "Results", getpid());
#endif
	
    // Implement Phase 1 here

    int classification_fd = open(CLASSIFICATION_FILE, O_RDWR | O_CREAT, 0600);
    if (classification_fd < 0) {
        printf("Error creating file \"%s\": %s\n", CLASSIFICATION_FILE, strerror(errno));
        return 1;
    }

	int cur_cluster = 0;
	int num_received = 0;
    struct task t;
    char rec_buffer[MESSAGE_SIZE_MAX];

    while (cur_cluster < num_clusters) {

        t.task_type = TASK_CLASSIFY;
        t.task_cluster = cur_cluster;
		while(mq_send(tasks_mqd, (char*)&t, sizeof(struct task), 0) < 0) {
			if(errno == EAGAIN) {
				if( mq_receive(results_mqd, rec_buffer, MESSAGE_SIZE_MAX, NULL) != 0) {
					struct result *my_result = (struct result *)rec_buffer;
					//add header types to intqueue
					if(my_result->res_cluster_type == 3 || my_result->res_cluster_type == 7 || my_result->res_cluster_type == 24 || my_result->res_cluster_type == 56) {
						enqueue(&headerq, my_result->res_cluster_number);
					}
					lseek(classification_fd, my_result->res_cluster_number, SEEK_SET);
					write(classification_fd, &my_result->res_cluster_type, sizeof(unsigned char));
					num_received++;
				} 
			}

        }
        cur_cluster++;
    }

	while(num_received < num_clusters) {
		if( mq_receive(results_mqd, rec_buffer, MESSAGE_SIZE_MAX, NULL) != 0) {
			struct result *my_result = (struct result *)rec_buffer;
			//add header types to intqueue
			if(my_result->res_cluster_type == 3 || my_result->res_cluster_type == 7 || my_result->res_cluster_type == 24 || my_result->res_cluster_type == 56) {
				enqueue(&headerq, my_result->res_cluster_number);
			}
			lseek(classification_fd, my_result->res_cluster_number, SEEK_SET);
			write(classification_fd, &my_result->res_cluster_type, sizeof(unsigned char));
			num_received++;
		} 
	}

	
	// End of Phase 1
	
#ifdef GRADING // do not delete this or you will lose points
    printf("(%d): Starting Phase 2\n", getpid());
#endif

    // Here you need to swtich the tasks queue to blocking
	close(tasks_mqd);
	tasks_mqd = mq_open(tasks_mq_name, O_WRONLY);

#ifdef GRADING // do not delete this or you will lose points
    test_mqdes(tasks_mqd, "Tasks", getpid());
#endif


    // Implement Phase 2 here
	unsigned char byte;
	int num_file = 1;
	while(isempty(&headerq) != 1) {
		int clust_num = dequeue(&headerq);
		lseek(classification_fd, clust_num, SEEK_SET);
		if(read(classification_fd, &byte, sizeof(byte)) < 0) {
			printf("Issue reading in supervisor: %s\n", strerror(errno));
		}
		
		if(byte >= TYPE_IS_HTML) {
			snprintf(t.task_filename, sizeof(t.task_filename),"file%04d.htm", num_file);
		} else {
			snprintf(t.task_filename, sizeof(t.task_filename),"file%04d.jpg", num_file);
		}
		t.task_type = TASK_MAP;
		t.task_cluster = clust_num;
		if(mq_send(tasks_mqd, (char*)&t, MESSAGE_SIZE_MAX, 0) < 0) {
			printf("Issue reading in supervisor: %s\n", strerror(errno));
		}
		num_file++;
	}


    // End of Phase 2

#ifdef GRADING // do not delete this or you will lose points
    printf("(%d): Starting Phase 3\n", getpid());
#endif

    // Implement Phase 3 here START HERE, make sure communication between parent child works
   for (int i = 0; i < NUM_PROCESSES; i++) {
		t.task_type = TASK_TERMINATE;
		if(mq_send(tasks_mqd, (char*)&t, MESSAGE_SIZE_MAX, 0) < 0) {
			printf("Issue sending in termination: %s\n", strerror(errno));
		} 
   }
   wait(NULL);
   mq_close(tasks_mqd);
   mq_close(results_mqd);
   mq_unlink(tasks_mq_name);
   mq_unlink(results_mq_name);
   close(classification_fd);
   exit(0);
   return 0;
}

