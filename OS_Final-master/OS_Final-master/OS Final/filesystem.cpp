#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include "filesystem.h"
#include <time.h>
#include <cstdlib>
#include <iostream>
using namespace std;


FileSystem::FileSystem(DiskManager *dm, char fileSystemName)
{
	//The assumption here is that a filesystem takes up ALL of a partition.
	
	myDM = dm;
	int psize = dm -> getPartitionSize(fileSystemName);
	myPM = new PartitionManager(dm, fileSystemName, psize);
	myfileSystemName = fileSystemName;
	myfileSystemSize = psize;
	lockID = 0;
	fileDescriptor = 0;
	// char dirInode[64];
	// dirInode[0];
}


//return blocknum for creating a file called file or -1 if the file doesn't exist.
//traverses directories listed in filename of fnamelen characters
//fnamelen is always a multiple of 2. we can't find the super-directory of the root.
//if type is 'f', the block number returned points to a file inode
//if type is 'd', the block number returned points to a block full of directory inodes.

//upgrade the getDir to getInode, add a char type parameter, 
//and then 4 length is weird, what about /a that you create a directory in root
int FileSystem::getDir(char *filename, int fnamelen, char file, int block)
{	
char dirList[64];
	myPM -> readDiskBlock(block, dirList);
	if(fnamelen = 2) //we're in the file's directory
	{
		int i = 0;
		while(i<60)
		{
			if(dirList[i] == filename[1])
			{
				/*the type variable is passed in by reference and tells us whether
				  the block returned points to a file inode or a block  of directory 
				  inodes.*/
				if(dirList[i+2] == 'f')
				{
					fileDInodeIndex = i;
					//remember the index of the dInode (for deleteFile)
					type = 'f';
				}
				else if(dirList[i+2] == 'd')
				{
					type = 'd';
				}
				return dirList[i+1]; //block address for writing inode
			}
			i+=6;
		}
		if(i == 60) //we didn't find the file in this block
		{
			if(dirList[60] != 'c') //if there's another block to look in
				return getInode(filename, fnamelen, file, fileDInodeIndex, (int) dirList[60], type); //we need to go to the next block of directory inodes and keep searching
			else
				return -1; //we didn't find the filename because: 1. the dirList is at the end and 2. we didn't get another block to look in.
		}
	}
	else if(fnamelen % 2 != 0)
		return -2; //there's a problem with the input
	int j = 0;
	while(j < 60) //if the fnamelen is too long, we still need to find our way around the directories
	{
		if(dirList[j] == filename[1]) //if we find the directory we want in the dirlist...
		{
			char fn[fnamelen-2]; //we want to shorten the file name length, since we're in a different directory now.
			int k = 0;
			for(int j = 2; j < fnamelen; j++)
			{
				fn[k] = filename[j]; //fn is the new filename, our goal is to get it to 4 (e.g. /a/b)
				k++;
			}
			char *shortFn = fn; //format it to be passed back into the function
			return getInode(shortFn, fnamelen-2, file, fileDInodeIndex, (int) dirList[j+1], type); //recursive call with the shorter fnamelen and the new block that we want.
		}
		j+=6;
	}
	if(j == 60) //same as above with j instead of i
	{
		if(dirList[60] != 'c')
				return getInode(filename, fnamelen, file, fileDInodeIndex, (int) dirList[60], type);
			else
				return -1;
	}

//this is code for finding a specific file inode by traversing directories (not what we needed).
	// char dirList[64];
	// myPM -> readDiskBlock(block, dirList);
	// if(fnamelen == 2)
	// {
		// int i = 0;
		// while(i<60)
		// {
			// if(dirList[i] == file)
			// {
				// return -1;
			// }
			// else if(dirList[i] == 'c')
				// break;
			// i+=6;
		// }
		// if(i == 60)
			// return getDir(filename, fnamelen, file, (int) dirList[60]);
		// else
			// return dirList[i-5];
	// }
	// for(int j = 0; j < 60; j+=6)
	// {
		// if(dirList[j] == filename[1])
		// {
			// char fn[fnamelen-2];
			// int k = 0;
			// for(int j = 2; j < fnamelen; j++)
			// {
				// fn[k] = filename[j];
				// k++;
			// }
			// char *shortFn = fn;
			// return getDir(shortFn, fnamelen-2, file, (int) dirList[j+1]);
		// }
	// }
	// return -2;
}

/*
FindSpot

Returns: a spot in a block that we can write a directory inode to

Updates actualBlk variable as more blocks are taken by a single
directory
*/
int FileSystem::findSpot(int dirBlk, int &actualBlk)
{
	char buffer[64];
	myPM -> readDiskBlock(dirBlk, buffer);
	for(int i = 0; i < 60; i+=6)
	{
		if(buffer[i] == 'c')
		{
			return i; //return position in the block to write to
		}
	}
	if(buffer[60] != 'c') //if the end of the block points somewhere...
	{
		//update actualBlk...
		actualBlk = (int) buffer[60];
		//return the same function, just different block
		return findSpot(actualBlk, actualBlk);
	}
	else
	{
		//get another disk block for addressing
		actualBlk = myPM -> getFreeDiskBlock();
		//tell the full block to go to the new one at the end
		buffer[60] = (char) actualBlk;
		myPM -> writeDiskBlock(dirBlk, buffer);
		//then return 0, because we can write at the beginning of the new block.
		return 0;
		/*REMEMBER TO USE ACTUALBLK VARIABLE PASSED IN, NOT 
		  THE BLOCKNUM PASSED IN!!! They might be the same, but actualblk
		  is what you definitely want to use. */
		
	}
}

