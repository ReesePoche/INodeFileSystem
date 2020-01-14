//
// Reese Poche     | Justin Varley
// 894329194       | 898511484
// rpoche3@lsu.edu | jvarle1@lsu.edu
// 
// Inode-based filesystem

#include "softwaredisk.h"
#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/////////////////////////////////////////////////////////////////////////
///       the data structs that the program will use:                 ///
/////////////////////////////////////////////////////////////////////////

typedef struct FileInternals {
	unsigned long fromNode;
	FileMode mode;
	char name[256];
	unsigned long length;
	unsigned long direct1;
	unsigned long direct2;
	unsigned long direct3;
	unsigned long indirect;
	unsigned long Cpos;
}   FileInternals;
//-------------------------------------------------------------------------------------------

/////////////////////////////////////////////////////////
///       Globals that the FS uses                    ///
/////////////////////////////////////////////////////////

extern void getDBV(void);

int FS_init = 0;     //asks if values of FS have been initalized, default value is 0
					 //and changed when this is initalized
unsigned long **DBV; //bitvector made up of 1 parts. keeps track of which block and node 
					 //is written to
unsigned long **OFBV;//length 2 array of longs that keeps track of open files  
					 //so no file can be opened twice
//FSError * fserror;

void init_FS(void) {
	//get the mallocs out of the way first
	DBV = malloc(128 * sizeof(unsigned long *));//pulling bv from disk [which blocks/inodes free]
	OFBV = malloc(1 * sizeof(unsigned long *)); //keeps track of which files are open
	*OFBV = malloc(sizeof(unsigned long));
	init_software_disk();
	//fserror = malloc(sizeof(FSError));
	//next, get the values of the Bitvectors
	**OFBV = 0;
	getDBV();
	FS_init = 1;
} 
//-------------------------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
///      Functions that manipulate and read the Bit Vectors                  ///
///      The bit vectors(BV) are in the 1st two blocks of memory.            ///
////////////////////////////////////////////////////////////////////////////////

unsigned long flag = 1; //31 0's followed by a single 1 digit 

void resetFlag(void) {
	flag = 1;
}

void setFlag(unsigned long pos) {
	flag = flag << pos;
}
//-------------------------------------------------------------------------------------------

//////////////////////////////////////////
///      the OFBV minipulators         ///
//////////////////////////////////////////

//Testing:
//Open some to ensure that they open [range 0-19]
//Close some to ensure that they close

void OFBVopen(unsigned long nodeNum) {
	setFlag(nodeNum);
	**(OFBV) = (**(OFBV)|flag);   
	resetFlag();
}

void OFBVclose(unsigned long nodeNum) {
	//int  rs = 9;
	//setFlag(rs);
	setFlag(nodeNum);
	**(OFBV) = (**(OFBV)^flag);   
	resetFlag();
}

int OFBVisOpen(unsigned long nodeNum) {
	//int jv = 8;
	//setFlag(jv);
	setFlag(nodeNum);
	unsigned long out = (**(OFBV)&flag);   
	resetFlag();
	return (out != 0);
}
//-------------------------------------------------------------------------------------------

////////////////////////////////////////
///     the DBV manipulators         ///
////////////////////////////////////////

//Removed:
//sets the given unsigned long pointer to the 1st memory block of the sd
//the pointer must have allocated to space for a size 64 array
//only the right most 20 bits in the 1st element is the NBV**, may change
//
//Used:
//sets given unsigned long pointer to 1st two memory blocks of the sd
//the pointer must have allocated to space for a size 128 array
//only right most 20 bits of the first element(0) is the NBV**, may change
//DBV starts @ 21st right most bit of first element(0) & ends @ 15th right most bit in 
//78th element of the DBV

void getDBV(void) {
	//printf("made it inside of getDBV\nabout to malloc the DBV");
	//DBV=malloc(128*sizeof(unsigned long *));
	unsigned long block1[64];
	read_sd_block(block1, 0);
	for(int i = 0; i < 64; i++) {
		//*(DBV + i)=malloc(sizeof(unsigned long));
		*(DBV + i) = malloc(sizeof(unsigned long));
		**(DBV + i) = block1[i];
		//printf("the value of the %d block of the 1st is %ul\n", i, **(DBV + i));
	}
	unsigned long block2[64];
	read_sd_block(block2, 1);
	for(int i = 0; i < 64; i++) {
		*(DBV + i + 64) = malloc(sizeof(unsigned long));
		**(DBV + i + 64) = block2[i];
		//printf("the value of the %d block of the 2nd is %ul\n", i, **(DBV+i));
	}
}
//-------------------------------------------------------------------------------------------

