#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include "flash_img_gen.h"

#define ARG_NUM_REQ   4


typedef struct
{
	tHeaderRow row;
	uint32_t size;
	void *next;
} tFileList;

void image_add_file_to_list(tFileList **list, const char *path, const char *fname)
{
	char fullpath[256];
	int fd;
	struct stat st;
	tFileList *ptr;
	
	if ( (!path) || (!fname))
	{
		return;
	}
	
	sprintf(fullpath, "%s/%s", path, fname);
	if (!stat(fullpath, &st))
	{
		if (S_ISREG(st.st_mode))
		{
			if (*list)
			{	
				uint32_t size_prev;

				ptr = *list;

				do 
				{
					if (!ptr->next)
					{
						break;
					}
					ptr = ptr->next;
				} while(1);
				size_prev = ptr->size;

				ptr->next = (tFileList *)malloc(sizeof(tFileList));
				ptr = ptr->next;
				ptr->row.offset = ptr->row.offset + size_prev;
			} else
			{
				ptr = (tFileList *)malloc(sizeof(tFileList));
				ptr->row.offset = 0;
				*list = ptr;
			}
			ptr->next = NULL;
			ptr->size = st.st_size;
			//TODO
			strncpy(ptr->row.fname, fname, MAX_FILE_NAME);
		}
	}
}

int image_write_header(int fd, tFileList *head, uint32_t file_count, int conv)
{
	int i;
	tFileList *current = head;
	uint16_t row_counter;
	uint32_t start_data = file_count * sizeof(tHeaderRow)  + sizeof(row_counter);
	
	if (conv)
	{
		row_counter = htons(file_count);
	} else
	{
		row_counter = file_count;
	}
	
	if (write(fd, &row_counter, sizeof(row_counter)) != sizeof(row_counter))
	{
		fprintf(stderr, "Failed to write image file\n");
		
		return -1;
	}
	
	for (i = 0; i < file_count; i++)
	{
		current->row.offset += start_data;
		if (conv)
		{
			current->row.offset = htons(current->row.offset);
		}
		
		if (write(fd, &current->row, sizeof(tHeaderRow)) != sizeof(tHeaderRow))
		{
			return -1;
		}
		
		current = current->next;
	}
	
	return 0;
}

int image_cpy_fd(int dst_fd, int src_fd)
{
	ssize_t bytes = 0;
	uint8_t buff[4096];

	while ((bytes = read(src_fd, buff, sizeof(buff))) > 0)
	{
		if (write(dst_fd, buff, bytes) != bytes)
		{
			return 1;
		}
	}
	
	if (bytes < 0)
	{
		return 1;
	}
	
	return 0;
}

int image_cpy_file_fd(int dst_fd, const char *path)
{
	int src_fd = open(path, O_RDONLY);
	if (src_fd < 0)
	{
		return 1;
	}
	
	if (image_cpy_fd(dst_fd, src_fd))
	{
		return 1;
	}
	
	close(src_fd);
	return 0;
}

int image_write_tail(int fd, tFileList *head, const char *dir, const uint32_t file_count)
{
	int i;
	char path[255];
	tFileList *current = head;
	
	for (i = 0; i < file_count; i++)
	{
		sprintf(path, "%s/%s", dir, current->row.fname);
		if (image_cpy_file_fd(fd, path))
		{
			return 1;
		}
		current = current->next;
	}

	return 0;
}

void image_generate(tFileList *list, const char *image_name, const char *dir, const uint32_t file_count, int conv)
{
	int fd = open(image_name, O_WRONLY | O_CREAT, S_IRGRP | S_IWUSR | S_IRUSR);
	
	if (fd < 0)
	{
		fprintf(stderr, "Failed to create image file\n");
		return;
	}

	if (image_write_header(fd, list, file_count, conv))
	{
		fprintf(stderr, "Failed to write header image\n");
		close(fd);
		return;
	}
	if (image_write_tail(fd, list, dir, file_count))
	{
		fprintf(stderr, "Failed to write tail of image\n");	
	}
	
	close(fd);
}


void free_list(tFileList *list)
{
	tFileList *tmp;
	tFileList *cur = list;
	
	do
	{
		tmp = cur;
		cur = cur->next;
		free(tmp);
		if (!cur)
		{
			break;
		}
	} while (cur->next);
}

int main (int argc, char *argv[])
{	
	int conv = 0;
	tFileList *list = NULL;
	uint16_t      file_count = 0;
	char *dir; 
	char *image_name;
	struct dirent *dp;
	DIR *dfd;
	
	if (argc != ARG_NUM_REQ)
	{
		fprintf(stderr, "Not enought arguments. Usage flash_img_gen dir conv output_file_name\n");
		return -1;
	}
	
	dir = argv[1];
	if (strcmp(argv[2], "0"))
	{
		conv = 1;
	}
	
	image_name = argv[3];
	if ((dfd = opendir(dir)) == NULL)
	{
		fprintf(stderr, "Failed to open directory\n");
		return -1;
	}
	
	while ((dp = readdir(dfd)))
	{
		if ((strcmp(dp->d_name, ".")) && (strcmp(dp->d_name, "..")))
		{
			image_add_file_to_list(&list, dir, dp->d_name);
			printf("Add file: %s/%s\n", dir, dp->d_name);
			file_count++;
		}
	}
	printf("Gen image file: %s Total files: %d\n", image_name, file_count);

	if (file_count)
	{
		image_generate(list, image_name, dir, file_count, conv);
		free_list(list);
	}
	
	return 0;
}