/*
// Create File
// 
// creates a new file, the name is pointed to by filename of size
// fname_len characters.
//
// Returns: -1 if file exists, -2 if there isnt enough disk space,
// 			-3 if invalid filename, and -4 if file cannot be created 
//			0 if the file is created successfully
*/
int FileSystem::createFile(char *filename, int fnameLen)
{
	char createBuffer[64];
	
	char *last;
	for(int i = 0; i < fnameLen; i+=2)
	{
		char* slash = &filename[i];
		char* name = &filename[i+1];
		if(*slash != '/')
			return -3;
		if(!isalpha(*name))
			return -3;
		last = name;
	}
	
	//traverse the dInodes to see if it is there in memory
	//If it exists, do nothing
	//If it doesnt exist, then we need to create it
	//Create the file by connecting it to the chain of file directories
	//		then find free space to put the file inode to start the file
	//		report to the partition table somehow and tell it the blocks that 
	//			are being used to create the file
	
	getdir(char filename, int fileName)
	{
		
	}
	
	//actually create the file
	int blk = myPM -> getFreeDiskBlock();
	if(blk != -1)
	{
		char *inode;
		inode[0] = *last; //name of the file stored here 
		inode[1] = 'f'; //type is file (duh)
		inode[2] = '0'; //size is zero (next 4 bytes)
		inode[3] = '0';
		inode[4] = '0';
		inode[5] = '0';
		for(int i = 6; i < 22; i++) //all of the rest is addressing
		{
			inode[i] = 'c';
		}
		int w = myPM -> writeDiskBlock(blk, inode);
		if(w == 0)
		{
			nameType[*last] = 'f';
			return 0;
		}
		else
			return -4;
	}
	else if(blk == -1)
		return -2;
	else
		return -4;
}

/*
Create Directory

creates a new directory whose name is pointed to by dirname

Returns: -1 if directory exists, -2 if isnt enough disk space, 
		 -3 if invalid directory name, -4 if it cannot be created
		 0 if created successfully

*/
int FileSystem::createDirectory(char *dirname, int dnameLen)
{
	// char *last;
	// for(int i = 0; i < fnameLen; i+=2)
	// {
		// char* slash = &filename[i];
		// char* name = &filename[i+1];
		// if(*slash == '/')
		// {
			// if(!isalpha(*name))
				// return -3;
			// last = name;
		// }
	// }
	
	// int blk = myPM -> getFreeDiskBlock();
	// if(blk != -1)
	// {
		// char *inode;
		// inode[0] = *last; //name of the file stored here 
		// inode[1] = 'd'; //type is file (duh)
		// inode[2] = '0'; //size is zero (next 4 bytes)
		// inode[3] = '0';
		// inode[4] = '0';
		// inode[5] = '0';
		// for(int i = 6; i < 22; i++) //all of the rest is addressing
		// {
			// inode[i] = 'a';
		// }
		// int w = myPM -> writeDiskBlock(blk, inode);
		// if(w == 0)
			// return 0;
		// else
			// return -4;
	// }
	// else if(blk == -1)
		// return -2;
	// else
		// return -4;
}

/*
LockFile

locks a file

A file cannot be locked when:
	it doesnt exist,
	it is already locked,
	it is curretnly opened
	
Returns: -1 if file already locked, -2 if it doesnt exist, 
		-3 if currently opened, -4 file cannot be locked
		
Note: once a file is locked, it can only be opened with the lock id
		it cannot be deleted or renamed until unlocked
*/
int FileSystem::lockFile(char *filename, int fnameLen)
{
	char *last;
	for(int i = 0; i < fnameLen; i+=2)
	{
		char* slash = &filename[i];
		char* name = &filename[i+1];
		if(*slash != '/')
			return -2;
		if(!isalpha(*name))
			return -2;
		last = name;
	}
	
	if(nameLock.find(*last) != nameLock.end())
		return -1;
	if(nameType.find(*last) == nameType.end())
		return -2;
	//if(openFileTable.find(*last) != openFileTable.end())
		//return -3;
	//else //cannot be locked
		//return -4;
	lockID++;
	nameLock[*last] = lockID;
	return lockID;
}

/*
UnlockFile

unlocks a file

lockId is the lock returned by LockFile after file is locked

Returns: 0 if successful, -1 if invalid lock id, -2 other 
*/
int FileSystem::unlockFile(char *filename, int fnameLen, int lockId)
{
	char *last;
	for(int i = 0; i < fnameLen; i+=2)
	{
		char* slash = &filename[i];
		char* name = &filename[i+1];
		if(*slash != '/')
			return -2;
		if(!isalpha(*name))
			return -2;
		last = name;
	}
	
	if(nameLock.find(*last) != nameLock.end())
	{
		if(nameLock[*last] == lockId)
		{
			std::map<char, int>::iterator del = nameLock.find(*last);
			nameLock.erase(del);
			return 0;
		}
		return -1;
	}
	return -2;
}