//array pointer must be >= size 2
//1st element: element the bit is in
//2nd element: bits position in the element, read as nth bit from right
//takes NBV into account

void findTheBitsPosition(unsigned long **array, unsigned long blockNum) {
	if(blockNum < 13) {//looking for block designated for bv&&inodes
		//Fix the length of this print statement on screen
		printf("the block value %u is invalid it is less than 13, you just tried to find its "
		"element number this block does not exist\n", blockNum);
	}
	//finding corresponding bit in bv
	else if(blockNum < 57) {//first bit is wonky af
		*(array)= malloc(sizeof(unsigned long));
		**(array) = 0;
		*(array + 1) = malloc(sizeof(unsigned long));
		**(array + 1) = (20 + (blockNum - 13));
	}
	else if(blockNum < 5000) {
		*(array)= malloc(sizeof(unsigned long));
		**(array) = (blockNum - 57) / 64 + 1;
		*(array + 1) = malloc(sizeof(unsigned long));
		**(array + 1) = (blockNum - 57) % 64;
	}
	else {
		//Fix the length of this print statement on screen
		printf("the block value %u is invalid it is greater than 5000, you just tried to "
		"find its element number this block does not exist\n", blockNum);
		*(array)= malloc(sizeof(unsigned long));
		**(array) = 5000;
		*(array + 1) = malloc(sizeof(unsigned long));
		**(array + 1) = 5000;
	}
}
//-------------------------------------------------------------------------------------------

//sets bit that corresponds to the DB to 1
//only 12 < numbers < 5000 are valid
//assumes DBV retrieved
//the DBV is a size 128 unL array
//TAKE OUT THE IF STATEMENTS IN FINAL PRODUCT THEY ARE FOR TESTING PURPOSES ONLY

void assignDBBV(unsigned long blockNum) {//assign Data Block Bit Vector
	if(blockNum < 0) {
		printf("the block value %u is invalid it is less than 0 HOW!!\n", blockNum);
	}
	else if(blockNum < 2) {
		printf("the block value %u is invalid it is less than 2, YOU ARE TRYING TO OVERRIGHT "
		"THE BVS\n", blockNum);
	}
	else if(blockNum < 13) {
		printf("the block value %u is invalid it is less than 2, YOU ARE TRYING TO OVERRIGHT "
		"THE INODES\n", blockNum);
	}
	else if(blockNum > 4999) {
		printf("the block value %u is invalid it is greater than 5000, this block does not "
		"exist you tried to assign it\n", blockNum);
	}
	else {//changes bv and disk
		unsigned long **positions;
		positions= malloc(2 * sizeof(unsigned long *));
		findTheBitsPosition(positions, blockNum);
		setFlag(**(positions + 1));
		**(DBV + **(positions)) = (**(DBV + **(positions))|flag);
		resetFlag();
		//next we got to make the change offical and write it.
		if(**(positions) < 64) {//only need to change the 1st block 
			unsigned long mem[64];
			for(unsigned long i = 0; i < 64; i++) {
				mem[i] = **(DBV + i);
			}
		write_sd_block(mem, 0);
		}
		else {
			unsigned long mem[64];
			for(unsigned long i = 0; i < 64; i++) {
				mem[i] = **(DBV + i + 64);
			}
			write_sd_block(mem, 1);
		}
	}
}

//does the opposite of the above function
void freeDBBV(unsigned long blockNum) {
	if(blockNum < 0) {
		printf("the block value %u is invalid it is less than 0 HOW!! trying "
		"to free\n", blockNum);
	}
	else if(blockNum < 2) {
		printf("the block value %u is invalid it is less than 2, YOU ARE TRYING "
		"TO free THE BVS\n", blockNum);
	}
	else if(blockNum < 13) {
		printf("the block value %u is invalid it is less than 2, YOU ARE TRYING TO "
		"free THE INODES\n", blockNum);
	}
	else if(blockNum > 4999) {
		printf("the block value %u is invalid it is greater than 5000, this block does "
		"not exist you tried to free it\n", blockNum);
	}
	else {
		unsigned long **positions;
		positions = malloc(2 * sizeof(unsigned long *));
		findTheBitsPosition(positions, blockNum);
		setFlag(**(positions + 1));
		//Here is the only line difference between the two methods
		**(DBV + **(positions)) = (**(DBV + **(positions))^flag); 
		resetFlag();
		//next we got to make the change offical and write it.
		if(**(positions) < 64) { //only need to change the 1st block 
			unsigned long mem[64];
			for(unsigned long i = 0; i < 64; i++) {
				mem[i] = **(DBV + i);
			}
			write_sd_block(mem, 0);
		}
		else {
			unsigned long mem[64];
			for(unsigned long i = 0; i < 64; i++) {
				mem[i] = **(DBV + i + 64);
			}
			write_sd_block(mem, 1);
		}
	}
}
//-------------------------------------------------------------------------------------------
//will make bit corresponding to a particular inode be set to 1
//assumed that DBV is found when this function is called
//and just include the pointer to it
//this function will change DBV values and actual disk space
//for Version 1.0 only 20 inodes are supported so only values 0-19 are valid
//for the inode number

