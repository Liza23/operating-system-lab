#include "simplefs-ops.h"
extern struct filehandle_t file_handle_array[MAX_OPEN_FILES]; // Array for storing opened files

int simplefs_create(char *filename){
    /*
	    Create file with name `filename` from disk
	*/
	int inodenum = simplefs_allocInode();
	if (inodenum == -1) return -1;

	struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
	
	simplefs_readInode(inodenum, inode);
	inode->status = INODE_IN_USE;
	for (int i = 0; i < sizeof(filename); i++) 
		inode->name[i] = filename[i];

	inode->file_size = 0;
	simplefs_writeInode(inodenum, inode);

    return inodenum;
}


void simplefs_delete(char *filename){
    /*
	    delete file with name `filename` from disk
	*/
    struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
    for(int i=0; i<NUM_INODES; i++){
        simplefs_readInode(i, inode);
        if(inode->status ==  INODE_IN_USE && !strcmp(inode->name,filename)){
			for(int i = 0; i<MAX_FILE_SIZE; i++) {
				if(inode->direct_blocks[i] != -1) {
					simplefs_freeDataBlock(inode->direct_blocks[i]);
				}
			}
			simplefs_freeInode(i);
			return;
		}
	}
}

int simplefs_open(char *filename){
    /*
	    open file with name `filename`
	*/
    struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
    for(int i=0; i<NUM_INODES; i++){
        simplefs_readInode(i, inode);
        if(inode->status ==  INODE_IN_USE && !strcmp(inode->name,filename)){
			    for(int j=0; j<MAX_OPEN_FILES; j++){
					if(file_handle_array[j].inode_number == -1) {
						file_handle_array[j].inode_number = i;
    				    file_handle_array[j].offset = 0;
						return j;
					}
				}
		}
	}

    return -1;
}

void simplefs_close(int file_handle){
    /*
	    close file pointed by `file_handle`
	*/
	file_handle_array[file_handle].inode_number = -1;
    file_handle_array[file_handle].offset = 0;
}

int simplefs_read(int file_handle, char *buf, int nbytes){
    /*
	    read `nbytes` of data into `buf` from file pointed by `file_handle` starting at current offset
	*/

	struct filehandle_t file = file_handle_array[file_handle];
	if (file.offset + nbytes > MAX_FILE_SIZE * BLOCKSIZE) 
		return -1;

	struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
	simplefs_readInode(file.inode_number, inode);

	char temp[BLOCKSIZE];
	int bytesRead = 0;
	int off = file.offset;

	for (int j = 0; j < MAX_FILE_SIZE; j++){
		if(off < (j + 1) * BLOCKSIZE) {
			if (inode->direct_blocks[j] != -1 && bytesRead < nbytes){
				simplefs_readDataBlock(inode->direct_blocks[j], temp);

				for(int i = 0; i < BLOCKSIZE; i++) {
					if(i >= off - j * BLOCKSIZE && bytesRead < nbytes) {
						buf[bytesRead] = temp[i];
						bytesRead++;
					}
				}	
				off = (j + 1)*BLOCKSIZE;
				if(bytesRead == nbytes) break;
			}
		}
	}	
    return 0;
}


int simplefs_write(int file_handle, char *buf, int nbytes){
    /*
	    write `nbytes` of data from `buf` to file pointed by `file_handle` starting at current offset
	*/
	struct filehandle_t file = file_handle_array[file_handle];
	if (file.offset + nbytes > MAX_FILE_SIZE * BLOCKSIZE) 
		return -1;

	struct inode_t *inode = (struct inode_t *)malloc(sizeof(struct inode_t));
	simplefs_readInode(file.inode_number, inode);

	inode->file_size += nbytes;
	char temp[BLOCKSIZE];
	int bytesWritten = 0;
	int off = file.offset;

	for (int j = 0; j < MAX_FILE_SIZE; j++){
		if(off < (j + 1) * BLOCKSIZE) {
		if(inode->direct_blocks[j] != -1) {
			simplefs_readDataBlock(inode->direct_blocks[j], temp);
			for(int k = 0; k < BLOCKSIZE; k++) {
				if(k >= off - j * BLOCKSIZE && bytesWritten < nbytes) {
					temp[k] = buf[bytesWritten];
					bytesWritten++;
				}
			}
			off = (j + 1)*BLOCKSIZE;
			simplefs_writeDataBlock(inode->direct_blocks[j], temp);		
			simplefs_writeInode(file.inode_number, inode);
		}

		else if (inode->direct_blocks[j] == -1){
			for(int k = 0; k < BLOCKSIZE; k++) {
				if(bytesWritten < nbytes) {
					temp[k] = buf[bytesWritten];
					bytesWritten++;
				}
			}	
			off = (j + 1)*BLOCKSIZE;

			int blocknum = simplefs_allocDataBlock();
			if(blocknum == -1) 
				return -1;
			
			inode->direct_blocks[j] = blocknum;
			simplefs_writeDataBlock(inode->direct_blocks[j], temp);
			simplefs_writeInode(file.inode_number, inode);
		}
			if(bytesWritten == nbytes) break;
		}
	}	
    return 0;
}


int simplefs_seek(int file_handle, int nseek){
    /*
	   increase `file_handle` offset by `nseek`
	*/
	int offset = file_handle_array[file_handle].offset; 
	file_handle_array[file_handle].offset += nseek;

	if(file_handle_array[file_handle].offset > MAX_FILE_SIZE * BLOCKSIZE 
				|| file_handle_array[file_handle].offset < 0) 
	{
		file_handle_array[file_handle].offset = offset;
		return -1;
	}
    
	return 0;
}