/*
DeleteFile

deletes the file whose name is pointed to by filename

Note: a open or locked file cannot be deleted 

	/*Delete a file by doing:
	//	check if file is open or locked, if so, error
	//  check if file exists, if not, error
	
	//  replace all direct address blocks with 'c'same
	//  Update BitVector (Dealocate memory) for all 
	// 		direct address blocks 
	//  replace all blocks connected to indirect with 'c's
	//  Update BitVector (Dealocate memory) for all blocks 
	//		connected to the indirect address
	//	replace the file Inode block with 'c's
	//  Update BitVector (Dealocate memory) for the file Inode
	//  replace the file dInode with 'c's
	
	//	Update tables (maps) by deleting existance of file/fileDesc
	
Returns: 0 if successfully deleted, -1 if file doesnt exist,
		-2 if file is open or locked, -3 if cannot be deleted or other
*/
int FileSystem::deleteFile(char *filename, int fnameLen)
{
	//_Variables_//
	int blockNum;// Address (Block number) to the beginning of the file Inode 
	char last;// Variable used to store the actual filename
	char clearBuffer[64];//Buffer used to 'delete' (replace with 'c's) blocks of 64 bytes
	char clearDInodeBuffer[6];//Buffer used to 'delete' (replace with 'c's) blocks of 6 bytes (file dInode)
	
	//Dummy Variables for the getInode function //
	//	(doesnt matter what they are set as) These variables will be dereferenced, so after the call to getInode, their values will be correct
	int filedInodeIndex = 77; //The index at which the file dInode (Im looking for specifically) sits within the directory block
	char type = 'dont care'; //The type of the dInode, 'f/d' (file or directory)
	
	
	//_Setup Variables_//
	
	//Fill both char buffers with 'c's
	fillClearBuffers(clearBuffer, clearDInodeBuffer);
	
	//Find the actual name of the file, and put it in 'last'
	last = findActualFilename(filename, fnameLen);
	
	//Find the block address for the file Inode
	//Param: (char *filename, int filenamelenght, char file(*last), int &rememberBlock(file dInode), int block(), char type(d/f))
	blockNum = getInode(filename, fnameLen, last, fileDInodeIndex, block, type);
	
	//Find the four direct/indirect addresses
	int direct1 = blockNum + 5;//1 + 1 + 4 -1;
	int direct2 = blockNum + 9; //1 + 1 + 4 + 4 -1;
	int direct3 = blockNum + 13; //1 + 1 + 4 + 4 + 4 -1;
	int indirect = blockNum + 17; //1 + 1 + 4 + 4 + 4 + 4 -1;
	
	
	//_Precondition Check for Errors_//
	
	//Check if file exists
	if(blockNum == -1)
	{
		return -1;
	}
	
	//Check if file is locked
	if(nameLock.find(*last) != nameLock.end())
	{
		return -2;
	}
	
	//Check if file is open
	char fileDesc = findFileDescriptor(char last);	//Find the file descriptor
	if(fdMode.find(filedesc) != fdMode.end()) //Check if it is open
	{
		return -2;
	}
	
	/*- File inode implementation
			name 1 byte
			type 1 byte: file/directory
			filesize 4 bytes:
			3 direct address at 4bytes each = 12 bytes
			1 indirect address (pointers to indirect inode) 4 bytes
			rest of the space for your attributes.
	*/
	
	
	//_Setup Copy Buffers_//	
	
	//Copy File Inode into a buffer
	char fInodeBuffer[64];// buffer used to store the file Inode
	myPM -> readDiskBlock(blocknum, fInodeBuffer);//copy the file Indoe block into the buffer
 
 	indirectAddress = fInodeBuffer[indrect];//address stored at indirect index of file inode
 
	//Copy Indirect Address Block into a buffer
	char indirectInodeBuffer[64];// buffer used to store the whole block that indirect address is pointing to
	myPM -> readDiskBlock(indirectAddress, indirectInodeBuffer);//copy the block into the buffer
	
	//Copy File dInode into a buffer
	char fileDInodeBuffer[6];// buffer used to store the whole block pointed to by the last directory
	myPM -> readDiskBlock(fileDInodeIndex, fileDInodeBuffer);//copy the file dInode into the buffer
	
	
	//_Calculate File Size (in # of Blocks used)_//
	
	int filesize = fInodeBuffer[2];
	//Calculate size of file in 'blocks' (1 block == 64 bytes), so I can determine how far the file extends
	int numOfFileBlocks = calcFileSizeInNumOfBlocks(filesize); //Calls a function to calculate the size of a function in Blocks
	
	//_Delete the File Itself_//
	
	//Loop through all of the links from the file Inode,
	//	Replace the data with 'c's 
	//  Update the bitVector (Dealocate memory)
	for(int i = 0; i < numOfFileBlocks; i++)
	{
		if(i < 3)//Delete Direct Blocks
		{
			switch(i)
			{
				case 0:
				
				myPM -> writeDiskBlock(direct1, clearBuffer);//write over the current direct block with a clearBuffer full of 'c'
				myPM -> returnDiskBlock(direct1);//deallocate direct1 address from bitVector
				break;
				
				case 1:
				myPM -> writeDiskBlock(direct2, clearBuffer);//write over the current direct block with a clearBuffer full of 'c'
				myPM -> returnDiskBlock(direct2);//deallocate direct2 address from bitVector
				break;
				
				case 2:
				myPM -> writeDiskBlock(direct3, clearBuffer);//write over the current direct block with a clearBuffer full of 'c'
				myPM -> returnDiskBlock(direct3);//deallocate direct3 address from bitVector
				break;
			}
			
		}
		else//Delete Indirect Blocks
		{
			myPM -> writeDiskBlock(indirectAddress, clearBuffer);//write over the current indirect block with a clearBuffer full of 'c'
			myPM -> returnDiskBlock(indirectAddress);//deallocate all indirect address blocks as they are looped through
			indirectAddress += 4;
		}
	}
	
	
	//_Delete the File Inode_//
	
	myPM -> writeDiskBlock(blockNum, clearBuffer);//write over the file inode with a clearBuffer full of 'c'
	myPM -> returnDiskBlock(blockNum);//deallocate file Inode by updating the bitVector
	
	
	//_'Delete' the File dInode_//
	
	myPM -> writeDiskBlock(fileDInodeIndex, clearDInodeBuffer);// Replace the file dInode with 'c's, dont need to update bitVector
	
	
	//_Update Tables (Maps): fdTable, fdName, nameType, fdMode_//
	
	//Update nameType
	int temp = updateCharCharMaps(nameType, last);
	if(temp == -3)
	{
		return -3;
	}
	
	//Update fdMode
	int temp = updateCharCharMaps(fdMode, fileDesc);
	if(temp == -3)
	{
		return -3;
	}
	
	//Update fdTable 
	if(fdTable.find(fileDesc) != fdTable.end())
	{
		if(fdTable[fileDesc] == fileDesc)
		{
			std::map<char, int>::iterator del = fdTable.find(fileDesc);
			fdTable.erase(del);
		}
		else
		{
			return -3;
		}
	}
	
	//Update fdName 
	if(fdName.find(fileDesc) != fdName.end())
	{
		if(fdName[fileDesc] == fileDesc)
		{
			std::map<char, int>::iterator del = fdName.find(fileDesc);
			fdName.erase(del);
		}
		else
		{
			return -3;
		}
	}
		
	return 0;//successfully deleted
}

