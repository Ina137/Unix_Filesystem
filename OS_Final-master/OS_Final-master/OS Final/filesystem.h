#include <map>

class FileSystem {
  DiskManager *myDM;
  PartitionManager *myPM;
  char myfileSystemName;
  int myfileSystemSize;
  
  /* declare other private members here */
  std::map<char, char> nameType; //*
  std::map<char, int> nameLock; 
  int lockID;
  std::map<int, char> fdFullname; //file descriptor to full name 
  std::map<char, char> fdToMode; //* file descriptor to mode (read/write)
  int fileDescriptor;
  std::map<int, int*> fdTable;//* file descriptor to rw pointer
  std::map<int, char*> fdName;//* file descriptor to name of file

  public:
    FileSystem(DiskManager *dm, char fileSystemName);
    int createFile(char *filename, int fnameLen);
    int createDirectory(char *dirname, int dnameLen);
    int lockFile(char *filename, int fnameLen);
    int unlockFile(char *filename, int fnameLen, int lockId);
    int deleteFile(char *filename, int fnameLen);
    int deleteDirectory(char *dirname, int dnameLen);
    int openFile(char *filename, int fnameLen, char mode, int lockId);
    int closeFile(int fileDesc);
    int readFile(int fileDesc, char *data, int len);
    int writeFile(int fileDesc, char *data, int len);
    int appendFile(int fileDesc, char *data, int len);
    int seekFile(int fileDesc, int offset, int flag);
    int renameFile(char *filename1, int fnameLen1, char *filename2, int fnameLen2);
    int getAttribute(char *filename, int fnameLen /* ... and other parameters as needed */);
    int setAttribute(char *filename, int fnameLen /* ... and other parameters as needed */);

    /* declare other public members here */
	
	static const int block = 1;// The root block, I need to send this to getInode

};
