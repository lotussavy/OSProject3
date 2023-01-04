/*
 * directory.C of CEG 433/633 File Sys Project
 * pmateti@wright.edu
 */

#include "fs33types.hpp"

#define iNumber(ptr) (*(ulong *) (ptr + strlen((char *)ptr) + 1))
using namespace std;

/* pre:: pfv must point to a proper file volume, in should be > 0;;
 * post:: Construct a directory object, parent == 0 means that on the
 * disk image no changes are made, otherwise yes.
 */

Directory:: Directory(FileVolume * pfv, uint in, uint parent)
{
  fv = pfv;
  dirf = 0;
  dirEntry = 0;
  nInode = in;
  if (parent == 0)
    return;

  fv->fbvInodes.setBit(in, 0); // the inode is in-use
  fv->inodes.setType(in, iTypeDirectory);
  addLeafName((byte *) ".", in);
  addLeafName((byte *) "..", parent);
}

Directory:: ~Directory()
{
  this->namesEnd();
}

/* pre:: dirEntry/dirf may or may not be 0;; post:: Get the next file
 * name + i-number pair from this directory.  Returns ptr to dirEntry,
 * which is a private data member.  Returns 0 when there are no more
 * entries. */

byte * Directory::nextName()
{
  if (dirf == 0)
    dirf = new File(fv, nInode);
  if (dirEntry == 0)
    dirEntry = new byte		// area of mem for file name + i-num
      [fv->superBlock.fileNameLengthMax + 1 + fv->superBlock.iWidth];

  byte * bp = dirEntry;
  while ((*bp = dirf->getNextByte()) != 0)
    bp++ ;
  if (bp == dirEntry)		// end of directory
    return 0;

  for (uint i = 0; i < fv->superBlock.iWidth; i++) // deposit the i-number
    *++bp = dirf->getNextByte();
  return dirEntry;
}

/* Must be called after invocation(s) of nextName(). */

void Directory::namesEnd()
{
  if (dirf) delete dirf;
  dirf = 0;
#if 0
  if (dirEntry) delete dirEntry;
  dirEntry = 0;
#endif
}

/* pre:: in may or may not be in this directory;; post:: If in is in
 * this dir, set dirEntry[] so that it contains the file name +
 * i-number. If in is not there, dirEntry[0] == 0. */

byte * Directory::nameOf(uint in)
{
  byte *bp = 0;
  while ((bp = nextName()) != 0) {
    if (in == (uint)iNumber(bp))
      break;
  }
  namesEnd();
  return bp;
}

/* pre:: ;; post:: Search directory and set dirEntry matching with
 * leafnm.  Return inumber. Otherwise zero is returned. Callers wish to use dirf and dirEntry
 * further; do not do namesEnd(). */

uint Directory::setDirEntry(byte *leafnm)
{
  if (leafnm == 0 || leafnm[0] == 0)
    return 0;

  uint nbToMatch = 1 + strlen((char *) leafnm), result = 0;
  for (byte * bp = 0; (bp = nextName());) {
    if (memcmp(bp, leafnm, nbToMatch) == 0) {
      result = iNumber(bp);
      break;
    }
  }
  return result;
}

uint  Directory::iNumberOf(byte *leafnm)
{
  uint in = setDirEntry(leafnm);
  namesEnd();
  return in;
}

uint okNameSyntax(byte *nm)
{
  return
    nm != 0			// null pointer?
    && nm[0] != 0		// empty "" nm?
    && isAlphaNumDot(nm[0])	// invalid begin char for name?
   && index((char *) nm, '/') == 0	// nm contains a slash?
    ;
}

/* pre:: none;; post:: Add file name newName with its inode number in
 * to this directory. */

void Directory::addLeafName(byte *newName, uint in)
{
  if (in == 0 || okNameSyntax(newName) == 0)
    return;

  // if name is too long, truncate it
  uint newNameLength = strlen((char *) newName);
  if (newNameLength > fv->superBlock.fileNameLengthMax - 1) {
    newNameLength = fv->superBlock.fileNameLengthMax - 1;
    newName[newNameLength] = 0;
  }

  if (setDirEntry(newName) == 0) {
    memcpy(dirEntry, newName, newNameLength + 1);	// append NUL also
    memcpy(dirEntry + newNameLength + 1, &in, fv->superBlock.iWidth);
    dirf->appendBytes
      (dirEntry, newNameLength + 1 + fv->superBlock.iWidth);
  }
  namesEnd();
}

/* pre:: in is valid;; post:: List the directory inode in's content in
 * a manner similar to Unix ls -lia. If printfFlag != 0, output it to
 * stdout.  Return the total number of files.  */