/*
FillClearBuffers

Param: clearBuffer[64], clearBuffer[6]

Fills both character buffers with 'c's
*/
void fillClearBuffers(char clearBuffer, char clearDInodeBuffer)// Helper for deleteFile
{
	for(int i = 0; i < 64; i++)
	{
		clearBuffer[i] = 'c';
		if(i < 6)
		{
			clearDInodeBuffer[i] = 'c';
		}
	}
}

/*
FindActualFilename

Param: filename array, length of the filename

Loops through the filename array to the end and returns the 
actual filename (not /a/b/c/d, but instead returns d)
*/
char findActualFilename(char filename, int fnameLen)// Helper for deleteFile
{
	//Find the actual filename
	for(int i = 0; i < fnameLen; i+=2)
	{
		char slash = filename[i];
		char name = filename[i+1];
		if(slash != '/')
			return -2;
		if(!isalpha(name))
			return -2;
		last = name;
	}
	return last;
}

/*
	CalcFileSizeInNumOfBlocks
	
	Param: size of the file
	
	Calculates the size of a file in number of blocks (1 block == 64 bytes)
	
	Returns: Number of blocks
	*/
	int calcFileSizeInNumOfBlocks(int filesize)// Helper for deleteFile
	{
		//I'm using integers, not floats, so I will determine if the filesize is odd,
		//	if it is, i will subtract one from filesize so I can divide evenly, then 
		//	add one after computation
		
		if (filesize % 2 != 0)//if filesize is odd
		{
			int evenFilesize = filesize - 1;
			numOfFileBlocks = calculateNumOfInodeBlocks(evenFilesize);
			numOfFileBlocks += 1;
		}
		else
		{
			numOfFileBlocks = calculateNumOfInodeBlocks(filesize);
		}
		
		return numOfFileBlocks;
	}

	/*
	FindFileDescriptor
	
	Param: the actual name of the file
	
	Uses the name of the file to search through fdName to find
	the File Descriptor
	*/
	char findFileDescriptor(char last)// Helper for deleteFile
	{
		if(fdName.find(last) != fdName.end())
		{
			fileDesc = fdName[last];
		}
		
		return fileDesc;
	}
	
	/*
	UpdateCharCharMaps
	
	Param: char char map, variable used to search for in map,

	Update basically the fdMode, and nameType Maps
	*/
	int updateCharCharMaps(map<char, char> map, char findWith)// Helper for deleteFile
	{ 
		if(map.find(findWith) != map.end())
		{
			if(map[findWith] == fileDesc)
			{	
				std::map<char, int>::iterator del = map.find(findWith);
				map.erase(del);
				return 0;
			}
			else
			{
				return -3;
			}
		}
	
	}

	
	
