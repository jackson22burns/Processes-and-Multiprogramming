/*
 * Names: Jackson Burns, David Kim
 *
 * Honor Code: This work complies with the JMU honor code.
 * We did not recieve unauthorized help on this assignment.
 *
 */



#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>


//struct for map entries
struct fd{
	char file_name[13];
	int fd;
};


int main (int argc, char **argv) 
{
	//create initial variables
	char *data_filename;
	char *map_filename;
	int data_file;
	int map_file;
	struct fd* files = malloc(200);
	int files_max = 10;
	
	
	//check number of arguments
	if (argc != 3) {
	   printf("Wrong number of arguments\n");
	   exit(1);
   }
   
	//set arguments
	data_filename = argv[1];
	map_filename = argv[2];
	
	//open data file
	data_file = open(data_filename, O_RDONLY);
	
	if (data_file == -1) {
		printf("Opening of file %s failed: %s\n", data_filename, strerror(errno));
		exit(1);
	}
	
	//open map file
	map_file = open(map_filename, O_RDONLY);
	
	if (map_file == -1) {
		printf("Opening of file %s failed: %s\n", map_filename, strerror(errno));
		exit(1);
	}
	
	//determine length of map_file
	lseek(map_file, 0, SEEK_SET); 
	int size= 0; //set files size
	
	
	//initialize variables
	char file_name[13];
	int pos; //position determines where in the file it should go
	int index;
	int equal_index;
	int fd;
	bool file_exists = false;
	ssize_t buff[4096];
	
	//iterate through map file until empty
	while (read(map_file, file_name, 12) != 0) {
		
		//read the position integer
		read(map_file, &pos, 4);
		file_exists = false;
		
		//check for if the file has been previously created
		for (int i = 0; i < size; i++) {
			//if it has been created
			if(strcmp(file_name, files[i].file_name) == 0) {
				file_exists = true;
				equal_index = i;
				break;
			}
		}
		
		//if the file has not existed before
		if (!file_exists) {
			fd = open(file_name, O_RDWR | O_TRUNC | O_CREAT , S_IRUSR | S_IWUSR);
			if (fd < 0) {
				printf("-a");
				perror("Error printed by perror");
				exit(1);
			}
			memcpy(files[size].file_name, file_name, 12);
			files[size].fd = fd;
			size++;
		} else { //it does exist
			fd = files[equal_index].fd;
		}
		
		
		//lseek data file
		lseek(data_file, index * 4096, SEEK_SET);
		//lseek file directory
		lseek(fd, pos * 4096, SEEK_SET);
		//read the data file into a buffer
		read(data_file, &buff, 4096);
		//write it
		write(fd, buff, 4096);
		
		index++; //increment
		
		//if the size of used indexes is greater than half of the total
		if (size > .5 * files_max) {
			files =(struct fd*) realloc(files, 2 * (20 * files_max));
			files_max*=2;
		}
	}
	
	//close everything up
	for (int i = 0; i < size; i++) {
		close(files[i].fd);
	}
	close(data_file);
	close(map_file);
	
	free(files);
		
	
}