void assignINode(unsigned long nodeNum) {
	if(nodeNum < 0) {
		printf("you are trying to assign a node of position < 0 how? ");
	}
	else if(nodeNum > 19) {
		printf("you are trying to assign a node of position > 20");
	}
	else {
		setFlag(nodeNum);
		**(DBV) = (**(DBV)|flag);    //took out a + nodenum here did not belong (?)
		resetFlag();
		unsigned long mem[64];
		for(unsigned long i = 0; i < 64; i++) {
			mem[i] = **(DBV + i);
		}
		write_sd_block(mem, 0);
	}
}

void freeInode(unsigned long nodeNum) {
	if(nodeNum < 0) {
		printf("you are trying to free a node of position < 0 how? ");
	}
	else if(nodeNum > 19) {
		printf("you are trying to free a node of position > 20");
	}
	else{
		setFlag(nodeNum);
		**(DBV) = (**(DBV)^flag);   
		resetFlag();
		unsigned long mem[64];
		for(unsigned long i = 0; i < 64; i++) {
			mem[i] = **(DBV + i);
		}
		write_sd_block(mem, 0);
	}
}

//scans already found DBV and finds 1st available 
//inode that is free for writing [its bit is 0]
//if return = 20, there is no free inode

unsigned long findFreeInode(void) {
	unsigned long pos = 0;
	while(pos < 20 && ((**(DBV))&flag)) {
		setFlag(1);
		pos++;
	}
	resetFlag();
	if(pos > 19) { 
		printf("NoMoreInodes to write to");
	}
	return pos;
}
//-------------------------------------------------------------------------------------------

//returns block position of the 1st free DB
//returns 5000 if there is no free DB

unsigned long findFreeDB(void) {
	unsigned long pos = 13;
	unsigned long elementNum = 0;
	setFlag(20); //moves the flag to the 1st bit of the DBBV i.e skips the NBV
	while(elementNum < 128) {
		while(flag) {
			if(!((**(DBV + elementNum))&flag)) {//you found a free one
				resetFlag();
				return pos;
			}
			pos++;
			setFlag(1);
		}
		resetFlag();
		elementNum++;
	}
	printf("There is no Free DataBlock");
	return 5000;
}
//-------------------------------------------------------------------------------------------

//makes given pointer point to a length 3 array of unsigned longs:
//1st position: block number
//2nd position: byte number
//3rd position: amount of bytes that flow to the next block

void findStartOfNode(unsigned long nodeNum, unsigned long **array) {
	*(array) = malloc(sizeof(unsigned long));
	**(array) = (nodeNum * 275) / 512 + 2;
	*(array+1) = malloc(sizeof(unsigned long));
	**(array+1) = (nodeNum * 275) % 512;
	*(array+2) = malloc(sizeof(unsigned long));
	if((**(array + 1) + 275 < 512)) 
	**(array+2) =  0 ;
	else 
	 **(array+2) = **(array + 1) + 275 - 512;
}

//Assumes NBV has be retrieved 
//returns node number of the next node written to
//if return = 20, no other node written to
//if done in loop, increment i/o by 1 else same file will be found continuously

unsigned long findNextNode(unsigned long fromPos) {
	unsigned long pos = fromPos;
	//printf("made it to fNN1\n");//////////////////////////////////////////////////
	while(pos < 20) {
		if((**(DBV))&flag) {
			resetFlag();
			return pos;
		}
		setFlag(1);
		pos++;
	}
	resetFlag();
	return pos;
}
//-------------------------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
///    the parsing functions, allows the system to grab the data        //
///    from the disk andmake the structures required.                   //                        
//////////////////////////////////////////////////////////////////////////

//makes the char *node copy all 279 bytes of the node into it. 
//node should be of size 512 before attempting this 