/*
DeleteDirectory 

deletes the directory whose name is pointed to by dirname

Note: only empty directories can be deleted
		used findDirectoryAddress to look through all possible extensions
		of the block the previous dInode points to

Returns: 0 if successfully, -1 if doesnt exist, 
		-2 if not empty, -3 cannot be deleted or other
		
*/
int FileSystem::deleteDirectory(char *dirname, int dnameLen)
{
	//_Variables_//
	int dblockNum;// Address (Block number) to the beginning of the dInode 
	int blockNum;// Address (Block number) to the beginning of the dInode's block 
	char last;// Variable used to store the actual filename
	char prevDirName; // Variable used to store the name of the previous directory name 
	char clearBuffer[64];//Buffer used to 'delete' (replace with 'c's) blocks of 64 bytes
	char clearDInodeBuffer[6];//Buffer used to 'delete' (replace with 'c's) blocks of 6 bytes (file dInode)
	
	
	//Dummy Variables for the getInode function //
	//	(doesnt matter what they are set as) These variables will be dereferenced, so after the call to getInode, their values will be correct
	int filedInodeIndex = 77; //The index at which the file dInode (Im looking for specifically) sits within the directory block
	char type = 'dont care'; //The type of the dInode, 'f/d' (file or directory)
	
	
	//_Setup Variable_//
	
	//Fill both char buffers with 'c's
	fillClearBuffers(clearBuffer, clearDInodeBuffer);
	
	//Find the actual name of the file, and put it in 'last'
	last = findActualFilename(filename, fnameLen);
	
	//Find the actual name of the previous directory, and put it in 'prevDirName'
	prevDirName = findActualDirectoryFilename(filename, fnameLen);
	
	//Find the block address for the block dInode sits in 
	//Param: (char *filename, int filenamelenght, char file(*last), int &rememberBlock(file dInode), int block(), char type(d/f))
	dblockNum = getInode(filename, fnameLen, prevDirName, fileDInodeIndex, block, type);

	
	//_Precondition Check for Errors_//
	
	//Check if file exists
	blockNum = getInode(filename, fnameLen, last, fileDInodeIndex, block, type);
	if(blockNum == -1)
	{
		return -1;
	}
	
	//Check if empty
	char dInodeBlockBuffer[64];// buffer used to store the dInode's Block 
	myPM -> readDiskBlock(blocknum, dInodeBlockBuffer);//copy the file Indoe block into the buffer
	
	for(int i = 0; i < 64; i++)// Check if buffer is full of 'c's
	{
		if(buffer[i] != 'c')
		{
			return -2;
		}
	}
	

	//_Copy previous dInode's Block in a Copy Buffer_//	
	
	char prev_dInodeBlockBuffer[64];// buffer used to store the Block that dInode sits in
	myPM -> readDiskBlock(dblocknum, prev_dInodeBlockBuffer);//copy the previous dInode's block into the buffer

	
	//_Find dInode address in Block_//
	
	int actualBlock = dblocknum;//Current block Im searching through, it only changes if 64 bytes is too small for the amount of dInodes stored 
	int dInodeIndex = findDirectoryAddress(dblocknum, actualBlock, last);//Find the address of the dInode
	
	
	//_Dealocate dInode's Block_//
	
	dInodeBlockIndex = dInodeIndex + 1;	// find address dInode points to 
	myPM -> writeDiskBlock(dInodeBlockIndex, clearBuffer);//write over the dInode's Block with a clearBuffer full of 'c'
	myPM -> returnDiskBlock(dInodeBlockIndex);//deallocate the block that dInode points to from bitVector
	
	
	//_'Delete' the dInode_//
	
	/* (I dont think I need this)
	//Copy dInode into a buffer
	char dInodeBuffer[6];// buffer used to store the whole block pointed to by the last directory
	myPM -> readDiskBlock(dInodeIndex, dInodeBuffer);//copy the file dInode into the buffer
	*/
	
	myPM -> writeDiskBlock(dInodeIndex, clearDInodeBuffer);//write over the current direct block with a clearBuffer full of 'c'
	
	return 0;//deleted successfully
}

	/*
	FindActualDirectoryFilename

	Param: filename array, length of the filename

	Loops through the filename array to the end and returns the 
	actual filename for the directory (not /a/b/c/d, but instead returns c)
	*/
	char findActualDirectoryFilename(char filename, int fnameLen)// Helper for deleteDirectory
	{
		//Find the actual directory filename
		for(int i = 0; i < (fnameLen - 2) ; i+=2)
		{
			char slash = filename[i];
			char name = filename[i+1];
			if(slash != '/')
				return -2;
			if(!isalpha(name))
				return -2;
			prevDirName = name;
		}
		return prevDirName;
	}

	/*
	FindDirectoryAddress
	
	Param: directory block you want to look through, 
			actual block it will be written to (because the block 
			might 'extend' past 64 bytes)
	
	Returns: Returns the beginning address of the dInode passed in
	*/
	inf FileSystem::findDirectoryAddress(int dInodeBlk, int &actualBlk, char last)// Helper for deleteDirectory
	{
		char buffer[64];
		myPM -> readDiskBlock(dInodeBlk, buffer);
		for(int i = 0; i < 60; i+=6)
		{
			if(buffer[i] == last)
			{
				return i; //return position in the block the dInode starts
			}
		}
		if(buffer[60] != last) //if the end of the block points somewhere...
		{
			//update actualBlk...
			actualBlk = (int) buffer[60];
			//return the same function, just different block
			return findSpot(actualBlk, actualBlk);
		}
		else
		{
			//get another disk block for addressing
			actualBlk = myPM -> getFreeDiskBlock();
			//tell the full block to go to the new one at the end
			buffer[60] = (char) actualBlk;
			myPM -> writeDiskBlock(dInodeBlk, buffer);
			//then return 0, because we can write at the beginning of the new block.
			return 0;
			/*REMEMBER TO USE ACTUALBLK VARIABLE PASSED IN, NOT 
			THE BLOCKNUM PASSED IN!!! They might be the same, but actualblk
			is what you definitely want to use. */
		}
	}



