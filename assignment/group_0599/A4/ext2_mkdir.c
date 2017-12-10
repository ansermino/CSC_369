#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"
#include <string.h>
#define EEXIST 17 /* File exists */
#define ENOENT 2  /* No such file or directory */

unsigned char *disk;
struct ext2_super_block *super_block;
struct ext2_group_desc *group_desc;
unsigned char *block_bitmap;
unsigned char *inode_bitmap;
struct ext2_inode *inodes;
unsigned int inode_index_given_name_parent(struct ext2_inode *parent_inode, char* name, unsigned char type);
struct ext2_inode* inode_given_path(char *input);

struct ext2_inode* inode_given_path(char *input){
	char real_path[1024];
	int path_len = strlen(input);
	strcpy(real_path, input);
	real_path[path_len + 1] = '\0';

	unsigned int num;

	// make the parent inode point to the root
	struct ext2_inode *parent_inode = &inodes[EXT2_ROOT_INO - 1];
	if (path_len != 1){
		char* temp = strtok(real_path, "/");
		while (temp != NULL){
			num = inode_index_given_name_parent(parent_inode, temp, EXT2_FT_DIR);
			if (num == 0){
				return NULL;
			}
			parent_inode = &inodes[num-1];
			temp = strtok(NULL, "/");
		}
	}
	else if (path_len == 1){
		if (real_path[0] == '/'){
			return parent_inode;
		}
	}
	return parent_inode;
}

unsigned int inode_index_given_path(char* input){

	if (strlen(input) == 1){
		if(input[0] == '/'){
			return EXT2_ROOT_INO ;
		}
	}

	char real_path[1024];
	int path_len = strlen(input);
	strcpy(real_path, input);
	real_path[path_len + 1] = '\0';
	unsigned int num = 0;

	struct ext2_inode *parent_inode = &inodes[EXT2_ROOT_INO - 1];
	if (path_len != 1){
		char* temp = strtok(real_path, "/");
		while (temp != NULL){
			num = inode_index_given_name_parent(parent_inode, temp, EXT2_FT_DIR);
			if (num == 0){
				return -1;
			}
			parent_inode = &inodes[num-1];
			temp = strtok(NULL, "/");
		}
	}
	else if (path_len == 1){
		if (real_path[0] == '/'){
			return num;
		}
	}
	return num;
}

unsigned int inode_index_given_name_parent(struct ext2_inode *parent_inode, char* name, unsigned char type){

	struct ext2_dir_entry *entry;
	int count = 0;
	int memory_count;
	int flag1 = 0;
	int flag2 = 0;
	int flag3 = 0;

	// loop through the inode's i_block
	while (count < 15){
		if (parent_inode->i_block[count]!=0){
			memory_count = 0;
			entry = (struct ext2_dir_entry*)(disk + EXT2_BLOCK_SIZE * parent_inode->i_block[count]);
			while(memory_count < EXT2_BLOCK_SIZE){
				if (strlen(name) == entry->name_len){
					flag1 = 1;
				}
				if (strncmp(name, entry->name, entry->name_len) == 0){
					flag2 = 1;
				}
				if (type & entry->file_type){
					flag3 = 1;
				}
				if ((flag1 == 1) & (flag2 == 1) & (flag3 == 1)){
					return entry->inode;
				}
			flag1 = 0;flag2 = 0;flag3 = 0;
			memory_count += entry->rec_len;
			entry = (struct ext2_dir_entry*)(disk + EXT2_BLOCK_SIZE * parent_inode->i_block[count] + memory_count);
				}
		}
		count ++;
		}
		return 0;
}


unsigned int init_inode(){
	unsigned char *pointer = inode_bitmap;
	unsigned char temp;
	int flag1 = 0;
	int flag2 = 0;
	int count = 0;
	int count2 = 0;
	int ind = 1;

	while (count < (super_block->s_inodes_count)/sizeof(char*)){
		temp = *pointer;
		while (count2 < 8) {

			if(ind >= 11){
				flag1 = 1;
			}
			if(!((temp>>(count2)) & 1)){
				flag2 = 1;
			}
			if ((flag1) & (flag2)){
				return ind;
			}
			flag1 = 0;flag2 = 0;
			count2 ++;
			ind++;
		}
		count2 = 0;
		count ++;
		pointer++;
	}
	return 0;
}