void grabNodeFromDisk(char *node, unsigned long nodeNum) {
	unsigned long **pos;
	//printf("made it to GNFD1\n");//////////////////////////////////////////////////
	pos=malloc(3 * sizeof(unsigned long *));
	findStartOfNode(nodeNum, pos);
	char block[512];
	//printf("made it to GNFD2\n");//////////////////////////////////////////////////
	read_sd_block(block, **(pos));
	//printf("made it to GNFD3\n");//////////////////////////////////////////////////
	int count = 0;
	char * nodeHolder = malloc(sizeof(char)*279);
	if(**(pos+2) == 0)
		memcpy(nodeHolder, (block + **(pos+1)), 275);
	else {
		memcpy(nodeHolder, (block + **(pos+1)), 275 - **(pos+2));
		read_sd_block(block, (**(pos) + 1));
		memcpy((nodeHolder+ 274 - **(pos+2)), block, **(pos+2));
	}
	char * intHolder = malloc(sizeof(char)*4);
	int LTI = (int)nodeNum;
	memcpy(intHolder, &LTI, 4);
	memcpy((nodeHolder + 274), intHolder, 4);
	memcpy(node, nodeHolder, 279);
	//printf("made it to GNFDEND\n");//////////////////////////////////////////////////
}
//-------------------------------------------------------------------------------------------

//used with the above function to make a File
//file node and node num will be set immediatly after this function 
//node of size 279 bytes
void parseNode(char *node, File file) {
	file->Cpos=0;	
	memcpy(&(file->name), node, 255);
	file->name[255]='\0';
	char * intHolder = malloc(sizeof(char)*4);
	int i = 255;
	unsigned int valueHolder;
	while(i < 259) {
		*(intHolder + i - 255) = *(node + i);
		i++;
	}
	memcpy(&valueHolder, intHolder, 4);
	file->length = valueHolder;
	
	while(i < 263) {
		*(intHolder + i - 259) = *(node + i);
		i++;
	}
	memcpy(&valueHolder, intHolder, 4);
	file->direct1 = valueHolder;
	
	
	while(i < 267) {
		*(intHolder + i - 263) = *(node + i);
		i++;
	}
	memcpy(&valueHolder, intHolder, 4);
	file->direct2 = valueHolder;
	
	
	while(i < 271) {
		*(intHolder + i - 267) = *(node + i);
		i++;
	}
	memcpy(&valueHolder, intHolder, 4);
	file->direct3 = valueHolder;
	
	
	while(i < 275) {
		*(intHolder + i - 271) = *(node + i);
		i++;
	}
	memcpy(&valueHolder, intHolder, 4);
	file->indirect = valueHolder;
	while(i < 279) {
		*(intHolder + i - 275) = *(node + i);
		i++;
	}
	memcpy(&valueHolder, intHolder, 4);
	file->fromNode = valueHolder;
}
//-------------------------------------------------------------------------------------------

//will save the given File into the system
void SaveNode(File file) {
	char node[275]; //The set of bytes that will be saved to the disk
	int i = 0;
	//turns the FileInternals stuct into a 275 char array to write to disk
	//turns the FileInternals stuct into a 275 char array to write to disk
	while(i < 255 && file->name[i] != '\0') {
		*(node + i) = file->name[i];
		i++;
	}
	if(i < 255) {
		*(node + i) = '\0';
		i = 255;
	}
	char * intHolder = malloc(sizeof(char) * 4);
	int LTI = (int)file->length;
	memcpy(intHolder, &LTI, 4);
	while(i < 259) {
		*(node+i) = *(intHolder + i - 255); //only bytes position 4 to 8 are copied cause numbers never get that big
		i++;
	}
	LTI = (int)file->direct1;
	memcpy(intHolder, &LTI, 4);
	while(i < 263) {
		*(node + i) = *(intHolder + i - 259);
		i++;
	}
	LTI = (int)file->direct2;
	memcpy(intHolder, &LTI, 4);
	while(i < 267) {
		*(node + i) = *(intHolder + i - 263);
		i++;
	}
	LTI = (int)file->direct3;
	memcpy(intHolder, &LTI, 4);
	while(i < 271) {
		*(node + i) = *(intHolder + i - 267);
		i++;
	}
	LTI = (int)file->indirect;
	memcpy(intHolder, &LTI, 4);
	while(i < 275) {
		*(node + i) = *(intHolder + i - 271);
		i++;
	}
	
	
	//Next find the DataBlocks this Inode should be written to
	unsigned long **nodePos = malloc(sizeof(unsigned long *)*3); 
	findStartOfNode(file->fromNode, nodePos);
	//now get the blocks from the disk that will be overwritten
	char block[512];
	read_sd_block(block, **(nodePos));
	i = 0;
	unsigned long j = **(nodePos + 1);
	//Over write the Block
	while(j < 512 && i < 275) {
		block[j] = node[i];
		i++;
	}
	//save the block to disk
	write_sd_block(block, **(nodePos));
	//now take care of overflow into next node if needed
	if(i < 275) {
		read_sd_block(block, **(nodePos) + 1);
		j = 0;
		while(i < 275) {
			block[j] = node[i];
			j++;
			i++;
		}
		write_sd_block(block, **(nodePos) + 1);
	}
}
//-------------------------------------------------------------------------------------------