/*
OpenFile

opens a filw whose name is pointed to by filename

Modes: 'r' is for read only, 'w' is for write only, 
		'm' is for read and write
		
Existing files cannot be opened if:
	file is locked, and lockId doesnt match the lockId 
	returned by lockFile when file was locked
	
	the file is not locked and lockId != -1
	
Returns: -1 if does not exist, -2 if invalid, 
		-3 cannot open due to locking restrictions, -4 if other
		or a unique +int (file descriptor) if successful 
		
if open was successful, a rw pointer is associated with this file descriptor
note: rw pointer is used by other operations for determining the 
	access point in a file, the initial value is 0 (beginning of the file)	
*/
int FileSystem::openFile(char *filename, int fnameLen, char mode, int lockId)
{
	char *last;
	for(int i = 0; i < fnameLen; i+=2)
	{
		char* slash = &filename[i];
		char* name = &filename[i+1];
		if(*slash != '/')
			return -1;
		if(!isalpha(*name))
			return -1;
		last = name;
	}
	if(nameType.find(*last) == nameType.end())
		return -1;
	if(mode != 'r' && mode != 'w' && mode != 'm')
		return -2;
	if(nameLock.find(*last) != nameLock.end())
	{
		if(nameLock[*last] == lockId)
		{
			openFileTable[*last] = mode;
			fileDescriptor++;
			int* rw = 0;
			fdTable[fileDescriptor] = rw;
			fdName[fileDescriptor] = last;
			return fileDescriptor;
		}
	}
	else if(lockId == -1)
	{
		openFileTable[*last] = mode;
		fileDescriptor++;
		int* rw = 0;
		fdTable[fileDescriptor] = rw;
		fdName[fileDescriptor] = last;
		return fileDescriptor;
	}
	else if(nameLock.find(*last) == nameLock.end())
		return -3;
	return -4;
}

/*
CloseFile

closes the file with file descriptor filedesc

Returns: 0 if successful, -1 if invalid, -2 if other reasons
*/
int FileSystem::closeFile(int fileDesc)
{
	if(fdTable.find(fileDesc) == fdTable.end())
		return -1;
	else if(fdTable.find(fileDesc) != fdTable.end())
	{
		std::map<int, int*>::iterator it1 = fdTable.find(fileDesc);
		std::map<int, char*>::iterator it2 = fdName.find(fileDesc);
		char *name = fdName[fileDesc];
		std::map<char, char>::iterator it3 = openFileTable.find(*name);
		fdTable.erase(it1);
		fdName.erase(it2);
		if(openFileTable.find(*name) != openFileTable.end())
		{
			openFileTable.erase(it3);
		}
		return 0;
	}
	else
		return -2;
}

/*
ReadFile

read operation on a file, file descriptor is fileDesc, length is 
the number of bytes to be read from the buffer pointed to by data

operate from the byte pointed to by the rw pointer
can read less bytes than the length if end of file is reached earlier

Return: -1 if fileDesc invalid (DNE), -2 negative length, 
		-3 operation not permitted,
		or number of bytes read if successful
		
		rw is updated to point to the byte follwoing the last byte read

*/
int FileSystem::readFile(int fileDesc, char *data, int len)
{
	char* name;
	int numOfBytesRead = 0;
	int rw;
	char mode;// read or write
	int sizeOfFile;//size of file I found in inode
	int numTilEOF;//counter to keep from reading off the end of the file
	char readBuffer[64];//buffer to copy the file into for reading
	
	//If operation is not permitted: 
	//		if file descriptor valid, is the mode correct
	
	//Check if invalid/DNE,
	if(fdTable.find(fileDesc) != fdTable.end())// if fileDesc is in fdTable, then grab the rw pointer
	{
		rw = fdTable[fileDesc];
	}
	else 
	{
		return -1;
	}
	//check the mode with new table that has file descriptor to mode 
	if(fdToMode.find(fileDesc) != fdToMode.end())
	{
		mode = fdToMode[fileDesc];
		if(mode == "w")
		{
			return -3;
		}
	}
	else
	{
		return -3;
	}
	
	//Check if length is negative
	if(len < 0)
	{
		return -2;
	}
		
	//At this point, we read 'len' amount of the file from the place rw points
	//to and read len amount of bytes, then return the amount of bytes read
	//as well as resetting the rw pointer to the last byte read
	
	/*
	//find inode
	//read the address and data into a buffer
	//copy the data into *data 
	//update the rw pointer
	*/
	
	//find inode


	numTilEOF = sizeOfFile - rw;
	
	for( int i = 0; i < len; i++)
	{
		if(numTilEOF > 0)//makes sure I dont fall off the EOF when I read 
		{
		//check if end of file
		//if()//inode has attribue of file size, maybe use this to check for EOF
		{
			//read another byte: add byte to *data, increment rw pointer
			
/*			//....how to get the data rw is pointing to and cons it to *data.*/
			*rw++;//increment position of rw pointer
			
			//store the info in a buffer of size 64 so we dont segfault
			
		}
		else{
			
		}		}
		
		numOfBytesRead++;
	}
	
	//update rw pointer
	rw += numOfBytesRead;
  return numOfBytesRead;
}














