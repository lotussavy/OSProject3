/*
 * shell.C -- CEG433 File Sys Project shell
 * pmateti@wright.edu
 */

#include "fs33types.hpp"
#include <sys/stat.h>
#include <sys/wait.h>
#include<stdlib.h>

extern MountEntry *mtab;
extern VNIN cwdVNIN;
FileVolume * fv;                // Suspicious!
Directory * wd;                 // Suspicious!

#define nArgsMax 10
char types[1+nArgsMax];		// +1 for \0

// Global variables that I have created
    int flagForRedirection=0;
    int file;
    int saved_stdout;
    int saved_stdin;
    int flagForPiping=0;
    int flagForBackgroundExecution=0;
    int childFlag=0;

/* An Arg-ument for one of our commands is either a "word" (a null
 * terminated string), or an unsigned integer.  We store both
 * representations of the argument. */

class Arg {
public:
  char *s;
  uint u;
} arg[nArgsMax];

uint nArgs = 0;

uint TODO()
{
  printf("to be done!\n");
  return 0;
}

uint TODO(char *p)
{
  printf("%s to be done!\n", p);
  return 0;
}

uint isDigit(char c)
{
  return '0' <= c && c <= '9';
}

uint isAlphaNumDot(char c)
{
  return c == '.' || 'a' <= c && c <= 'z'
      || 'A' <= c && c <= 'Z' || '0' <= c && c <= '9';
}

int toNum(const char *p)
{
  return (p != 0 && '0' <= *p && *p <= '9' ? atoi(p) : 0);
}

SimDisk * mkSimDisk(byte *name)//Set all the parameters of the disk
{
  SimDisk * simDisk = new SimDisk(name, 0);
  if (simDisk->nSectorsPerDisk == 0) {
    printf("Failed to find/create simDisk named %s\n", name);
    delete simDisk;
    simDisk = 0;
  }
  return simDisk;
}

void doMakeDisk(Arg * a)
{
  SimDisk * simDisk = mkSimDisk((byte *) a[0].s);
  if (simDisk == 0)
    return;
  printf("new SimDisk(%s) = %p, nSectorsPerDisk=%d,"
	 "nBytesPerSector=%d, simDiskNum=%d)\n",
	 simDisk->name, (void*) simDisk, simDisk->nSectorsPerDisk,
	 simDisk->nBytesPerSector, simDisk->simDiskNum);
  delete simDisk;
}

void doWriteDisk(Arg * a)
{
  SimDisk * simDisk = mkSimDisk((byte *) a[0].s);
  if (simDisk == 0)
    return;
  char *st = a[2].s;		// arbitrary word
  if (st == 0)			// if it is NULL, we use ...
    st = "CEG433/633/Mateti";
  char buf[1024];		// assuming nBytesPerSectorMAX < 1024
  for (uint m = strlen(st), n = 0; n < 1024 - m; n += m)
    memcpy(buf + n, st, m);	// fill with several copies of st
  uint r = simDisk->writeSector(a[1].u, (byte *) buf);
  printf("write433disk(%d, %s...) == %d to Disk %s\n", a[1].u, st, r, a[0].s);
  delete simDisk;
}

void doReadDisk(Arg * a)
{
  SimDisk * simDisk = mkSimDisk((byte *) a[0].s);
  if (simDisk == 0)
    return;
  char buf[1024];		// assuming nBytesPerSectorMAX < 1024
  uint r = simDisk->readSector(a[1].u, (byte *) buf);
  buf[10] = 0;			// sentinel
  printf("read433disk(%d, %s...) = %d from Disk %s\n", a[1].u, buf, r, a[0].s);
  delete simDisk;
}

void doQuit(Arg * a)
{
  exit(0);
}

void doEcho(Arg * a)
{
  printf("%s#%d, %s#%d, %s#%d, %s#%d\n", a[0].s, a[0].u,
	 a[1].s, a[1].u, a[2].s, a[2].u, a[3].s, a[3].u);
}

