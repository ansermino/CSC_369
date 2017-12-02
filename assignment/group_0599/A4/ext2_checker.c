#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

unsigned char *disk;
struct ext2_super_block *super_block;
struct ext2_group_desc *group_desc;
unsigned char *block_bitmap;
unsigned char *inode_bitmap;
struct ext2_inode *inodes;

int open_image(char **argv){
  int fd = open(argv[1], O_RDWR);
  disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(disk == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }


  super_block = (struct ext2_super_block*)(disk + EXT2_BLOCK_SIZE);
  group_desc = (struct ext2_group_desc*)(disk + EXT2_BLOCK_SIZE*2);
  inodes = (struct ext2_inode*)(disk + EXT2_BLOCK_SIZE * group_desc->bg_inode_table);

  int total_fixes = 0;
  //1
  total_fixes = check_bitmap(disk);
  //2345
  total_fixes += check_inode(disk);
  return total_fixes;
}

//1check super_block and group_desc block_bitmap
int check_bitmap(unsigned char *disk){
  int total_fixes = 0;
  struct ext2_group_desc *address = (struct ext2_group_desc *)(disk + 1024*2);
  struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);

  printf("count gd block bitmap: \n");
  unsigned char *block_bitmap = (unsigned char*)(disk + 1024*3);
  int used_inode_count = 0;
  for (int i=0 ; i<=15 ; i++){
     for (int j=0 ; j<8 ; j++){
         if((block_bitmap[i] >> j) & 1) used_block_count++;
     }
  }
  printf("used_block_count is %d\n", used_block_count);
  if (used_block_count != (128 - address->bg_free_blocks_count){
    address->bg_free_blocks_count = 128 - used_block_count;
    total_fixes += 128 - used_block_count;
    printf("Fixed: block group's free blocks counter was off by %d compared to the bitmap\n",(128 - used_block_count));
  }

  printf("count gd inode bitmap: \n");
  unsigned char *inode_bitmap = (unsigned char*)(disk + 1024*4);
  int used_inode_count = 0;
  for (int i=0 ; i<=3 ; i++){
     for (int j=0 ; j<8 ; j++){
         if (inode_bitmap[i] >> j) & 1) used_inode_count++;
     }
  }
  printf("used_inode_count is %d\n", used_inode_count);
  if (used_inode_count != (32 - address->bg_free_inodes_count)){
    address->bg_free_inodes_count = 32 - used_inode_count;
    total_fixes += 32 - used_inode_count;
    printf("Fixed: block group's free inodes counter was off by %d compared to the bitmap\n", (32 - used_inode_count));
  }


  // NOTE :sb count
  printf("count sb block bitmap: \n");
  unsigned char *block_bitmap = (unsigned char*)(disk + 1024*3);
  used_block_count = 0;
  for (int i=0 ; i<=15 ; i++){
     for (int j=0 ; j<8 ; j++){
         if((block_bitmap[i] >> j) & 1) used_block_count++;
     }
  }
  printf("used_block_count is %d\n", used_block_count);
  if (used_block_count != (128 - sb->s_free_blocks_count){
    sb->s_free_blocks_count = 128 - used_block_count;
    total_fixes += 128 - used_block_count;
    printf("Fixed: superblock's free blocks counter was off by %d compared to the bitmap\n",(128 - used_block_count));
  }

  printf("count sb inode bitmap: \n");
  unsigned char *inode_bitmap = (unsigned char*)(disk + 1024*4);
  used_inode_count = 0;
  for (int i=0 ; i<=3 ; i++){
     for (int j=0 ; j<8 ; j++){
         if (inode_bitmap[i] >> j) & 1) used_inode_count++;
     }
  }
  printf("used_inode_count is %d\n", used_inode_count);
  if (used_inode_count != (32 - sb->s_free_inodes_count)){
    sb->bg_free_inodes_count = 32 - used_inode_count;
    total_fixes += 32 - used_inode_count;
    printf("Fixed: superblock's free inodes counter was off by %d compared to the bitmap\n", (32 - used_inode_count));
  }

  return total_fixes;
}