//Doing this one:
//write a function, given char string, compares to every named inode in system
//use grabNodeFromDisk method && findMaxNode
//grabNodeFromDisk returns string of 279 bytes
//no node shares name of given character array [char *]
//returns 0 if name not found & returns insigned long if name found
//unsigned long = node @ which it is found
//pass char = char of size 279

unsigned long findInodeByName(char * name) {
	unsigned long cPos = findNextNode(0);
	while(cPos < 20) {
		//printf("made it to FNBN1\n");//////////////////////////////////////////////////
		char *temp_node  = malloc(sizeof(char) * 279);
		grabNodeFromDisk(temp_node , cPos);
		*(temp_node + 255) = '\0';
		if (strcmp(name, temp_node)) {
			return cPos;
		}	
		cPos = findNextNode(cPos + 1);
	}
	return 0;		
}
//-------------------------------------------------------------------------------------------

//ONLY CALL IF file->cPos + 1 <= file->length
unsigned long readDirect(File file, char * buff, unsigned long direct, unsigned long posInBlock, unsigned long  * numBytesLeft) {
	unsigned long bytesRead = 0;
	char block[512];
	read_sd_block(block, direct);
	char * holder = malloc(sizeof(char)*512);
	long i = posInBlock;
	unsigned long posInHolder = 0;
	while(i < 512 && *numBytesLeft && file->Cpos + 1 <= file->length) {
		*(holder + posInHolder) = block[i];
		file->Cpos++;
		posInHolder++;
		i++;
		*numBytesLeft = *numBytesLeft - 1;
		bytesRead++;
		if(file->Cpos + 1 > file->length){
			file->Cpos--;
			break;
		}
	}
	memcpy(buff, holder,posInHolder+1);
	return bytesRead;
}
//-------------------------------------------------------------------------------------------

unsigned long getDirect(File file, unsigned long curDir) {
	if(curDir == 0) {
		return file->direct1;
	}
	if(curDir == 1)
		return file->direct2;
	if(curDir == 2)
		return file->direct3;
	if(curDir > 131) {
		return 5000;
	}
	char block[512];
	if(file->indirect == 0) {
		return 0;
	}
	read_sd_block(block, file->indirect);
	char * intholder = malloc(sizeof(char) * 4);
	for(int i = 0; i < 4; i++){
		*(intholder + i) = block[4 * (curDir - 3) + i];
	}
	int ITL;
	memcpy(&ITL, intholder, 4);
	unsigned long out = (unsigned long) ITL;
	return out;
}
//-------------------------------------------------------------------------------------------

void eraseBlock(unsigned long blockNum) {
	unsigned long ** zero = malloc(sizeof(unsigned long *) * 64);
	for(int i = 0; i < 512; i++) {
		*(zero + i) = malloc(sizeof(unsigned long));
		**(zero + i) = 0;
	}
	write_sd_block(*zero, blockNum);
}
//-------------------------------------------------------------------------------------------

void putDirInNode(File file, unsigned long dirNum, unsigned long blockNum){
	if(dirNum == 0)
		file->direct1 = blockNum;
	else if(dirNum == 1)
		file->direct2 = blockNum;
	else if(dirNum == 2)
		file->direct3 = blockNum;
	if(dirNum < 3)
		SaveNode(file);
	else{
		if(file->indirect == 0) {
			unsigned long idBlockNum = findFreeDB();
			if(idBlockNum == 5000) {
				///error no free blocks to wrote the 
				return;
			}
			eraseBlock(idBlockNum);
			file->indirect = idBlockNum;
		}
		char block[512];
		read_sd_block(block, file->indirect);
		char * intholder = malloc(sizeof(char) * 4);
		int LTI = blockNum;
		memcpy(intholder, &LTI, 4);
		for(int i = 0; i < 4; i++){
			block[4 * (dirNum - 3) + i] = *(intholder + i);
		}
		write_sd_block(block, file->indirect);
	}
}
//-------------------------------------------------------------------------------------------