uint Directory::lsPrivate(uint in, uint printfFlag)
{
  uint nFiles = 0;
  Directory *d = new Directory(fv, in, 0);
  for (byte *bp = 0; (bp = d->nextName()); nFiles++) {
    uint in = iNumber(bp);
    if (printfFlag) {
      byte c = (d->fv->inodes.getType(in) == iTypeDirectory? 'd' : '-');//detect directory or file
      printf("%7d %crw-rw-rw-    1 yourName yourGroup %7d Jul 15 12:34 %s\n",
	     in, c, d->fv->inodes.getFileSize(in), bp);
    }
  }
  delete d;
  return nFiles - 2;		// -2 because of "." and ".."
}

uint Directory::ls()
{
  return lsPrivate(nInode, 1);	// 1 ==> printf it
}

uint Directory::countFiles()
{
  uint nFiles = 0;
  for (byte *bp = 0; (bp = this->nextName()); nFiles++);
  namesEnd();
  return nFiles - 2;		// -2 because of "." and ".."
}

uint Directory::findEntries(char * nameDir)
{
  return findEntriesPrivate(nInode,nameDir);	// 1 ==> printf it
}

uint Directory::findEntriesPrivate(uint in,char* nameDir)
{
  Directory*current= getDirectoryPointer(nameDir);
  uint num=current->countFiles();
  return(num);
}


uint Directory::createFile(byte *leafnm, uint dirFlag)
{
  uint in = iNumberOf(leafnm);
  if (in  == 0) 
  {
    in = fv->inodes.getFree();
    if (in > 0) 
    {
      addLeafName(leafnm, in);
      if (dirFlag)
	      delete new Directory(fv, in, nInode);
      else
        fv->inodes.setType(in, iTypeOrdinary);
      fv->inodes.setCount(in,0);
    }
  }
  return in;
}



uint Directory::createHardLink(byte * leafnm, uint in)
{
  addLeafName(leafnm, in);
  fv->inodes.setCount(in,fv->inodes.getCount(in)+1);
	fv->inodes.setType(in, iTypeOrdinary);
  return in;
}


uint Directory::createSoftLink(byte *leafnm, uint dirFlag,byte* path)
{
  uint in = fv->inodes.getFree();
  if (in>0)
  {
    addLeafName(leafnm, in);
    if (dirFlag)
	    delete new Directory(fv, in, nInode);
    fv->inodes.setType(in, iTypeSoftLink);
    fv->inodes.setCount(in,0);
    uint bn = fv->inodes.setPathBlock(in);
    fv->writeBlock(bn, path);
  }
  return in;
}

uint Directory::copySoftLink(byte* from, byte* to)
{
  uint in = iNumberOf(from);
  uint dirFlag;
  if (in>0)
  {
    //get the path of the source
    uint bn=fv->inodes.getPathBlock(in);
    uint bsz=fv->superBlock.nBytesPerBlock;
    byte * buf = new byte[bsz];
    fv->readBlock(bn,buf);
    
    fv->inodes.getType(in)==iTypeDirectory?dirFlag=1:dirFlag=0; //find if source is directory of file
    uint destinationIn=createSoftLink(to,dirFlag,buf); // create new softlink using path and dirFlag
    
    //Update the count of the copied softlink
    uint count=fv->inodes.getCount(in);
    fv->inodes.setCount(destinationIn,count);
  }
  return in;
}


/* Do not delete if it is dot or dotdot.  Do not delete if it is a
 * non-empty dir. */

uint Directory::deleteFile(byte *leafnm, uint freeInodeFlag)
{
  if (strcmp((char *) leafnm, ".") == 0 ||
      strcmp((char *) leafnm, "..") == 0) return 0;
  uint in = setDirEntry(leafnm);
  if (in > 0) {
    dirf->deletePrecedingBytes
      (1 + strlen((char *) leafnm) + fv->superBlock.iWidth);
    if (freeInodeFlag) fv->inodes.setFree(in);
  }
  return in;
}

/* pre:: pn is a dir inode, leafnm != 0, leafnm[0] != 0;; post:: Move
 * file named leafnm whose current parent is pn into this directory.;;
 */

uint Directory::moveFile(uint pn, byte * leafnm)
{
  return TODO("Directory::moveFile");
}


Directory* Directory::getDirectoryPointer(char *nameDir)
{
  return (getDirectoryPointerPrivate(nInode,nameDir));
}

Directory* Directory::getDirectoryPointerPrivate(uint in, char *nameDir)
{
  uint nFiles = 0;
  uint nDirectories=0;
  Directory *d = new Directory(fv, in, 0);
  Directory *current=NULL;
  for (byte *bp = 0; (bp = d->nextName()); nFiles++) 
  {
    uint in = iNumber(bp);
    if (d->fv->inodes.getType(in) == iTypeDirectory)
    {
      uint nbToMatch = 1 + strlen(nameDir);
      nDirectories++;
      if (memcmp(bp, (byte*)nameDir, nbToMatch) == 0)
      {
        current= new Directory(fv,in,0);
      }
    }
  }
  delete d;
  return current;
}