void doMakeFV(Arg * a)
{
  SimDisk * simDisk = mkSimDisk((byte *) a[0].s);
  if (simDisk == 0)
    return;
  fv = simDisk->make33fv();
  printf("make33fv() = %p, Name == %s, Disk# == %d\n",
	 (void*) fv, a[0].s, simDisk->simDiskNum);

  if (fv) {
      wd = new Directory(fv, 1, 0);
      cwdVNIN = mkVNIN(simDisk->simDiskNum, 1);
  }
}

void doCopyTo(byte* from, byte* to)
{
  uint r = fv->write33file(to, from);
  printf("%s%s",from,to);
  printf("write33file(%s, %s) == %d\n", to, from, r);
}

void doCopyFrom(byte* from, byte* to)
{
  uint r = fv->read33file(to, from);
  printf("read33file(%s, %s) == %d\n", to, from, r);
}

void doCopy33(byte* from, byte* to)
{
  uint in= wd->iNumberOf(to);
  if (wd->fv->inodes.getType(in)==iTypeSoftLink)
  {
    wd->copySoftLink(to,from);
  }
  else
  {
    uint r = fv->copy33file(to, from);
    printf("copy33file(%s, %s) == %d\n", to, from, r);
  }
}

void doCopy(Arg * a)
{
  byte* to = (byte *) a[0].s;
  byte* from = (byte *) a[1].s;

  if (a[0].s[0] == '@' && a[1].s[0] != '@') {
    doCopyTo(from, (to + 1));
  }
  else if (a[0].s[0] != '@' && a[1].s[0] == '@') {
    doCopyFrom((from + 1), to);
  }
  else if (a[0].s[0] != '@' && a[1].s[0] != '@') {
    doCopy33(from, to);
  }
  else {
    puts("Wrong arguments to cp.");
  }
}

void doLsLong(Arg * a)
{
  printf("\nDirectory listing for disk %s, cwdVNIN == 0x%0lx begins:\n",
	 wd->fv->simDisk->name, (ulong) cwdVNIN);
   if (a[0].s==0)
   {
     wd->ls();
   }
   else
   {
     printf("%s",a[0].s);
   }
  printf("Directory listing ends.\n");
}

void helperDoRm(char* name)
{
  uint in=wd->iNumberOf((byte*)name);
  if (index(name, '.')!=NULL) //For files only
  {
    if (wd->fv->inodes.getCount(in)!=0)
    {
      wd->deleteFile((byte *) name,0);
      wd->fv->inodes.setCount(in,(wd->fv->inodes.getCount(in))-1);
      printf("The current link count is %d\n",wd->fv->inodes.getCount(in));
    }
    else
    {
      wd->deleteFile((byte *)name,1);
      printf("The current link count is 0\n");
    }
    wd->rm(); // for namesEnd()
  }
  else  //For directories only
  {
    //char* name = a[0].s;
    bool filePresent=wd->findFileInDirectory(name);
    if (filePresent)
    {
      if (wd->fv->inodes.getType(in)== iTypeDirectory)
      {
        uint numFiles = wd->findEntries(name);
        printf("Directory %s has %d entries.\n", name, numFiles);
        if (numFiles==0)
        {
          wd->deleteFile((byte*)name,1);
          wd->rm();
        }
      }
      else
      {
        if (wd->fv->inodes.getCount(in)!=0)
        {
          wd->deleteFile((byte *) name,0);
          wd->fv->inodes.setCount(in,(wd->fv->inodes.getCount(in))-1);
          printf("The current link count is %d\n",wd->fv->inodes.getCount(in));
        }
        else
        {
          wd->deleteFile((byte *)name,1);
          printf("The current link count is 0\n");
        }
        wd->rm(); // for namesEnd()
      }
    }
  }
}

void doRm(Arg * a)
{
  helperDoRm(a[0].s);
}