// open existing file with pathname 'name' and access mode 'mode'.  Current file
// position is set at byte 0.  Returns NULL on error. Always sets 'fserror' global.
File open_file(char *name, FileMode mode){
	if(!FS_init) {
		init_FS();
	}
	char * realName = malloc(sizeof(char)*256);
	memcpy(realName, name, 255);
	*(realName + 255) = '\0';
	unsigned long InodeNum = findInodeByName(realName);
	if(!InodeNum) {
		//you got an error son
		//file by that name does not exist
		//deal with error
		return NULL;
	}
	if(OFBVisOpen(InodeNum)) {
		//got an error
		//file is already open
		return NULL;
	}
	else {
		OFBVopen(InodeNum);
		char * node = malloc(sizeof(char) * 279);
		grabNodeFromDisk(node, InodeNum);
		File newFile = malloc(sizeof(FileInternals));
		parseNode(node, newFile);
		newFile->mode = mode;
		return newFile;
	}
}
//-------------------------------------------------------------------------------------------

// create and open new file with pathname 'name' and access mode 'mode'.  Current file
// position is set at byte 0.  Returns NULL on error. Always sets 'fserror' global.
File create_file(char *name, FileMode mode) {
	if(!FS_init) {
		init_FS();
	}
	char * realName = malloc(sizeof(char) * 256);
	memcpy(realName, name, 255);
	*(realName + 255) = '\0';
	printf("made it to point 1 \n");
	if(findInodeByName(realName)) {
		//got an error, a file by that name exists already
		//handle it here
		return NULL;
	}
	printf("made it to point 2 \n");
	unsigned long InodeNum = findFreeInode();
	if(InodeNum == 20) {
		//got an error there are no free Inodes to create a file took
		//handle the error
		return NULL;
	}
	printf("made it to point 3 \n");
	File newFile = malloc(sizeof(FileInternals));
	newFile->mode = mode;
	newFile->Cpos = 0;
	newFile->direct1 = 0;
	newFile->direct2 = 0;
	newFile->direct3 = 0;
	newFile->indirect = 0;
	newFile->fromNode = InodeNum;
	newFile->length = 0;
	printf("made it to point 14\n");
	memcpy(newFile->name, realName, 256);
	assignINode(InodeNum);
	printf("made it to point 5 \n");
	SaveNode(newFile);
	printf("made it to point 6 \n");
	return newFile;
}
//-------------------------------------------------------------------------------------------

//close 'file'. Always sets 'fserror' global.
void close_file(File file) {
	if(file->fromNode <= 19) {
		OFBVclose(file->fromNode);
	}
	free(file);
}
//-------------------------------------------------------------------------------------------

//read at most 'numbytes' of data from 'file' into 'buf', starting at the 
//current file position. Returns the number of bytes read. If end of file is reached,
//then a return value less than 'numbytes' signals this condition. Always sets
//'fserror' global.
unsigned long read_file(File file, void *buf, unsigned long numbytes) {
	unsigned long bytesRead = 0;
	unsigned long posInOutBuff = 0;
	unsigned long curDir = (file->Cpos)/512;
	unsigned long posInBlock = (file->Cpos)%512;
	unsigned long bytesLeft = numbytes;
	while(bytesLeft > 0 && file->Cpos + 1 <= file->length) {
		unsigned long bytesReadfromDir = 0;
		unsigned long blockNum = getDirect(file, curDir);
		char * directReadbuff = malloc(sizeof(char)*512);
		if(blockNum == 0) {
			for(int i = 0; i < 512 -posInBlock && bytesLeft > 0 && file->Cpos+1 <= file->length ; i++){
				*(directReadbuff + i) = '\0';
				bytesReadfromDir++;
				bytesLeft--;
				file->Cpos++;
				if(file->Cpos + 1 > file->length) {
					file->Cpos--;
					break;
				}
			}				
		}
		else {
			bytesReadfromDir = readDirect(file, directReadbuff, blockNum, posInBlock, &bytesLeft);
		}
		memcpy((buf+posInOutBuff), directReadbuff, bytesReadfromDir);
		posInBlock = 0;
		bytesLeft--;
		bytesRead += bytesReadfromDir;
	}
	if(bytesRead < numbytes) {
		//means the read didn't happen right
		//check for any softwarediskerrors thrown 
		//then
		if(file->Cpos + 1 == file->length){
			//you hit the end of the file :((((
		}
	}
	return bytesRead;
}
//-------------------------------------------------------------------------------------------