int check_inode(unsigned char *disk){
  super_block = (struct ext2_super_block*)(disk + EXT2_BLOCK_SIZE);
  group_desc = (struct ext2_group_desc*)(disk + EXT2_BLOCK_SIZE*2);
  block_bitmap = (unsigned char*)(disk + EXT2_BLOCK_SIZE * group_desc->bg_block_bitmap);
  inode_bitmap = (unsigned char*)(disk + EXT2_BLOCK_SIZE * group_desc->bg_inode_bitmap);

  // give a ~ name and check files recursively
  struct ext2_inode *root_inode = &inodes[EXT2_ROOT_INO - 1];
  return loop_check(root_inode);
}

int loop_check(struct ext2_inode *current_inode){
  int total_fixes = 0;

  struct ext2_dir_entry *entry;
  int count2 = 0;
  int memory_count;
  char type;
  struct ext2_group_desc *address = (struct ext2_group_desc *)(disk + 1024*2);
  struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);

  group_desc = (struct ext2_group_desc*)(disk + EXT2_BLOCK_SIZE*2);

  while(count2 < 32){
    if (count2 == 1 || count2 > 10){
      if(inodes[count2].i_size != 0){
        entry = (struct ext2_dir_entry*)(disk + 1024*inodes[count2].i_block[p]);
        if (inodes[count2].i_mode & EXT2_S_IFDIR) || (inodes[count2].i_mode & EXT2_S_IFREG) || (inodes[count2].i_mode & EXT2_S_IFLNK){
          //b d
          if(inodes[count2].i_mode != entry->file_type){
            entry->file_type = inodes[count2].i_mode;
            printf("Fixed: Entry type vs inode mismatch: inode [%d]\n",%count2);
            total_fixes += 1;
          }
          if(inodes[count2].i_dtime != 0){
            inodes[count2].i_dtime = 0;
            printf("Fixed: valid inode marked for deletion: [%d]\n", %count2);
            total_fixes += 1;
          }

          //c
          unsigned char *inode_bitmap = (unsigned char*)(disk + 1024*4);
          block_bitmap = (unsigned char*)(disk + EXT2_BLOCK_SIZE * group_desc->bg_block_bitmap);
          if(current_inode->i_mode == EXT2_S_IFLNK || current_inode->i_mode == EXT2_S_IFREG || current_inode->i_mode == EXT2_S_IFDIR){
            if (!(inode_bitmap >> count2) & 1)){
              inode_bitmap >> count2 = 1;
              printf("Fixed: Entry type vs inode mismatch: inode [%d]\n", count2);
              total_fixes += 1;
            }

          }

          int p = 0;
          int data_block_unmarked = 0;
          while(p<15){
            if (inodes[count2].i_block[p] != 0){
              unsigned char *block_bitmap = (unsigned char*)(disk + 1024*3);
              if ((block_bitmap >> inodes[count2].i_block[p]) & 0){
                data_block_unmarked++;
                (block_bitmap >> inodes[count2].i_block[p]) = 1;
              }
            }
            p++;
          }
          if (data_block_unmarked > 0){
            address->bg_free_blocks_count -= data_block_unmarked;
            sb->s_free_blocks_count -= data_block_unmarked;
            printf("Fixed: %d in-use data blocks not marked in data bitmap for inode: [%d]\n",%data_block_unmarked,%count2 );
            total_fixes += data_block_unmarked;
          }
        }
      }
    }
    count2++;
  }
  return total_fixes;
}

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
			printf("the temp : %s \n", temp);
			num = inode_index_given_name_parent(parent_inode, temp, EXT2_FT_DIR);
			printf("inode num : %d\n", num);
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

int main(int argc, char **argv) {

  if(argc != 2) {
    fprintf(stderr, "Usage: <disk name> <absolute_path>\n");
    exit(1);
  }
  int temp = open_image(argv);
  printf("%d\n", temp);
}