void doInode(Arg * a)
{
  uint in = a[0].u;
  wd->fv->inodes.show(in);
  // if (wd->fv->inodes.getType(in)== iTypeOrdinary)
  // {
  //   printf("iType=File Node\n");
  // }
  // if (wd->fv->inodes.getType(in)== iTypeDirectory)
  // {
  //   printf("iType=Directory Node\n");
  // }
  // if (wd->fv->inodes.getType(in)== iTypeSoftLink)
  // {
  //   printf("iType=SoftLink Node\n");
  //   uint bn=wd->fv->inodes.getPathBlock(in);
  //   uint bsz=wd->fv->superBlock.nBytesPerBlock;
  //   byte * buf = new byte[bsz];
  //   wd->fv->readBlock(bn,buf);
  //   printf("%s\n",buf);
  // }
  // printf("Link Count=%d\n",wd->fv->inodes.getCount(in));
  
}

void doMkDir(Arg * a)
{
  if (wd->findFileInDirectory(a[0].s))
  {
    printf("0\n");
    return;
  }
  
  // createfile from directory to create in current directory and from volume to create in root
  wd->createFile((byte *) a[0].s,1);
  uint iNumber=wd->iNumberOf((byte*)a[0].s);
  wd->fv->inodes.show(iNumber);
}


void helperDoChdir(char* name)
{
  if (index(name, '/')!=NULL)
  {
    char * token = strtok(name, "/");
    while( token != NULL ) 
    {
      if (wd->findFileInDirectory(token))
      {
        Directory* current=wd->getDirectoryPointer(token);
        wd=current;
      }
      //printf( " %s\n", token );
      token = strtok(NULL,  "/");
    }
    wd->pwd();
    printf("\n");
  }
  else
  {
      if (wd->findFileInDirectory(name))
      {
        Directory* current=wd->getDirectoryPointer(name);
        wd=current;
        wd->pwd();
        printf("\n");
      }
  }
}

void doChDir(Arg * a)
{
  uint iNumber=wd->iNumberOf((byte*)a[0].s);
  if (wd->fv->inodes.getType(iNumber)== iTypeSoftLink)
  {
    uint bn=wd->fv->inodes.getPathBlock(iNumber);
    uint bsz=wd->fv->superBlock.nBytesPerBlock;
    byte * buf = new byte[bsz];
    wd->fv->readBlock(bn,buf);
    helperDoChdir((char*)buf);
  }
  else
    helperDoChdir(a[0].s);
}

void doPwd(Arg * a)
{
  wd->pwd();
  printf("\n");
}



//Its an function to mv when source is file and destination is file
void mvForFileToFile(char *source, char* destination)
{
    uint iNumberSource=wd->iNumberOf((byte*)source);
    uint iNumberDestination=wd->iNumberOf((byte*)destination);
    wd->deleteFile((byte *) source,0); //delete source file without freeing inodes

    if (iNumberDestination!=0) //Destination file exist
    {
       wd->deleteFile((byte*)destination,1); //delete destination file releasing inodes
    }
    wd->addLeafName((byte*) destination, iNumberSource); // create file again using the source inodes
    //printf("success\n");
}


//Its an function for mv command when source is file and destination is directory
void mvForFileToDirectory(char* source, char* destination)
{
  Directory* destinationDirectory= wd->getDirectoryPointer(destination);
  uint iNumberSource=wd->iNumberOf((byte*)source);
  uint iNumberDestination=wd->iNumberOf((byte*)destination);
  uint iNumberSourceFile=destinationDirectory->iNumberOf((byte*)source);
  wd->deleteFile((byte *) source,0); //delete source file without freeing inodes

// //Check if destination exist or not
  if (iNumberDestination==0) //Destination doesnot exist
  {
    return; //Source is file and destination directory doesnot exist 
  }
  else//Destination directory exist
  {
    //Check for file in the destination directory
    if (iNumberSourceFile!=0) //File exist in destination folder
    {
      destinationDirectory->deleteFile((byte*)source,1); //delete destination file releasing inodes
    }
    destinationDirectory->addLeafName((byte*) source, iNumberSource); // create file again using the source inodes
  }
  //printf("success\n");
}


