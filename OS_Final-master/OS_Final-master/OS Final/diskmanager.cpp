#include "disk.h"
#include "diskmanager.h"
#include <iostream>
using namespace std;

DiskManager::DiskManager(Disk *d, int partcount, DiskPartition *dp)
{
  myDisk = d;
  partCount= partcount;
  int r = myDisk->initDisk();
  char buffer[64];

  /* If needed, initialize the disk to keep partition information */
  diskP = new DiskPartition[partCount];
  if(dp != NULL)
	for(int i = 0; i < partCount; i++)
	{
		diskP[i] = dp[i];
	}
	/* else  read back the partition information from the DISK1 */
  else
	for(int i = 0; i < partCount; i ++)
	{
		char *table;
		int p = myDisk -> readDiskBlock(0, table);
		if(p != 0)//err
		{
			cout << "Error creating partitions"<<endl;
		}
		else{
			for(int i = 0; i < partCount*2; i += 2)
			{
				diskP[i].partitionName = table[i];
				diskP[i].partitionSize = table[i+1];
			}
		}
	}
  

}

/*
 *   returns: 
 *   0, if the block is successfully read;
 *  -1, if disk can't be opened; (same as disk)
 *  -2, if blknum is out of bounds; (same as disk)
 *  -3 if partition doesn't exist
 */
int DiskManager::readDiskBlock(char partitionname, int blknum, char *blkdata)
{
  /* write the code for reading a disk block from a partition */
  int offset = 0; // give right blknum for whole disk, not just partition
  for (int i = 0; i < partCount; i++) 
  {
    if (partitionname == diskP[i].partitionName)
    {
      break;
    }
    else if ( i== partCount) 
    {
      // partition doesn’t exist
      return (-3);
    }
    else
    {
      offset += diskP[i].partitionSize;
    } 
  }

  return myDisk->readDiskBlock(blknum + offset, blkdata); // will return any errors disk has
}
/*
 *   returns: 
 *   0, if the block is successfully written;
 *  -1, if disk can't be opened; (same as disk)
 *  -2, if blknum is out of bounds;  (same as disk)
 *  -3 if partition doesn't exist
 */
int DiskManager::writeDiskBlock(char partitionname, int blknum, char *blkdata)
{
  /* write the code for writing a disk block to a partition */
  int offset = 0; // give right blknum for whole disk, not just partition
  for (int i = 0; i < partCount; i++) 
  {
    if (partitionname == diskP[i].partitionName)
    {
      break;
    }
    else if ( i== partCount) 
    {
      // partition doesn’t exist
      return (-3);
    }
    else
    {
      offset += diskP[i].partitionSize;
    } 
  }

  return myDisk->writeDiskBlock(blknum + offset, blkdata); // will return any errors disk has
}


/*
 * return size of partition
 * -1 if partition doesn't exist.
 */
int DiskManager::getPartitionSize(char partitionname)
{
  /* write the code for returning partition size */
  for (int i = 0; i < partCount; i++) 
  {
    if (partitionname == diskP[i].partitionName) 
    {
        return diskP[i].partitionSize;
    }
    else if (i == partCount) 
    {
        // partition doesn't exist
        return (-1);
    }
  }
}