unsigned int init_block(){
	unsigned char *pointer = block_bitmap;
	unsigned char temp;
	int count = 0;
	int count2 = 0;
	int ind = 1;
	int section_num = (super_block->s_blocks_count)/sizeof(char*);
	while (count < section_num ){
			temp = *pointer;

			while (count2 < 8) {
				if(!((temp>>count2) & 1)){
					return ind;
				}
				ind ++;
				count2 ++;
			}
			count2 = 0;
			count ++;
			pointer++;
	}
	return 0;
}

void block_bitmap_alter(unsigned int ind, int result){
	int off = ind / (sizeof(char*));
	int rem = ind % (sizeof(char*));

	unsigned char* position = block_bitmap + off;
	*position = ((*position & ~(1 << rem)) | (result << rem));
}

void inode_bitmap_alter(unsigned int ind, int result){
	int off = ind / (sizeof(char*));
	int rem = ind % (sizeof(char*));

	unsigned char* position = inode_bitmap + off;
	*position = ((*position & ~(1 << rem)) | (result << rem));
}

int alter_inode(unsigned int ind, unsigned short mode, unsigned int size, unsigned int new_block){

	//subject to change
	inodes[ind].i_mode = mode;
	inodes[ind].i_uid = 0;
	inodes[ind].i_gid = 0;
	inodes[ind].i_dtime = 0;
	inodes[ind].osd1 = 0;
	inodes[ind].i_links_count = 0;
	inodes[ind].i_blocks = 2;
	inodes[ind].i_size = size;
	inodes[ind].i_block[0] = 1 + new_block;
	inodes[ind].i_generation = 0;
	inodes[ind].i_file_acl = 0;
	inodes[ind].i_dir_acl = 0;
	inodes[ind].i_faddr = 0;
	inodes[ind].extra[0] = 0;
	inodes[ind].extra[1] = 0;
	inodes[ind].extra[2] = 0;
	return 0;
}

int get_entry_size(char* name){
	int result = 8;
	if((strlen(name)%4) != 0){
		result += (strlen(name)/4)*4 + 4;
	}else{
		result += strlen(name);
	}

	return result;
}

struct ext2_dir_entry * add_dir_entry(unsigned char file_type, struct ext2_inode *parent_inode, struct ext2_dir_entry* new_entry, char* new_name){

	int memory_record;
	int actual_rec_len;
	struct ext2_dir_entry *entry;
	for (int i = 0; i < parent_inode->i_blocks/2; i ++){
		memory_record = 0;
		while (memory_record < EXT2_BLOCK_SIZE){
			entry = (struct ext2_dir_entry*)(disk + EXT2_BLOCK_SIZE*parent_inode->i_block[i] + memory_record);
			if (memory_record + entry->rec_len == EXT2_BLOCK_SIZE){
				entry = (struct ext2_dir_entry*)(disk + EXT2_BLOCK_SIZE*parent_inode->i_block[i] + memory_record);
				actual_rec_len = get_entry_size(entry->name);
				if(actual_rec_len + new_entry->rec_len < entry->rec_len){
					entry->rec_len = actual_rec_len;
					// move to the
					entry = (struct ext2_dir_entry*)(disk + EXT2_BLOCK_SIZE*parent_inode->i_block[i] + memory_record + actual_rec_len);
					entry-> inode = new_entry->inode;
					entry->rec_len = EXT2_BLOCK_SIZE - actual_rec_len - memory_record;
					strcpy(entry->name, new_name);
					entry->name_len = strlen(new_name);
					entry->name[strlen(entry->name)] = '\0';
					entry->file_type = file_type;
					parent_inode->i_links_count++;
					return entry;
				}
			}
			memory_record += entry->rec_len;
		}
	}
	return NULL;
}

int check_inter(char* parent_path){
	if (strlen(parent_path) == 1){
		if(parent_path[0] == '/'){
			return 0;
		}
	}

	char copy[1024];
	strcpy(copy, parent_path);
	char* temp = strtok(copy, "/");
	struct ext2_inode *parent_inode = &inodes[EXT2_ROOT_INO - 1];
	int num;
	while (temp != NULL){
		num = inode_index_given_name_parent(parent_inode, temp, EXT2_FT_DIR);
		parent_inode = &inodes[num-1];
		temp = strtok(NULL, "/");
		if (num == 0){
			return -1;
		}
	}
	return 0;
}

