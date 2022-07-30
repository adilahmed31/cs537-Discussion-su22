#include <stdio.h>
#include "ext2_fs.h"
#include "read_ext2.h"

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("expected usage: ./runscan inputfile outputfile\n");
		exit(0);
	}
	
	int fd;

	fd = open(argv[1], O_RDONLY);    /* open disk image */

	ext2_read_init(fd);

	struct ext2_super_block super;
	struct ext2_group_desc group;
	
	// example read first the super-block and group-descriptor
	read_super_block(fd, 0, &super);
	read_group_desc(fd, 0, &group);
	
	printf("There are %u inodes in an inode table block and %u blocks in the idnode table\n", inodes_per_block, itable_blocks);
	//iterate the first inode block
	off_t start_inode_table = locate_inode_table(0, &group);
    for (unsigned int i = 0; i < inodes_per_block; i++) {
            printf("inode %u: \n", i);
            struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
            read_inode(fd, 0, start_inode_table, i, inode);
	    /* the maximum index of the i_block array should be computed from i_blocks / ((1024<<s_log_block_size)/512)
			 * or once simplified, i_blocks/(2<<s_log_block_size)
			 * https://www.nongnu.org/ext2-doc/ext2.html#i-blocks
			 */
			unsigned int i_blocks = inode->i_blocks/(2<<super.s_log_block_size);
            printf("number of blocks %u\n", i_blocks);
             printf("Is directory? %s \n Is Regular file? %s\n",
                S_ISDIR(inode->i_mode) ? "true" : "false",
                S_ISREG(inode->i_mode) ? "true" : "false");
			
			// print i_block numberss
			for(unsigned int i=0; i<EXT2_N_BLOCKS; i++)
			{       if (i < EXT2_NDIR_BLOCKS)                                 /* direct blocks */
							printf("Block %2u : %u\n", i, inode->i_block[i]);
					else if (i == EXT2_IND_BLOCK)                             /* single indirect block */
							printf("Single   : %u\n", inode->i_block[i]);
					else if (i == EXT2_DIND_BLOCK)                            /* double indirect block */
							printf("Double   : %u\n", inode->i_block[i]);
					else if (i == EXT2_TIND_BLOCK)                            /* triple indirect block */
							printf("Triple   : %u\n", inode->i_block[i]);

			}
			
            free(inode);

        }

	
	close(fd);
}