/* project3.c 
 *
 * Names: Jackson Burns, David Kim
 *	
 * 	Honor Code: This work complies with the JMU honor code.
 *	We did not recieve unauthorized help on this assignment.
 *
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

#include "common.h"
#include "classify.h"

int main(int argc, char *argv[])
{
    int input_fd;
    int classification_fd, map_fd;
    int cluster_number;
    int bytes_read;
    unsigned char classification, cluster_type;
    char cluster_data[CLUSTER_SIZE];
    
    // We only accept running with one command line argumet: the name of the
    // data file
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
    

	classification_fd = open(CLASSIFICATION_FILE, O_RDWR | O_TRUNC | O_CREAT , S_IRUSR | S_IWUSR); //open the classification file
	if (classification_fd < 0) {
        printf("Error opening file \"%s\": %s\n", CLASSIFICATION_FILE, strerror(errno));
        return 1;
    }
	
	
	// Iterate through all the clusters, reading their contents 
    // into cluster_data
    cluster_number = 0;
    while ((bytes_read = read(input_fd, &cluster_data, CLUSTER_SIZE)) > 0) {
        assert(bytes_read == CLUSTER_SIZE);
        classification = TYPE_UNCLASSIFIED;
        
		
		if (has_jpg_header(cluster_data) == 1) { //if has jpg header
			classification += TYPE_JPG_HEADER; 
			
		}
		
		if (has_jpg_body(cluster_data) == 1 || (has_jpg_body(cluster_data) == 0 
		&& (has_jpg_header(cluster_data) == 1 || has_jpg_footer(cluster_data) == 1))) { //jpg data
			classification += TYPE_IS_JPG;
		}
		if (has_jpg_footer(cluster_data) == 1) { //jpg footer
			classification += TYPE_JPG_FOOTER; 
			
		}
		
		if (has_html_header(cluster_data) == 1) { //html header
			classification+= TYPE_HTML_HEADER;
			
		}
		if ( has_html_body(cluster_data) == 1 || (has_html_body(cluster_data) == 0 
		&& (has_html_header(cluster_data) == 1 || has_html_footer(cluster_data) == 1))) { //html data
			classification += TYPE_IS_HTML;
			
		}
		if (has_html_footer(cluster_data) == 1) { //html footer
			classification += TYPE_HTML_FOOTER;
			
		}
		
		//write into the classification file
		write(classification_fd, &classification, sizeof(classification));
        
        
        cluster_number++;
    }
    
    close(input_fd);
    // Try opening the classification file for reading, exit with appropriate
    // error message if open fails
	lseek(classification_fd, 0, SEEK_SET);
    
	
	
	
	//creates a map file
	map_fd = open(MAP_FILE, O_RDWR | O_TRUNC | O_CREAT , S_IRUSR | S_IWUSR);
	if (map_fd < 0) {
        printf("Error opening file \"%s\": %s\n", CLASSIFICATION_FILE, strerror(errno));
        return 1;
    }
	
    // Iterate over each cluster, reading in the cluster type attributes
	int file_num = 1; //for naming purposes
	int html_file_num = 0;
    int jpg_file_num = 0;
	int html_cluster_num = 0; //determining the relative cluster
	int jpg_cluster_num = 0;
	char *html_filename = malloc(13);
	char *jpg_filename = malloc(13);
	
	
    while ((bytes_read = read(classification_fd, &cluster_type, 1)) > 0) {
		assert(bytes_read == 1);
		//jpg
		if(cluster_type == TYPE_JPG_HEADER) {
			
			if(jpg_file_num != file_num) { //set the number
				jpg_file_num = file_num;
			}
			file_num++; //increment the file number
			
			dprintf(map_fd, "file%04d.jpg", jpg_file_num);//write the data in
			write(map_fd, &jpg_cluster_num, sizeof(int));
			jpg_cluster_num++; //increment the cluster
		} 
		if(cluster_type == TYPE_JPG_HEADER + TYPE_IS_JPG) {
			if(jpg_file_num != file_num) { //set the number
				jpg_file_num = file_num;
			}
			file_num++; //increment the file number
			
			dprintf(map_fd, "file%04d.jpg", jpg_file_num);//write the data in
			write(map_fd, &jpg_cluster_num, sizeof(int));
			jpg_cluster_num++;//increment the cluster
		}
		if(cluster_type == TYPE_IS_JPG) {
			dprintf(map_fd, "file%04d.jpg", jpg_file_num);//write the data in
			write(map_fd, &jpg_cluster_num, sizeof(int));
			jpg_cluster_num++;//increment the cluster
		}
		if(cluster_type == TYPE_IS_JPG + TYPE_JPG_FOOTER) {
			dprintf(map_fd, "file%04d.jpg", jpg_file_num);//write the data in
			write(map_fd, &jpg_cluster_num, sizeof(int));
			jpg_cluster_num = 0; //resets cluster number
			
		}
		if(cluster_type == TYPE_JPG_FOOTER) {
			dprintf(map_fd, "file%04d.jpg", jpg_file_num);//write the data in
			write(map_fd, &jpg_cluster_num, sizeof(int));
			jpg_cluster_num = 0; //resets cluster number
		}
		if(cluster_type == TYPE_JPG_HEADER + TYPE_IS_JPG + TYPE_JPG_FOOTER) {
			if(jpg_file_num != file_num) { //set the number
				jpg_file_num = file_num;
			}
			file_num++; //increment the file number
			
			dprintf(map_fd, "file%04d.jpg", jpg_file_num); //write the data in
			write(map_fd, &jpg_cluster_num, sizeof(int));
			jpg_cluster_num = 0;//reset cluster
		}
		
		
		
		//html
		if(cluster_type == TYPE_HTML_HEADER) {
			if(html_file_num != file_num) { //set the number
				html_file_num = file_num;
			}
			file_num++; //increment the file number
			
			dprintf(map_fd, "file%04d.htm", html_file_num); //write the data in
			write(map_fd, &html_cluster_num, sizeof(int));
			html_cluster_num++; //increment the cluster
			
		}
		if(cluster_type == TYPE_HTML_HEADER + TYPE_IS_HTML) {
			if(html_file_num != file_num) {//set the number
				html_file_num = file_num;
			}
			file_num++; //increment the file number
			
			dprintf(map_fd, "file%04d.htm", html_file_num);//write the data in
			write(map_fd, &html_cluster_num, sizeof(int));
			html_cluster_num++;//increment the cluster
			
		}
		if(cluster_type == TYPE_IS_HTML) {
			dprintf(map_fd, "file%04d.htm", html_file_num);//write the data in
			write(map_fd, &html_cluster_num, sizeof(int));
			html_cluster_num++;//increment the cluster
			
		}
		if(cluster_type == TYPE_IS_HTML + TYPE_HTML_FOOTER) {
			dprintf(map_fd, "file%04d.htm", html_file_num);//write the data in
			write(map_fd, &html_cluster_num, sizeof(int));
			html_cluster_num = 0; //resets cluster number
		}
		if(cluster_type == TYPE_HTML_FOOTER) {
			dprintf(map_fd, "file%04d.htm", html_file_num);//write the data in
			write(map_fd, &html_cluster_num, sizeof(int));
			html_cluster_num = 0; //resets cluster number
			
		}
		if(cluster_type == TYPE_HTML_HEADER + TYPE_IS_HTML + TYPE_HTML_FOOTER) {
			if(html_file_num != file_num) { //set the number
				html_file_num = file_num;
			}
			file_num++; //increment the file number
			
			dprintf(map_fd, "file%04d.htm", html_file_num); //write the data in
			write(map_fd, &html_cluster_num, sizeof(int));
			html_cluster_num = 0;//reset the cluster
		}
		
    }
    
	//wrap up the program, no loose ends
	close(map_fd);
	close(classification_fd);
	free(html_filename);
	free(jpg_filename);
    return 0;
};