int main(int argc, char **argv) {

	if(argc != 3) {
	    fprintf(stderr, "Usage: <disk name> <absolute_path>\n");
	    exit(1);
	}

	char* chosen_disk = argv[1];
	char* chosen_path = argv[2];

	// initialize the disk
	int fd = open(chosen_disk, O_RDWR);
	disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(disk == MAP_FAILED) {
    perror("mmap");
    exit(1);
	}

	// find useful component
	super_block = (struct ext2_super_block*)(disk + EXT2_BLOCK_SIZE);
	group_desc = (struct ext2_group_desc*)(disk + EXT2_BLOCK_SIZE*2);
	block_bitmap = (unsigned char*)(disk + EXT2_BLOCK_SIZE * group_desc->bg_block_bitmap);
	inode_bitmap = (unsigned char*)(disk + EXT2_BLOCK_SIZE * group_desc->bg_inode_bitmap);
	inodes = (struct ext2_inode*)(disk + EXT2_BLOCK_SIZE * group_desc->bg_inode_table);

	//check if the path already exists
	char* real_path = malloc(256);
	int path_len = strlen(chosen_path);
	if (chosen_path[path_len - 1] == '/'){
		strcpy(real_path, chosen_path);
		real_path[path_len + 1] = '\0';
		real_path[path_len + 2] = '\0';
	} else if (chosen_path[path_len - 1] != '/'){
		strcpy(real_path, chosen_path);
		strcat(real_path, "/");
		real_path[path_len + 1] = '\0';
	}

	if (inode_given_path(real_path) != NULL){
		return EEXIST;
	}

	char* parent_path;
	//start getting the parent's inode and other information.
	int k = strlen(real_path) - 2;
	while ((k > -1) & (real_path[k]!='/')){
		k --;
	}

	if (k < 0){
		parent_path = malloc(2);
		strcpy(parent_path, "/");
		parent_path[1] = '\0';
	}else{
		parent_path = malloc(k+1);
		strncpy(parent_path, real_path, k+1);
		parent_path[k+1] = '\0';
	}

	if (check_inter(parent_path) == -1){
		return ENOENT;
	}

	unsigned int inode_index = init_inode();
	unsigned int block_index = init_block();


	block_bitmap_alter(block_index - 1, 1);
	inode_bitmap_alter(inode_index - 1, 1);
	group_desc->bg_free_blocks_count --;
	group_desc->bg_free_inodes_count --;
	group_desc->bg_used_dirs_count ++;
	super_block->s_free_blocks_count --;
	super_block->s_free_inodes_count --;
	alter_inode(inode_index - 1, EXT2_S_IFDIR, EXT2_BLOCK_SIZE, block_index - 1);


	char* new_name = malloc(256);
	char copy_path[1024];
	strcpy(copy_path, real_path);
	char* temp = strtok(copy_path, "/");
	while (temp != NULL){
		strcpy(new_name, temp);
	temp = strtok(NULL, "/");
	}
	new_name[strlen(new_name)] = '\0';

	struct ext2_inode* parent_inode = inode_given_path(parent_path);
	int parent_inode_index = inode_index_given_path(parent_path);

	//add an entry to the parent's i_block
	struct ext2_dir_entry* new_entry = malloc(sizeof(struct ext2_dir_entry));
	new_entry -> inode = inode_index;
	new_entry -> file_type = EXT2_FT_DIR ;
	new_entry -> name_len = strlen(new_name);
	new_entry -> rec_len = get_entry_size(new_name);
	strcpy(new_entry->name, new_name);
	add_dir_entry(EXT2_FT_DIR, parent_inode, new_entry, new_name);


	struct ext2_dir_entry* current = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE*(block_index));
	char* current_name = malloc(2);
	current_name[0] = '.';
	current_name[1] = '\0';
	current -> inode = inode_index;
	current -> file_type = EXT2_FT_DIR ;
	current -> name_len = strlen(current_name);
	current -> rec_len = 12;
	memset(current->name, '.', 1);
	current->name[1] = '\0';
	inodes[inode_index - 1].i_links_count ++;

	struct ext2_dir_entry* upper = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE*(block_index) + current->rec_len);
	char* upper_name = malloc(3);
	upper_name[0] = '.';
	upper_name[1] = '.';
	upper_name[2] = '\0';
	upper -> inode = parent_inode_index;
	upper -> file_type = EXT2_FT_DIR ;
	upper -> name_len = strlen(upper_name);
	upper -> rec_len = 1012;
	memset(upper->name, '.', 2);
	upper->name[2] = '\0';
	inodes[inode_index - 1].i_links_count ++;

	return 0;
}