//Its an function for mv command when source is directory and destination is directory
void mvForDirectoryToDirectory(char* source, char* destination)
{
  Directory* destinationDirectory= wd->getDirectoryPointer(destination);
  uint iNumberSource=wd->iNumberOf((byte*)source);
  uint iNumberDestination=wd->iNumberOf((byte*)destination);
  uint iNumberSourceDirectory=destinationDirectory->iNumberOf((byte*)source);
  
  wd->deleteFile((byte *) source,0); //delete source directory without freeing inodes
  destinationDirectory->addLeafName((byte*) source, iNumberSource);

  //Check if destination exist or not
  if (iNumberDestination==0) //Destination doesnot exist
  {
    wd->addLeafName((byte*)destination,iNumberSource); 
  }
  else
  {
    //Check for directory in the destination directory
    if (iNumberSourceDirectory!=0) //Directory exist in destination folder
    {
      destinationDirectory->deleteFile((byte*)source,1); //delete destination directory releasing inodes
    }
    destinationDirectory->addLeafName((byte*) source, iNumberSource); // create directory again using the source inodes
  }
  //printf("success\n");
}

void mvForPathname(char* source, char* destination)
{
  // char* newSource;
  // uint iNumberSource;
  // char* newDestination;
  // uint iNmuberDestination;
  //   char * token = strtok(source, "/");
  //   while( token != NULL ) 
  //   {
  //     if (wd->findFileInDirectory(token))
  //     {
  //       strcpy(token,newSource);
  //     }
  //     token = strtok(NULL,  "/");
  //   }
  //   iNumberSource=wd->iNumberOf(newSource);
  printf("Not done\n");
}

void mvForSoftLink(char* source,char* destination)
{
    uint iNumberSource=wd->iNumberOf((byte*)source);
    uint iNumberDestination=wd->iNumberOf((byte*)destination);
    wd->deleteFile((byte *) source,0); //delete source without freeing inodes

    if (iNumberDestination!=0) //Destination exist
    {
       wd->deleteFile((byte*)destination,1); //delete destination releasing inodes
    }
    wd->addLeafName((byte*) destination, iNumberSource); // create again using the source inodes
    //printf("success\n");
}


void doMv(Arg * a)
{
  char* source = a[0].s;
  char* destination=a[1].s;
  int dirFlagDestination;
  int dirFlagSource;
  if (index(destination, '/')!=NULL && index(source, '/')!=NULL)
  {
    mvForPathname(source,destination);
  }
  
  uint iNumberSource=wd->iNumberOf((byte*)source);
  if (iNumberSource==0)
  {
    return;  //return if no source file or directory exist
  }

  if (wd->fv->inodes.getType(iNumberSource)==iTypeSoftLink)
  {
    mvForSoftLink(source,destination);
    return;
  }

  index(destination, '.')==NULL ? dirFlagDestination=1: dirFlagDestination=0;
  index(source, '.')==NULL ? dirFlagSource=1: dirFlagSource=0;

  if ( dirFlagSource==0 && dirFlagDestination==0)
    mvForFileToFile(source,destination);      
  else if (dirFlagSource==1 && dirFlagDestination==0)
    return;
  else if (dirFlagSource==0 && dirFlagDestination==1)
    mvForFileToDirectory(source,destination);
  else if (dirFlagSource==1 && dirFlagDestination==1)
    mvForDirectoryToDirectory(source,destination);
  wd->rm();
}

void doMountDF(Arg * a)		// arg a ignored
{
  TODO("doMountDF");
}

void doMountUS(Arg * a)
{
  TODO("doMountUS");
}

void doUmount(Arg * a)
{
  TODO("doUmount");
}

//Function to print all the files and folders in current directory
void doLsDirectory(Arg* a)
{
  char* directoryName = a[0].s;
  Directory* current = wd->getDirectoryPointer(directoryName);
  if(current==NULL)
  return;
  current->lsDirectory();
}

//Function to print the inode of the file
void doInodeFile(Arg* a)
{
  uint iNumber=wd->iNumberOf((byte*)a[0].s);
  wd->fv->inodes.show(iNumber);
}

//Function that print all the files inside the directories as well as the sub-directories
void doLsRecursion(Arg* a)
{
  if (strcmp(a[0].s,"-lR")!=0)
  {
    return;
  }
  char* directoryName = a[1].s;
  Directory* current = wd->getDirectoryPointer(directoryName);
  current->lsRecursion();
  current=wd;
}

