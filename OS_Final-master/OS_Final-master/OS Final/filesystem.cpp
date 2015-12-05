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


//return blocknum for creating a file called file
//traverses directories listed in filename of fnamelen characters
//fnamelen is always a multiple of 2. we can't find the super-directory of the root.

//upgrade the getDir to getInode, add a char type parameter, 
//and then 4 length is weird, what about /a that you create a directory in root
int FileSystem::getDir(char *filename, int fnamelen, char file, int block)
{	
	char dirList[64];
	myPM -> readDiskBlock(block, dirList);
	if(fnamelen == 4)
	{
		int i = 0;
		while(i<60)
		{
			if(dirList[i] == filename[1]) //e.g. directory inode name = a, filename = /a/b
			{
				return (int) dirList[i+1]; //return directory inode pointer, e.g. dInode = [a | 35 | d | etc.] returns 35
				//should be the block number where the directory inode associated with /b is
			}
			//if this doesn't happen, i goes to 60.
			i+=6; //move 6 at a time, the size of a directory inode.
		}
		if(i == 60)
		{
			if(dirList[60] != 'c')
				return getDir(filename, fnamelen, file, (int) dirList[60]); //we need to go to the next block of directory inodes and keep searching
			else
				return -1; //we didn't find the filename because: 1. the dirList is at the end and 2. we didn't get another block to look in.
		}
	}
	else if(fnamelen < 4 || fnamelen % 2 != 0)
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
			return getDir(shortFn, fnamelen-2, file, (int) dirList[j+1]); //recursive call with the shorter fnamelen and the new block that we want.
		}
		j+=6;
	}
	if(j == 60) //same as above with j instead of i
	{
		if(dirList[60] != 'c')
				return getDir(filename, fnamelen, file, (int) dirList[60]);
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

void traverseDirectories()
{
	//first make sure the file directory is vailid (aka /a/cc/...)
	//look at block 1 of the partition (Directory)
	//read through it and find the directory we need, say /a/b/c
	//if c isnt there, create it. Otherwise do nothing
	
	//partitionName, diskBlock, the data buffer you want the data to be read into
	myPM -> readDiskBlock(partitionName, 1, createBuffer);
	
	//name, type, pointer to another block
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
	int inodeAddress;// Address (Block number) to the beginning of the file Inode 
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
	if(openFileTable.find(*last) != openFileTable.end())
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
	
	//Find the file descriptor
	char tempFileDesc = findFileDescriptor(char last);
	
	//Update nameType
	int temp = updateCharCharMaps(nameType, last);
	if(temp == -3)
	{
		return -3;
	}
	
	//Update fdMode
	int temp = updateCharCharMaps(fdMode, tempFileDesc);
	if(temp == -3)
	{
		return -3;
	}
	
	//Update fdTable 
	if(fdTable.find(tempFileDesc) != fdTable.end())
	{
		if(fdTable[tempFileDesc] == fileDesc)
		{
			std::map<char, int>::iterator del = fdTable.find(tempFileDesc);
			fdTable.erase(del);
		}
		else
		{
			return -3;
		}
	}
	
	//Update fdName 
	if(fdName.find(tempFileDesc) != fdName.end())
	{
		if(fdName[tempFileDesc] == fileDesc)
		{
			std::map<char, int>::iterator del = fdName.find(tempFileDesc);
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
void fillClearBuffers(char clearBuffer, char clearDInodeBuffer)
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
char findActualFilename(char filename, int fnameLen)
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
	int calcFileSizeInNumOfBlocks(int filesize)
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
	char findFileDescriptor(char last)
	{
		if(fdName.find(last) != fdName.end())
		{
			tempFileDesc = fdName[last];
		}
		
		return tempFileDesc;
	}
	
	/*
	UpdateCharCharMaps
	
	Param: char char map, variable used to search for in map,

	Update basically the fdMode, and nameType Maps
	*/
	int updateCharCharMaps(map<char, char> map, char findWith)
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

Returns: 0 if successfully, -1 if doesnt exist, 
		-2 if not empty, -3 cannot be deleted or other
*/
int FileSystem::deleteDirectory(char *dirname, int dnameLen)
{

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
	//Array<char> readBuffer;
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