void Directory::lsRecursion()
{
  Directory *d = new Directory(fv, nInode, 0);
  lsRecursionPrivate(d);
}

void Directory::lsRecursionPrivate(Directory* current)
{
  byte *bp = 0; 
  if(!(bp = current->nextName()))
    return; 
  uint in = iNumber(bp);
  if (strcmp((char*)bp,".")!=0 && strcmp((char*)bp,"..")!=0)
  {
    byte c = (current->fv->inodes.getType(in) == iTypeDirectory? 'd' : '-');//detect directory or file
    printf("%7d %crw-rw-rw-    1 yourName yourGroup %7d Jul 15 12:34 %s\n", in, c, current->fv->inodes.getFileSize(in), bp);
    if (current->fv->inodes.getType(in) == iTypeDirectory)
    {
      printf("Inside %s\n",bp);
      Directory* dPointer= current->getDirectoryPointer((char *)bp);
      lsRecursionPrivate(dPointer);
      printf("Directory %s Ends\n",bp);
    }
  }
  lsRecursionPrivate(current);
  //delete current;
}

void Directory::rmRecursion()
{
  Directory *d = new Directory(fv, nInode, 0);
  rmRecursionPrivate(d);
}

void Directory::rmRecursionPrivate(Directory* current)
{
  byte *bp = 0; 
  if(!(bp = current->nextName()))
    return; 
  uint in = iNumber(bp);
  if (strcmp((char*)bp,".")!=0 && strcmp((char*)bp,"..")!=0)
  {
    if (current->fv->inodes.getType(in) == iTypeDirectory)
    {
      Directory* dPointer= current->getDirectoryPointer((char *)bp);
      rmRecursionPrivate(dPointer);
    }
     current->deleteFileWithInode(bp,in,1);
  }
  rmRecursionPrivate(current);
  
  //current->deleteFile(bp,1);
}


void Directory::pwd()
{
  Directory *d = new Directory(fv, nInode, 0);
   return (pwdRecursion(d));
}
void Directory::pwdRecursion(Directory* current)
{
  if (current->nInode==1)
  {
    printf("/");
    return;
  }
  Directory* dPointer=NULL;
  byte *bp = 0;
  bp = current->nextName();
  uint in = iNumber(bp);
  bp=current->nextName();
  namesEnd();
    Directory* parent= current->getDirectoryPointer((char*)bp);
    byte* cName=parent->nameOfForPwd(in);
    dPointer= current->getDirectoryPointer((char *)bp);
  pwdRecursion(dPointer);
  printf("%s/",cName);
}

byte * Directory::nameOfForPwd(uint in)
{
  byte *bp = 0;
  while ((bp = nextName()) != 0) {
    if (in == (uint)iNumber(bp))
      break;
  }
  namesEnd();
  return bp;
}

bool Directory::findFileInDirectory(char *name)
{
  bool fileFlag=false;
  byte *bp = 0;
  while ((bp = nextName()) != 0) 
  {
    if (strcmp(name,(char*)bp)==0)
    {
      fileFlag=true;
      break;
    }
  }
  namesEnd();
  return fileFlag;
}


void Directory::deleteFileWithInode(byte *leafnm, uint in, uint freeInodeFlag)
{
  if (strcmp((char *) leafnm, ".") == 0 ||
      strcmp((char *) leafnm, "..") == 0) return ;
  if (in > 0) {
    dirf->deletePrecedingBytes
      (1 + strlen((char *) leafnm) + fv->superBlock.iWidth);
    if (freeInodeFlag) fv->inodes.setFree(in);
  }
}


uint Directory::lsDirectoryPrivate(uint in, uint printfFlag)
{
  uint nFiles = 0;
  Directory *d = new Directory(fv, in, 0);
  for (byte *bp = 0; (bp = d->nextName()); nFiles++) {
    uint in = iNumber(bp);
    if (printfFlag && strcmp((char*)bp,".")!=0 && strcmp((char*)bp,"..")!=0) {
      byte c = (d->fv->inodes.getType(in) == iTypeDirectory? 'd' : '-');//detect directory or file
      printf("%7d %crw-rw-rw-    1 yourName yourGroup %7d Jul 15 12:34 %s\n",
	     in, c, d->fv->inodes.getFileSize(in), bp);
    }
  }
  delete d;
  return nFiles - 2;		// -2 because of "." and ".."
}

uint Directory::lsDirectory()
{
  return lsDirectoryPrivate(nInode, 1);	// 1 ==> printf it
}


void Directory::rm()
{
  namesEnd();
}
// -eof-