/*
WriteFile

write operation on a file, file descriptor is fileDesc, length is 
	the number of bytes to be written to the buffer pointed to by data

operate from the byte pointed to by the rw pointer
overwrites the existing data in the file and can increase 
	the size of file

Return: -1 if invalid, -2 negative length, -3 operation not permitted,
		or number of bytes written if successful
		
		rw is updated to point to the byte follwoing the last byte written
		
*/
int FileSystem::writeFile(int fileDesc, char *data, int len)
{
	//_Variables_//
	int numOfBytesWritten = 0;// Keeps track the number of successful bytes written
	int blockNum;// Address (Block number) to the beginning of the file inode 	
	char fname;// Variable used to store the filename
	char fullfname; //Store the name and directories to file (exp. /a/b/c/d)
	char fullfnameLength; //Length of the full name of the file (exp. /a/b/c/d, length is 8)
	
	//_Dummy Variables for the getInode function_//
	//	(doesnt matter what they are set as) These variables will be dereferenced, so after the call to getInode, their values will be correct
	int filedInodeIndex = 77; //The index at which the file dInode (Im looking for specifically) sits within the directory block
	char type = 'dont care'; //The type of the dInode, 'f/d' (file or directory)
	
	
	//_Setup Variable_//
	
	//Find the name of the file 
	fname = findFileNameUsingDesc(fileDesc);
	
	//Find the full filename (so I can find the file Inode)
	fullfname = findFullFileNameUsingDesc(fileDesc);

	//Find the length of the full filename
	fullfnameLength = findFullFilenameLength(fullfname);
	
	//Find the block address for the file Inode
	blockNum = getInode(fullfname, fullfnameLength, fname, fileDInodeIndex, block, type);


	//_Precondition Check for Errors_//
	
	//Check if invalid: File DNE (Does not exist)
	if(fname == '1')// check if name DNE
	{
		return -1;
	}
	
	if(fullfname == '1')//Check if full filename DNE
	{
		return -1;
	}
	
	if(fullfnameLength == 9000)//Check if full name is valid 
	{
		return -1;
	}
	
	//Check if invalid: file is not open
	if(fdMode.find(fileDesc) == fdMode.end())
	{
		return -1;
	}
	else
	{
		mode = fdMode[fileDesc];
	}
	
	//Check if len is negative 
	if(len < 0)
	{
		return -2;
	}
	
	//Check if operation is not permitted: Check the mode
	if(mode != 'w' || mode != 'm')
	{
		return -3;
	}
	
	//_Find File Inode_//
	
	
	
	
	
	
	
	/*

	//_find the file_//
	//use fileDesc to locate the file name in maps
	//find where the inode sits by using name as a param to findInode()
	//copy the inode to a buffer
	//loop through the buffer to find the adress of where the file sits
	//now I should be at the beginning of the file
	
	//use the inode buffer to find the filesize and save it
	
	//_find rw pointer_//
	//find rw pointer by using fileDesc in maps
	
	//_Calculate if file expansion is necessary_//
	//using the value of the rw pointer, calculate if the file needs to be extended
	//	if rw pointer + length > filesize
	
	//_Extend a file (only if necessary)_//
	//to extend the file: (means it needs to be bigger than the first direct address block can hold)
	
	if second and third direct adress blocks are 0
	//move to second/third direct address block:
	need to find the first free disk block, 
	grab the address to the free block and put it into the second/third direct address slot
	
	//if directs are taken, now indirect
	
	//copy indirect's block of addresses into a buffer
	//look through until you find a 0, this means free
	//then rember that index
	//find the next free disk block and 
	// copy that adress into the index of the indirect's adress block
	
	
	//_Write to file_//
	use rw pointer to write len amount of data (data written should be in *data)
	//now I have extended the file as needed but I havent written 
		anything or moved the rw pointer, so now i need to write
	
	//If i extended the file, i need to calculate how much i should write 
		before i need to jump to the next block
	//create x amount of new buffers of what to write (one for the current block 
		and the others for the new blcoks)
	//write the first buffer to the file, update the pointer, and move to the
		next block to write to
	//then write the second buffer to the second block and so on until everything has been written
	// keep updateing the rw pointer
	// commit the changes to the disk?

	
	
	*/
	
	

  return 0;
}

