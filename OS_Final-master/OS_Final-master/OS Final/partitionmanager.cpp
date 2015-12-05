#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include "bitvector.h"
#include <iostream>

using namespace std;
// BitVector *bv;

PartitionManager::PartitionManager(DiskManager *dm, char partitionname, int partitionsize)
{
  myDM = dm;
  myPartitionName = partitionname;
  myPartitionSize = myDM->getPartitionSize(myPartitionName);

  /* If needed, initialize bit vector to keep track of free and allocted
     blocks in this partition */
  BitVector *bv = new BitVector(myPartitionSize);
  char buffer[64];
  bv -> setBit(0); // the block the bitvector gets written to
  bv -> setBit(1); // the block the root is allocated to
  bv -> getBitVector((unsigned int *)buffer);
  myDM -> writeDiskBlock(myPartitionName, 0, buffer); 
}

PartitionManager::~PartitionManager()
{
}

/*
 * return blocknum, -1 otherwise
 */
int PartitionManager::getFreeDiskBlock()
{
  /* write the code for allocating a partition block */
  BitVector *bv = new BitVector(myPartitionSize);
  char buffer[64];
  myDM -> readDiskBlock(myPartitionName, 0, buffer);
  bv -> setBitVector((unsigned int *)buffer);
  
	for(int i = 0; i < myPartitionSize; i++)
	{
		if(bv -> testBit(i) == 0)
		{
			bv -> setBit(i);
			return i;
		}
	}
	return -1;
}

/*
 * return 0 for sucess, -1 otherwise
 */
int PartitionManager::returnDiskBlock(int blknum)
{
  /* write the code for deallocating a partition block */
  BitVector *bv = new BitVector(myPartitionSize);
  char buffer[64];
  myDM -> readDiskBlock(myPartitionName, 0, buffer);
  bv -> setBitVector((unsigned int *)buffer);
  
  if(bv -> testBit(blknum) != 0)
  {
	  bv -> resetBit(blknum);
	  return 0;
  }
  return -1;
}


int PartitionManager::readDiskBlock(int blknum, char *blkdata)
{
  return myDM->readDiskBlock(myPartitionName, blknum, blkdata);
}

int PartitionManager::writeDiskBlock(int blknum, char *blkdata)
{
  return myDM->writeDiskBlock(myPartitionName, blknum, blkdata);
}

int PartitionManager::getBlockSize() 
{
  return myDM->getBlockSize();
}