void doRmRecursion(Arg* a)
{
  if (strcmp(a[0].s,"-fr")!=0)
  {
    return;
  }
  if (wd->findFileInDirectory(a[1].s) && index(a[0].s, '.')==NULL)
  {
    char* directoryName = a[1].s;
    Directory* current = wd->getDirectoryPointer(directoryName);
    current->rmRecursion();
    wd->deleteFile((byte*)directoryName,1);
    current=wd;
  }
}

void doLn(Arg* a)
{
  char* npnm=a[1].s;
  char* opnm=a[0].s;
  if (wd->findFileInDirectory(npnm) || index(npnm, '.')==NULL)
  {
    printf("0\n");
    return;
  }
  if (!(wd->findFileInDirectory(opnm)) || index(opnm, '.')==NULL)
  {
    printf("0\n");
    return;
  }
  uint iNumber=wd->iNumberOf((byte*)opnm);
  wd->createHardLink((byte*)npnm,iNumber);
  iNumber=wd->iNumberOf((byte*)npnm);
  printf("Inode number of %s is %d.\n", npnm, iNumber);
}



void doLnPath(Arg* a)
{
  char* source=a[0].s;
  char * token;
  char *fileName=(char*)malloc(20);
  Directory* current=wd;
  uint iNumber;
  if (index(source, '.')==NULL)
  {
    printf("0\n");
    return;
  }
  // if (index(source, '/')!=NULL)
  // {
  //   token = strtok(source, "/");
  //   //printf("%s",token);
  //   if (wd->findFileInDirectory(token))
  //   {
  //     current=wd->getDirectoryPointer(token);
  //   }
  //   else
  //   {
  //     printf("0\n");
  //     return;
  //   }
  //   token = strtok(NULL,  "/");
  //   printf( " %s\n", token );
  //   if (!(current->findFileInDirectory(token)))
  //   {
  //     printf("0\n");
  //     return;
  //   }
  // }
  //printf("Got you\n");
  if (index(source, '/')!=NULL)
  {
    token = strtok(source, "/");
    while( token != NULL ) 
    {
      if (current->findFileInDirectory(token))
      {
        if (index(token,'.')==NULL)
        {
          current=current->getDirectoryPointer(token);
          //wd=current;
        }
        else
        {
          iNumber=current->iNumberOf((byte*)token);
        }
      }
      //printf( " %s\n", token );
      strcpy(fileName,token);
      token = strtok(NULL,  "/");
    }
  }

  //uint iNumber=current->iNumberOf((byte*)token);
  wd->createHardLink((byte*)fileName,iNumber);
  free(fileName);
}



void doLnS(Arg* a)
{
  uint dirflag;
  if (strcmp(a[0].s,"-s")!=0)
  {
    return;
  }
  char* npnm=a[2].s;
  char* opnm=a[1].s;
  if (wd->findFileInDirectory(npnm))
  {
    printf("0\n");
    return;
  }
  if (!(wd->findFileInDirectory(opnm)))
  {
    printf("0\n");
    return;
  }

  if (index(opnm, '.')==NULL)
    dirflag=1;
  else
    dirflag=0;

  // char* npnmNew="/";
  // strcat(npnmNew,npnm);

  //strcat(npnm,"/");
  wd->createSoftLink((byte*) npnm,dirflag,(byte*) opnm);
  uint iNumber=wd->iNumberOf((byte*)npnm);
  wd->fv->inodes.show(iNumber);
}

/* The following describes one entry in our table of commands.  For
 * each cmmdName (a null terminated string), we specify the arguments
 * it requires by a sequence of letters.  The letter s stands for
 * "that argument should be a string", the letter u stands for "that
 * argument should be an unsigned int."  The data member (func) is a
 * pointer to the function in our code that implements that command.
 * globalsNeeded identifies whether we need a volume ("v"), a simdisk
 * ("d"), or a mount table ("m").  See invokeCmd() below for exact
 * details of how all these flags are interpreted.
 */