//write 'numbytes' of data from 'buf' into 'file' at the current file position. 
//Returns the number of bytes written. On an out of space error, the return value may be
//less than 'numbytes'. Always sets 'fserror' global.
unsigned long write_file(File file, void *buf, unsigned long numbytes) {
	unsigned long bytesWritten = 0;
	if(file->mode != READ_WRITE) {
		//ERROR FILE MODE DOES NOT HAVE PERMISSION
		return bytesWritten;
	}
	unsigned long posInputbuf = 0;
	unsigned long bytesLeft = numbytes;
	unsigned long curDir = (file->Cpos) / 512;
	unsigned long posInBlock = (file->Cpos) % 512;
	if(!(file->Cpos < 67072)) {
		//error trying to write past the max file size
		return bytesWritten;
	}
	if(curDir > 131) {
		//error trying to get a dir that is over the max allocted block
		return bytesWritten;
	}
	else {
		while(bytesLeft > 0 && file->Cpos < 67072) {  //file->Cpos + 1 <= file->length
			unsigned long bytesWrittentoDir = 0;
			unsigned long blockNum = getDirect(file, curDir);
			if(blockNum == 5000) {
				//error hit end of file 
				return bytesWritten;
			}
			if(blockNum == 0) {
				blockNum = findFreeDB();
				if(blockNum == 5000) {
					//error there is no more blocks to write to
					return bytesWritten;
				}
				//remove if have time makes sute at least 2 blocks are open to make an indirect and block 
				if(curDir > 2) {
					unsigned long d1 = findFreeDB();
					assignDBBV(d1);
					unsigned long d2 = findFreeDB();
					freeDBBV(d1);
					if(d2 == 5000) {
						//error not enough DB to make indirect and a direct
						return bytesWritten;
					}
				}
				assignDBBV(blockNum);
				eraseBlock(blockNum);
				putDirInNode(file, curDir, blockNum);
				blockNum = getDirect(file, curDir);
			}
			char * block=malloc(sizeof(char) * 512);
			unsigned long bytesLefttoBlock = 512 - posInBlock;
			if(posInBlock != 0 || bytesLeft < 512) {
				read_sd_block(block, blockNum);
				unsigned long tempInputBufPos = posInputbuf;
				if(bytesLeft < 512 - posInBlock){
				memcpy((block + posInBlock), (buf + tempInputBufPos), bytesLeft);
				tempInputBufPos = bytesLeft;
				}
				else {
					memcpy((block + posInBlock), (buf + tempInputBufPos), 512 - posInBlock);
					tempInputBufPos = 512 - posInBlock;
				}
				write_sd_block(block, blockNum); //check
				bytesWrittentoDir = tempInputBufPos - posInputbuf;
			}
			else {
				memcpy(block, (buf + posInputbuf), 512);
				write_sd_block(block, blockNum); //check this to make sure it wrote i guess
				bytesWrittentoDir = 512;
			}
			posInputbuf = posInputbuf + bytesWrittentoDir;
			bytesWritten = bytesWritten + bytesWrittentoDir;
			bytesLeft = bytesLeft - bytesWrittentoDir;
			file->Cpos = file->Cpos + bytesWrittentoDir;
			if(file->Cpos + bytesWrittentoDir < 67072) {
				file->Cpos += bytesWrittentoDir;
				curDir++;
			}
			if(file->Cpos + 1 > file->length) {
				file->length = file->Cpos + 1;
			}
			posInBlock = 0;
		}
	}
	return bytesWritten;
}
//-------------------------------------------------------------------------------------------

//sets current position in file to 'bytepos', always relative to the
//beginning of file. Seeks past the current end of file should
//extend the file. Returns 1 on success and 0 on failure. Always
//sets 'fserror' global.
int seek_file(File file, unsigned long bytepos) {
	//file->Cpos = bytepos;
	if(bytepos > file->length){
		if(bytepos > (512 * 3 + 512 * (512 / 4)) - 1) {
			//error that is past the maxFilesize
			return 0;
		}
		file->length = bytepos + 1;
		SaveNode(file);
	}
	file->Cpos = bytepos;
	return 1;
}
//-------------------------------------------------------------------------------------------

//returns the current length of the file in bytes. Always sets 'fserror' global.
unsigned long file_length(File file) {
	return file->length;
}
//-------------------------------------------------------------------------------------------

//deletes the file named 'name', if it exists. Returns 1 on success, 0 on failure. 
//Always sets 'fserror' global.   
int delete_file(char *name) {
	if(!FS_init) {
		init_FS();
	}
	char * realName = malloc(sizeof(char) * 256);
	memcpy(realName, name, 255);
	*(realName + 255) = '\0';
	unsigned long InodeNum = findInodeByName(realName);
	if(!(InodeNum < 20)) {
		//error, file by that name does not exist
		return 0;
	}
	char * node = malloc(sizeof(char) * 279);
	grabNodeFromDisk(node, InodeNum);
	File file = malloc(sizeof(FileInternals));
	parseNode(node, file);
	unsigned long blockNum;
	if(!(file->indirect == 0)){
		char block[512];
		read_sd_block(block, file->indirect);
		for(int i = 0; i < 128; i++) {
			char * intHolder = malloc(sizeof(char)*4);
			for(int j = 0; j < 4; j++) {
				*(intHolder + j)=block[4 * i + j];
			}
			int ITL;
			memcpy(&ITL, intHolder, 4);
			if(ITL != 0){
				blockNum = (unsigned long)ITL;
				freeDBBV(blockNum);
			}
		}
		freeDBBV(file->indirect);
	}
	if(file->direct1 != 0) {
		freeDBBV(file->direct1);
	}
	if(file->direct2 != 0) {
		freeDBBV(file->direct2);
	}
	if(file->direct3 != 0) {
		freeDBBV(file->direct3);
	}
	freeInode(file->fromNode);
	free(file);
	return 1;
}	
//-------------------------------------------------------------------------------------------