/*
	FindFileNameUsingDesc
	
	Param: file descriptor
	
	Uses the name of the file to search through fdName to find
	the File Descriptor
	*/
	char findFileNameUsingDesc(int fileDesc)// Helper for writeFile
	{
		if(fdName.find(fileDesc) == fdName.end())
		{
			tempFilename = '1';
			return tempFilename;
		}
		else
		{
			tempFilename = fdName[fileDesc];
		}
		
		return tempFilename;
	}
	
	/*
	FindFullFileNameUsingDesc
	
	Param: file descriptor
	
	Uses the name of the file to search through fdFullName to find
	the files full name (exp. /a/b/c/d/e)
	*/
	char findFullFileNameUsingDesc(int fileDesc)// Helper for writeFile
	{
		if(fdFullName.find(fileDesc) == fdFullName.end())
		{
			tempFullFilename = '1';
			return tempFullFilename;
		}
		else
		{
			tempFullFilename = fdFullName[fileDesc];
		}
		
		return tempFullFilename;
	}

/*
FindFullFilenameLength

Param: filename array

Loops through the filename array to the end and returns the 
length of the full filename (/a/b/c/d is length 8)
*/
int findFullFilenameLength(char fullfname)// Helper for writeFile
{
	//Find the length of the full filename
	int i = 0;// Loop variable
	bool done = false; // Loop exit condition
	char name, slash; // Used to check if full name is valid
	char next slash; // Used to prevent falling off the name
	int length = 0; //lenght of the full filename 
	
	while(!done)
	{
		slash = fullfname[i];
		name = fullfname[i+1];
		
		if(slash != '/' || !isalpha(name))//Check if the name is valid
		{
			length = 9000;//error
		}
		
		nextSlash = fullfname[i+2];// Look ahead to next slash
		if(nextSlash != '/')// Is there a next slash?
		{
			done = true;
		}
		else
		{
			length += 2;
			i += 2;
		}
	}
	
	return length;
}





















/*
AppendFile

append operation on a file, file descriptor is fileDesc, length is 
the number of bytes to be appended to the buffer pointed to by data

appends the data at the end of the file

Return: -1 if invalid, -2 negative length, -3 operation not permitted,
		or number of bytes appended if successful+
		
		rw is updated to point to the byte follwoing the last byte appended
		
*/
int FileSystem::appendFile(int fileDesc, char *data, int len)
{
	if(len < 0)
		return -2;
	if(fdTable.find(fileDesc) == fdTable.end())
		return -1;
	char *name = fdName[fileDesc];
	if(nameLock.find(*name) != nameLock.end()) //any other reasons it wouldn't be permitted?
		return -3;
	return 0;
}

/*
SeekFile

modifies the rw pointer of the file whose file descriptor is fileDesc

if flag = 0 
	the rw pointer is moved offset bytes forward
Otherwise
	it is set to byte number of offset in the file 
	
Returns: -1 if the file descriptor, offset or flag is invalid, a negative 
			offset is valid
		 -2 if an attempt to go outside the file bounds is made (end
			of file or beginning of file)
		 0 if successful, 

*/
int FileSystem::seekFile(int fileDesc, int offset, int flag)
{
	if(flag < 0)
		return -1;
	if(flag == 0)
	{
		return 0;
	}
	else
	{
		if(fdTable.find(fileDesc) != fdTable.end())
		{
			return 0;
		}
	}
}

/*
RenameFile

Changes the name of the file whose name is pointed to by fname1 to the name
	pointed to by fname2
	
Returns: 0 if successful, -1 if invalid, -2 if doesnt exist, 
	-3 if already exists, -4 if file is opened or locked, -5 if other
	
Note: you can rename a directory with this operation 
*/
int FileSystem::renameFile(char *filename1, int fnameLen1, char *filename2, int fnameLen2)
{
	char *last1;
	for(int i = 0; i < fnameLen1; i+=2)
	{
		char* slash = &filename1[i];
		char* name = &filename1[i+1];
		if(*slash != '/')
			return -1;
		if(!isalpha(*name))
			return -1;
		last1 = name;
	}
	
	char *last2;
	for(int i = 0; i < fnameLen2; i+=2)
	{
		char* slash = &filename2[i];
		char* name = &filename2[i+1];
		if(*slash != '/')
			return -1;
		if(!isalpha(*name))
			return -1;
		last2 = name;
	}
	if(nameType.find(*last1) == nameType.end())
		return -2;
	else if(nameType.find(*last2) != nameType.end())
		return -3;
	else if(nameLock.find(*last1) != nameLock.end())
		return -4;
	else if(nameType.find(*last1) != nameType.end())
	{
		return 0;
	}
	else
		return -5;
}

/*
GetAttribute

get the attributes of a file whose name is pointed to by filename 

Work out the details of these operations based on the file attributes
you choose. You must choose a minimum of two attributes for your filesystem.
*/
int FileSystem::getAttribute(char *filename, int fnameLen /* ... and other parameters as needed */)
{

}

/*
SetAttribute

set the attributes of a file whose name is pointed to by filename 

Work out the details of these operations based on the file attributes
you choose. You must choose a minimum of two attributes for your filesystem.
*/
int setAttribute(char *filename, int fnameLen /* ... and other parameters as needed */)
{

}