class CmdTable {
public:
  char *cmdName;
  char *argsRequired;
  char *globalsNeeded;		// need d==simDisk, v==cfv, m=mtab
  void (*func) (Arg * a);
} cmdTable[] = {
  {"cd", "s", "v", doChDir},
  {"cp", "ss", "v", doCopy},
  {"echo", "ssss", "", doEcho},
  {"inode", "u", "v", doInode},
  {"ls", "", "v", doLsLong},
  {"lslong", "", "v", doLsLong},
  {"mkdir", "s", "v", doMkDir},
  {"mkdisk", "s", "", doMakeDisk},
  {"mkfs", "s", "", doMakeFV},
  {"mount", "us","", doMountUS},
  {"mount", "", "", doMountDF},
  {"mv", "ss", "v", doMv},
  {"rddisk", "su", "", doReadDisk},
  {"rmdir", "s", "v", doRm},
  {"rm", "s", "v", doRm},
  {"pwd", "", "v", doPwd},
  {"q", "", "", doQuit},
  {"quit", "", "", doQuit},
  {"umount", "u", "m", doUmount},
  {"wrdisk", "sus", "", doWriteDisk},
  {"ls","s","",doLsDirectory},
  {"inode","s","",doInodeFile},
  {"ls","ss","",doLsRecursion},
  {"rm","ss","",doRmRecursion},
  {"ln","ss","",doLn},
  {"ln","sss","",doLnS},
  {"ln","s","",doLnPath}
};

uint ncmds = sizeof(cmdTable) / sizeof(CmdTable);

void usage()
{
  printf("The shell has only the following cmds:\n");
  for (uint i = 0; i < ncmds; i++)
    printf("\t%s\t%s\n", cmdTable[i].cmdName, cmdTable[i].argsRequired);
  printf("Start with ! to invoke a Unix shell cmd\n");
}

/* pre:: k >= 0, arg[] are set already;; post:: Check that args are
 * ok, and the needed simDisk or cfv exists before invoking the
 * appropriate action. */

void invokeCmd(int k, Arg *arg)
{
  uint ok = 1;
  if (cmdTable[k].globalsNeeded[0] == 'v' && cwdVNIN == 0) {
    ok = 0;
    printf("Cmd %s needs the cfv to be != 0.\n", cmdTable[k].cmdName);
  }
  else if (cmdTable[k].globalsNeeded[0] == 'm' && mtab == 0) {
    ok = 0;
    printf("Cmd %s needs the mtab to be != 0.\n", cmdTable[k].cmdName);
  }

  char *req = cmdTable[k].argsRequired;
  uint na = strlen(req);
  for (uint i = 0; i < na; i++) {
    if (req[i] == 's' && (arg[i].s == 0 || arg[i].s[0] == 0)) {
      ok = 0;
      printf("arg #%d must be a non-empty string.\n", i);
    }
    if ((req[i] == 'u') && (arg[i].s == 0 || !isDigit(arg[i].s[0]))) {
	ok = 0;
	printf("arg #%d (%s) must be a number.\n", i, arg[i].s);
    }
  }
  if (ok)
    (*cmdTable[k].func) (arg);
}

/* pre:: buf[] is the command line as typed by the user, nMax + 1 ==
 * sizeof(types);; post:: Parse the line, and set types[], arg[].s and
 * arg[].u fields.
 */

void setArgsGiven(char *buf, Arg *arg, char *types, uint nMax)
{
  for (uint i = 0; i < nMax; i++) {
    arg[i].s = 0;
    types[i] = 0;
  }
  types[nMax] = 0;

  strtok(buf, " \t\n");		// terminates the cmd name with a \0

  for (uint i = 0; i < nMax;) {
      char *q = strtok(0, " \t");
      if (q == 0 || *q == 0) break;
      arg[i].s = q;
      arg[i].u = toNum(q);
      types[i] = isDigit(*q)? 'u' : 's';
      nArgs = ++i;
  }
}

