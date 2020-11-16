/* project4.c

Team: 10
Names: Jackson Burns & David Kim
Honor code statement: This work complies with the JMU honor code. We did not give or receive any unauthorized help on this assignment.

*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

#include "common.h"
#include "classify.h"
#ifdef GRADING // do not delete this or the next two lines
#include "gradelib.h"
#endif


#define NUM_PROCESSES 5
//#define CLUSTER_SIZE 4096

// This is the recommended struct for sending a message about
// a cluster's type through the pipe. You need not use this,
// but I would recommend that you do.
struct msg { // send from child
    int msg_cluster_number;  // The cluster number from the input file
    unsigned char msg_cluster_type;  // The type of the above cluster
                                     // as determined by the classifier
									 // single byte for writing each cluster of classification file!
};

int main(int argc, char *argv[])
{
    int input_fd;
    int classification_fd;
    pid_t pid;
    int pipefd[2];
    struct msg message;
    int start_cluster = 0; // When a child process is created, this variable must
                       // contain the first cluster in the input file it
                       // needs to classify
					   // set by the parent before fork() called?
    int clusters_to_process; // This variable must contain the number of
                             // clusters a child process needs to classify

    // The user must supply a data file to be used as input
    if (argc != 2) {
        printf("Usage: %s data_file\n", argv[0]);
        return 1;
    }

	// Try opening the file for reading, exit with appropriate error message
    // if open fails
    input_fd = open(argv[1], O_RDONLY);
    if (input_fd < 0) {
        printf("Error opening file \"%s\" for reading: %s\n", argv[1], strerror(errno));
        return 1;
    }

    // Open classification file for writing. Create file if it does not
    // exist. Exit with error if open() fails.
    classification_fd = open(CLASSIFICATION_FILE, O_WRONLY | O_CREAT, 0600);
    if (classification_fd < 0) {
        printf("Error creating file \"%s\": %s\n", CLASSIFICATION_FILE, strerror(errno));
        return 1;
    }

    // Create the pipe here. Exit with error if this fails.
	if (pipe (pipefd) < 0)
    {
       printf ("ERROR: Failed to open pipe\n");
       exit (1);
    }


    // The pipe must be created at this point
#ifdef GRADING // do not delete this or you will lose points
    test_pipefd(pipefd, 0);
#endif


	int length = lseek(input_fd,0,SEEK_END);
	int num_clusters = length / CLUSTER_SIZE;
	int remainder = num_clusters % NUM_PROCESSES;
	int quotient = num_clusters / NUM_PROCESSES;
    lseek(input_fd,0,SEEK_SET);
	int offset = 1;




    // Fork NUM_PROCESS number of child processes
    for (int i = 0; i < NUM_PROCESSES; i++) {
		if(remainder != 0) {
			clusters_to_process = quotient + 1;
			remainder--;
            start_cluster = i * clusters_to_process;
		} else {
			clusters_to_process = quotient;
            start_cluster = (i * (quotient + 1)) - offset;
			offset++;
		}
		// initialize start_cluster and clusters_to_process before calling fork() --> parent communicating to each child
        pid = fork();
        // Exit if fork() fails.
        if (pid == -1)
            exit(1);
        // This is the place to write the code for the child processes
        else if (pid == 0) {
			input_fd = open(argv[1], O_RDONLY);

            // In this else if block, you need to implement the entire logic
            // for the child processes to be aware of which clusters
            // they need to process, classify them, and create a message
            // for each cluster to be written to the pipe.


            // At this point, the child must know its start cluster and
            // the number of clusters to process.
#ifdef GRADING // do not delete this or you will lose points
            printf("Child process %d\n\tStart cluster: %d\n\tClusters to process: %d\n",
              getpid(), start_cluster, clusters_to_process);
#endif
			close(pipefd[0]);
			//(close appropriate end of pipe) --> is being tested in code below
            // At this point the pipe must be fully set up for the child
            // This code must be executed before you start iterating over the input
            // file and before you generate and write messages.
#ifdef GRADING // do not delete this or you will lose points
            test_pipefd(pipefd, getpid());
#endif
			char cluster_data[CLUSTER_SIZE];
			int j = start_cluster;
			int bytes_read;
			struct msg m2;

			lseek(input_fd, j * CLUSTER_SIZE, SEEK_SET);
			while ((bytes_read = read(input_fd, &cluster_data, CLUSTER_SIZE)) > 0 && j <= start_cluster + clusters_to_process) {
				assert(bytes_read == CLUSTER_SIZE);
				m2.msg_cluster_type = TYPE_UNCLASSIFIED;

				if (has_jpg_header(cluster_data) == 1) { //if has jpg header - seems to always go into just this conditional
					m2.msg_cluster_type += TYPE_JPG_HEADER;
				}

				if (has_jpg_body(cluster_data) == 1 || (has_jpg_body(cluster_data) == 0
					&& (has_jpg_header(cluster_data) == 1 || has_jpg_footer(cluster_data) == 1))) { //jpg data
					m2.msg_cluster_type += TYPE_IS_JPG;
				}
				if (has_jpg_footer(cluster_data) == 1) { //jpg footer
					m2.msg_cluster_type += TYPE_JPG_FOOTER;
				}

				if (has_html_header(cluster_data) == 1) { //html header
					m2.msg_cluster_type+= TYPE_HTML_HEADER;
				}
				if ( has_html_body(cluster_data) == 1 || (has_html_body(cluster_data) == 0
				&& (has_html_header(cluster_data) == 1 || has_html_footer(cluster_data) == 1))) { //html data
					m2.msg_cluster_type += TYPE_IS_HTML;
				}
				if (has_html_footer(cluster_data) == 1) { //html footer
					m2.msg_cluster_type += TYPE_HTML_FOOTER;
				}
				m2.msg_cluster_number = j;
				write(pipefd[1], &m2, sizeof(m2));
                j++;

			}



            // Implement the main loop for the child process below this line
			// child needs to acces input file and start with "the cluster its supposed to stat with"
			// and do classification (from pa3) - make sure it only does number of clustrs its supposed to do
			// write() it to the pipe


            exit(0); // This line needs to be the last one for the child
                     // process code. Do not delete this!
					 // if we dont exit() the child will continue with the loop and create a grandchild
        }

    }

    // All the code for the parent's handling of the messages and
    // creating the classification file needs to go in the block below
    close(pipefd[1]);
    // At this point, the pipe must be fully set up for the parent
#ifdef GRADING // do not delete this or you will lose points
    test_pipefd(pipefd, 0);
#endif
    // Read one message from the pipe at a time
    while (read(pipefd[0], &message, sizeof(message)) > 0) { // writing to classification file. child should have exited at this point.
        // In this loop, you need to implement the processing of
        // each message sent by a child process. Based on the content,
        // a proper entry in the classification file needs to be written.

        lseek(classification_fd, message.msg_cluster_number, SEEK_SET);

		write(classification_fd, &message.msg_cluster_type, sizeof(message.msg_cluster_type));

	}
	close(input_fd);
    close(classification_fd); // close the file descriptor for the
                              // classification file
    return 0;
};