//determines if a file with 'name' exists and returns 1 if it exists, otherwise 0.
//Always sets 'fserror' global.
int file_exists(char *name) {
	if(!FS_init) {
		init_FS();
	}
	char * realName = malloc(sizeof(char) * 256);
	memcpy(realName, name, 255);
	*(realName + 255) = '\0';
	unsigned long InodeNum = findInodeByName(realName);
	return (InodeNum < 20);
}
//-------------------------------------------------------------------------------------------

//describe current filesystem error code by printing a descriptive message to standard
//error.
//void fs_print_error(void);
// describe current software disk error code by printing a descriptive message to
// standard error.
void fs_print_error(void) {
	switch (fserror) {
		case FS_NONE:
			printf("FS: No error.\n");
			break;
		case FS_OUT_OF_SPACE:
			printf("FS: The operation caused the software disk to fill up\n");
			break;
		case FS_FILE_NOT_OPEN:
			printf("FS: Attempted read/write/close/etc. on file that isn't open.\n");
			break;
		case FS_FILE_OPEN:
			printf("FS: The file is already open. Concurrent opens are not supported and neither is deleting a file that is open.\n");
			break;
		case FS_FILE_NOT_FOUND:
			printf("FS: Attempted open or delete of file that does not exist.\n");
			break;
		case FS_FILE_READ_ONLY:
			printf("FS: Attempted write of file opened for READ_ONLY.\n");
			break;
		case FS_FILE_ALREADY_EXISTS:
			printf("FS: Attempted creation of file with existing name.\n");
			break;
		case FS_EXCEEDS_MAX_FILE_SIZE:
			printf("FS: Seek or write would exceed max file size.\n");
			break;
		case FS_ILLEGAL_FILENAME:
			printf("FS: Filename begins with a null character.\n");
			break;
		case FS_IO_ERROR:
			printf("FS: Something really bad happened.\n");
			break;
		default:
			printf("FS: Unknown error code %d.\n", fserror);
  }
}
//filesystem error code set (set by each filesystem function)
FSError fserror;

//disk has 5000 blocks use constants in final built
//each block is 512 bytpes use constant final build
//-------------------------------------------------------------------------------------------

//File create_file(char *name, FileMode mode)

int main(void) {
	//delete_file("file2");
	eraseBlock(1);
	eraseBlock(2);
	//
	File f2 = create_file("file2", READ_ONLY);
	File f3 = create_file("file3", READ_ONLY);
	File f4 = create_file("file4", READ_ONLY);
	File f5 = create_file("file5", READ_ONLY);
	File f6 = create_file("file6", READ_ONLY);
	File f7 = create_file("file7", READ_ONLY);
	File f8 = create_file("file8", READ_ONLY);
	File f9 = create_file("file9", READ_ONLY);
	File f0 = create_file("file0", READ_ONLY);
	File f1 = create_file("file1", READ_ONLY);
	write_file(f1, "please work plllllleeease work", 31);
	printf("the name of f1 is %s\n the length is %lu\n the d1 is %lu\nthe d2 is %lu\nthe d3 is %lu\n"
	"the id is %lu\n", f1->name, f1->length, f1->direct1, f1->direct2, f1->direct3, f1->indirect);
	char * ablock = malloc(sizeof(char)* 512*2);
	for(int i = 0; i < 512*2; i++){
		*(ablock+i) = 'A';
	}
	write_file(f1, ablock, 512*2);
	printf("the name of f1 is %s\n the length is %lu\n the d1 is %lu\nthe d2 is %lu\nthe d3 is %lu\n"
	"the id is %lu\n", f1->name, f1->length, f1->direct1, f1->direct2, f1->direct3, f1->indirect);
	if(file_exists)
		printf("f1 exits as it should");
	else
		printf("f1 does not exist as it shouldnt");
	return 0;
}

//only watch for functions that call read/write sd disk, each call, 
//for void functions, change voids to int and return 1 and/or 0
//all instances of write_sd_block has to return a 1 or 0
//if software disk operation fails, return 0 and end function
//if everything s gucci, return 1