/* pre:: name pts to the command token, argtypes[] is a string of
 * 's'/'u' indicating the types of arguments the user gave;; post::
 * Find the row number of the (possibly overloaded) cmd given in
 * name[].  Return this number if found; return -1 otherwise. */

int findCmd(char *name, char *argtypes)
{
  for (uint i = 0; i < ncmds; i++) {
    if (strcmp(name, cmdTable[i].cmdName) == 0
	&& strcmp(argtypes, cmdTable[i].argsRequired) == 0) {
      return i;
    }
  }
  return -1;
}

void ourgets(char *buf) {
  fgets(buf, 1024, stdin);
  char * p = index(buf, '\n');
  if (p) *p = 0;
}
/* Following is the function that implements the redirection
  It creates file with the name as mentioned in buffer updates the
  standard output and after doing all the output function value of STDOUt
  restored to perform other task as usual
*/

int redirectionFunction(char* buf)
{
    char* fileToWrite;
      fileToWrite = index(buf,'>')+2;
      file= open(fileToWrite, O_WRONLY | O_CREAT, 0777);
      saved_stdout = dup(STDOUT_FILENO);
      dup2(file, STDOUT_FILENO);
      flagForRedirection=1;
    uint i=0;
    while(buf[i] !='>')
    {
      i++;
    }
    buf[i-1]='\0';
    return(file);
}
/*This fucntion implements the pipe in the Shell. It supports up to two pipes
It is implemented mainly by usnign fork() and pipe() function at its core.
Child process are created whereever necessay to implement the pipe
*/
void pipingFunction(char* buf)
{
  int numberOfPipes=0;
  int a=0;
  int k;
  int i=0;
  int fd[2];
  char* commandToExecute2;
  char* commandToExecute3;
  int status =0;
  pid_t wpid;
  while(buf[a] !='\0')
  {
    a++;
    if (buf[a]=='|')
    {
      numberOfPipes++;
    }
  }
  if (numberOfPipes==1)
  {
      pipe(fd);
      flagForPiping=1;
      commandToExecute2=index(buf,'|')+2;
      while(buf[i] !='|')
      {
          i++;
      }
      buf[i-1]='\0';
      

      int pid1=fork();
      if (pid1==0)
      {
        childFlag=1;
        saved_stdout = dup(STDOUT_FILENO);
        dup2(fd[1],STDOUT_FILENO);
        close(fd[0]);
        if (buf[0] == '!')    // begins with !, execute it as
              system(buf + 1);    // a normal shell cmd
        else
        {
          setArgsGiven(buf, arg, types, nArgsMax);
          k = findCmd(buf, types);
          if (k >= 0)
            invokeCmd(k, arg);
          else
            usage();
        }
      }
      else
      {
        while ((wpid = wait(&status)) > 0);
        saved_stdin = dup(STDIN_FILENO);
        dup2(fd[0],STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        buf=commandToExecute2;
        if (buf[0] == '!')    // begins with !, execute it as
          system(buf + 1);    // a normal shell cmd
        else
        {
          setArgsGiven(buf, arg, types, nArgsMax);
          k = findCmd(buf, types);
          if (k >= 0)
            invokeCmd(k, arg);
          else
            usage();
        }
        dup2(saved_stdin, STDIN_FILENO);
      }
  }
  else
  {
    flagForPiping=1;
    commandToExecute2=index(buf,'|')+2;
    i=0;
    while(buf[i] !='|')
    {
        i++;
    }
    buf[i-1]='\0';
    commandToExecute3= index(commandToExecute2,'|')+2;
    i=0;
    while(commandToExecute2[i] !='|')
    {
          i++;
    }
    commandToExecute2[i-1]='\0';
    
    int fd[2][2];
    for ( i = 0; i < 2; i++)
    {
      pipe(fd[i]);
    }
    
    int pid1=fork();
    if (pid1==0)
    {
      childFlag=1;
      saved_stdout = dup(STDOUT_FILENO);
      dup2(fd[0][1],STDOUT_FILENO);
      close(fd[0][0]);
      close(fd[1][0]);
      close(fd[1][1]);
            
      if (buf[0] == '!')    // begins with !, execute it as
        system(buf + 1);    // a normal shell cmd
      else
      {
        setArgsGiven(buf, arg, types, nArgsMax);
        k = findCmd(buf, types);
        if (k >= 0)
          invokeCmd(k, arg);
        else
          usage();
      }
    }
    else
    {
      while ((wpid = wait(&status)) > 0);
      strcpy(buf,commandToExecute2);
          int pid2=fork();
          if (pid2==0)
          {
            childFlag=2;
            saved_stdout = dup(STDOUT_FILENO);
            saved_stdin= dup(STDIN_FILENO);
            dup2(fd[1][1],STDOUT_FILENO);
            dup2(fd[0][0],STDIN_FILENO);
            close(fd[0][1]);
            close(fd[1][0]);
                  
            if (buf[0] == '!')    // begins with !, execute it as
              system(buf + 1);    // a normal shell cmd
            else
            {
              setArgsGiven(buf, arg, types, nArgsMax);
              k = findCmd(buf, types);
              if (k >= 0)
                invokeCmd(k, arg);
              else
                usage();
            }
          }
          else
          {
            while ((wpid = wait(&status)) > 0);
            strcpy(buf,commandToExecute3);
            if (buf[0] == '!')    // begins with !, execute it as
              system(buf + 1);    // a normal shell cmd
            else
            {
              setArgsGiven(buf, arg, types, nArgsMax);
              k = findCmd(buf, types);
              if (k >= 0)
                invokeCmd(k, arg);
              else
                usage();
            }
          }
    }
  }
}


/*
This is the fuction for implementing the background execution in the shell.
It uses fork function to implement background execution in the child while 
returning to the main.
*/
void backgroundFunction(char* buf)
{
  int k;
  flagForBackgroundExecution=1;
  uint i=0;
  while(buf[i] !='&')
  {
      i++;
  }
  int pid4=fork();
  if (pid4==0)
  {
    childFlag=1;
    if (buf[0] == '!')    // begins with !, execute it as
      system(buf + 1);    // a normal shell cmd
    else
    {
      buf[i-1]='\0';
      setArgsGiven(buf, arg, types, nArgsMax);
      k = findCmd(buf, types);
      if (k >= 0)
        invokeCmd(k, arg);
      else
        usage();
    }
  }
}

int main()
{
    char buf[1024];   // better not type longer than 1023 chars
    usage();
    for (;;) 
    {
        flagForPiping=0;
        flagForRedirection=flagForBackgroundExecution=0;
        *buf = 0;     // clear old input
        if (childFlag==0)
        {
          printf("%s", "sh33% "); // prompt
          ourgets(buf);
          printf("cmd [%s]\n", buf);  // just print out what we got as-is
        }
        

        if (buf[0] == 0)
          continue;
        
        if (buf[0] == '#')
          continue;     // this is a comment line, do nothing
        


        if ((buf[0] == '!') && !index(buf, '|') && !index(buf,'&'))    // begins with !, execute it as
          system(buf + 1);    // a normal shell cmd
        else 
        {
          if (index(buf, '>'))
          {
            file=redirectionFunction(buf);
          }
          if (index(buf, '|'))
          {
            pipingFunction(buf);
            //childFlag=0;
          }
          if (index(buf, '&'))
          {
            backgroundFunction(buf);
          }

          if (flagForPiping==0 && flagForBackgroundExecution==0)
          {
              setArgsGiven(buf, arg, types, nArgsMax);
              int k = findCmd(buf, types);

              if (k >= 0)
              {
                  invokeCmd(k, arg);
                  if (flagForRedirection==1)
                  {
                        dup2(saved_stdout, STDOUT_FILENO);
                        close(file);
                  }
              }
              else
                usage();
          }
        }
        if (childFlag==1)
        {
          dup2(saved_stdout, STDOUT_FILENO);
          //printf("I am in child");
          exit(0);
        }
        if (childFlag==2)
        {
          dup2(saved_stdout, STDOUT_FILENO);
          dup2(saved_stdin,STDIN_FILENO);
          //printf("I am in child");
          exit(0);
        }
    }
}

// -eof-

