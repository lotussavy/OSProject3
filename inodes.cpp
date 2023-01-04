/*
 * inodes.C of CEG 433/633 File Sys Project
 * pmateti@wright.edu
 */

#include "fs33types.hpp"

#define xCount (fv->superBlock.iHeight - 3)
#define xType (fv->superBlock.iHeight - 2)
#define xFileSize (fv->superBlock.iHeight - 1)
#define xPath 0

/* pre:: fv->superBlock partially initialized, iHeight includes
 * file-size field, iHeight >= 3 ;; post:: Construct the inode array
 * on the disk. */

uint Inodes::create
    (FileVolume * pfv, uint nBegin, uint nInodes, uint iHeight) {
  uint iWidth = sizeof(uint);		// assumption

  // *INDENT-OFF* // Decide how many indirect entries to have
  uint iIndirect = 0;
  if (iHeight > 4)    iIndirect = 1;
  if (iHeight > 5)    iIndirect = 2;
  if (iHeight > 6)    iIndirect = 3;
  // *INDENT-ON*

  fv = pfv;
  uint bsz = fv->superBlock.nBytesPerBlock;
  fv->superBlock.nBlockBeginInodes = nBegin;
  fv->superBlock.nInodes = nInodes;
  fv->superBlock.iWidth = iWidth;
  fv->superBlock.iHeight = iHeight;
  fv->superBlock.nBlocksOfInodes =
      (nInodes * iWidth * iHeight + bsz - 1) / bsz;
  fv->superBlock.inodesPerBlock = bsz / iWidth / iHeight;
  fv->superBlock.iDirect = iHeight - 1 - 1 - 1 - iIndirect;	// see xType, xFileSize, xCount

  // set all inodes to zero, and mark blocks occupied by inodes as in-use
  uintbuffer = (uint *) new byte[bsz]; // inodes from one block
  memset(uintbuffer, 0, bsz);
  for (uint i = fv->superBlock.nBlockBeginInodes,
       j = i + fv->superBlock.nBlocksOfInodes; i < j; i++) {
    fv->writeBlock(i, uintbuffer);
    fv->fbvBlocks.setBit(i, 0);
  }
  return nInodes;
}

uint Inodes::reCreate(FileVolume * pfv)
{
  fv = pfv;
  uint bsz = fv->superBlock.nBytesPerBlock;
  uintbuffer = (uint *) new byte[bsz];
  return fv->superBlock.nInodes;
}

/* pre:: 0 < in < nInodes;; post:: Read the disk block containing
 * inode numbered in into uintbuffer, and return a ptr to it. If ne !=
 * 0, set *ne to the number of blocks in file with inode in. */

uint *Inodes::getInode(uint in, uint * ne)
{
  uint nblock = fv->superBlock.nBlockBeginInodes
      + in / fv->superBlock.inodesPerBlock;
  fv->readBlock(nblock, uintbuffer);
  uint *pin = uintbuffer
      + (in % fv->superBlock.inodesPerBlock) * fv->superBlock.iHeight;
  if (ne != 0) {
    uint bsz = fv->superBlock.nBytesPerBlock;
    uint fileSize = pin[xFileSize];
    *ne = (fileSize + bsz - 1) / bsz;
  }
  return pin;
}

/* pre:: inode numbered in is residing in uintbuffer ;; post:: Write
 * the inode numbered in to disk from uintbuffer. Other inodes that
 * happen to be in the uintbuffer also will be written out to disk. */

uint Inodes::putInode(uint in)
{
  uint nblock = fv->superBlock.nBlockBeginInodes
      + in / fv->superBlock.inodesPerBlock;
  return fv->writeBlock(nblock, uintbuffer);
}

/* pre:: none ;; post:: Return the number of a free inode,
 * if available and set the inode to all-zero, 0 otherwise. */

uint Inodes::getFree()
{
  uint in = fv->fbvInodes.getFreeBit();
  if (in > 0) {
    uint *pin = getInode(in, 0);
    memset(pin, 0, fv->superBlock.iWidth * fv->superBlock.iHeight);
    putInode(in);
  }
  return in;
}

/* pre:: 0 < in < nInodes ;; post:: Return the inode entry at index x
 * of inode numberd in. */

uint Inodes::getEntry(uint in, uint x)
{
  uint *pin = getInode(in, 0);
  return pin[x];
}

uint Inodes::setEntry(uint in, uint x, uint tp)
{
  uint *pin = getInode(in, 0);
  pin[x] = tp;
  putInode(in);
  return pin[x];
}

uint Inodes::getType(uint in)
{
  return getEntry(in, xType);
}

uint Inodes::setType(uint in, uint tp)
{
  return setEntry(in, xType, tp);
}


//This is code to get and set the count of the inode

uint Inodes::getCount(uint in)
{
  return getEntry(in, xCount);
}

uint Inodes::setCount(uint in, uint tp)
{
  return setEntry(in, xCount, tp);
}


//This is code to get and set the path of the soft link inode

uint Inodes::getPathBlock(uint in)
{
  return getEntry(in, xPath);
}

uint Inodes::setPathBlock(uint in)
{
  uint bn = fv->getFreeBlock();
  return setEntry(in, xPath, bn);
}


/* pre:: 0 < in < nInodes ;; post:: Return the size of file whose
 * i-number is in. */

uint Inodes::getFileSize(uint in)
{
  return getEntry(in, xFileSize);
}

uint Inodes::setFileSize(uint in, uint sz)
{
  return setEntry(in, xFileSize, sz);
}

uint Inodes::incFileSize(uint in, int inc)
{
  uint *pin = getInode(in, 0);	// TBD use get/setFileSize
  pin[xFileSize] += inc;
  putInode(in);
  return pin[xFileSize];
}

uint Inodes::setSingleIndirect(uint * single, uint nu, uint bn)
{
  uint changed=0;
  uint blockNumber;
  if (nu==0)
  {
    blockNumber=fv->getFreeBlock();
    *single=blockNumber;
    changed=1;
  }
  uint bsz=fv->superBlock.nBytesPerBlock;
  uint * buf = new uint[bsz];
  fv->readBlock(*single,buf);
  buf[nu]=bn;
  fv->writeBlock(*single,buf);
  //printf("%d\n",nu);
  return changed;
}

uint Inodes::setDoubleIndirect(uint * duble, uint nu, uint bn)
{
  uint changed=0;
  uint blockNumber;
  uint bsz=fv->superBlock.nBytesPerBlock;
  if (*duble==0)
  {
    blockNumber=fv->getFreeBlock();
    *duble=blockNumber;
    changed=1;
  }
  uint firstIndex=nu/(bsz/4);
  uint * buf = new uint[bsz];
  fv->readBlock(*duble,buf);
  if (buf[firstIndex]==0)
  {
    blockNumber=fv->getFreeBlock();
    //uint i=nu/(bsz/4);
    buf[firstIndex]=blockNumber;
    fv->writeBlock(*duble,buf);
    //changed=1;
  }
  //uint first=nu/(bsz/4);
  uint second=nu%(bsz/4);
  uint * buf1 = new uint[bsz];
  uint* buf2= new uint[bsz];
  fv->readBlock(*duble,buf1);
  fv->readBlock(buf1[firstIndex],buf2);
  buf2[second]=bn;
  fv->writeBlock(buf1[firstIndex],buf2);
  return changed;
}

uint Inodes::setTripleIndirect(uint * triple, uint nu, uint bn)
{
  return TODO("Inodes::setTripleIndirect");
}

/* pre:: none;; post:: To inode numbered in, append block number bn.  */

uint Inodes::setLastBlockNumber(uint in, uint bn)
{
  uint bnpb = fv->superBlock.nBytesPerBlock / fv->superBlock.iWidth;
  uint iDirect = fv->superBlock.iDirect;
  uint iIndirectOne = iDirect + bnpb;
  uint iIndirectTwo = iIndirectOne + bnpb * bnpb;
  uint iIndirectThree = iIndirectTwo + bnpb * bnpb * bnpb;

  uint nu, *pin = getInode(in, &nu), changed = 0;

  // pin[iDirect+2] may be 0
  // pin[iDirect+1] may be 0

  if (nu >= iIndirectThree) ; // beyond capacity!
  else if (nu >= iIndirectTwo)
    changed = setTripleIndirect(&pin[iDirect+2], nu - iIndirectTwo , bn);
  else if (nu >= iIndirectOne)
    changed = setDoubleIndirect(&pin[iDirect+1], nu - iIndirectOne, bn);
  else if (nu >= iDirect)
    changed = setSingleIndirect(&pin[iDirect], nu - iDirect, bn);
  else {
    if (bn == 0) fv->fbvBlocks.setBit(pin[nu], 1);
    pin[nu] = bn;		// nu < iDirect bn has the data
    changed = (bn > 0? 1 : 2);
  }
  if (changed > 0) putInode(in);
  //printf("%d\n",nu);
  return 1;
}

uint Inodes::addBlockNumber(uint in, uint bn)
{
  return
    in == 0 || in >= fv->superBlock.nInodes
    || bn == 0 || bn >= fv->superBlock.nTotalBlocks
    ? 0
    : setLastBlockNumber(in, bn);
}

/* pre:: 0 <= yth < block-numbers-per-block;; post:: From the single
 * indirect block numbered bn, obtain the xth entry.;;
 */

uint Inodes::getBlockNumberSingleIndirect(uint bn, uint nth)
{ 
  uint bsz=fv->superBlock.nBytesPerBlock;
  uint * buf = new uint[bsz]; 
  fv->readBlock(bn,buf);
  return (buf[nth]);
}

uint Inodes::getBlockNumberDoubleIndirect(uint bn, uint nth)
{
  uint bsz=fv->superBlock.nBytesPerBlock;
  uint * buf = new uint[bsz];
  uint * buf1 = new uint[bsz];

    fv->readBlock(bn,buf);
   // printf("nth=%d\n",nth);
    uint i=nth/(bsz/4);
    //printf("i=%d\n",i);
    fv->readBlock(buf[i],buf1);
    //printf("%d\n",buf1[nth]);
    return(buf1[nth%(bsz/4)]);
}

uint Inodes::getBlockNumberTripleIndirect(uint bn, uint nth)
{
  return TODO("Inodes::getBlockNumberTripleIndirect");
}

/* pre:: inode numbered in is in-use;; post:: Return the n-th block
 * number of file associated with inode numbered in. */

uint Inodes::getBlockNumber(uint in, uint nth)
{
  uint nthmax, *pin = getInode(in, &nthmax);
  if (nth >= nthmax)
    return 0;

  uint iDirect = fv->superBlock.iDirect;
  if (nth < iDirect)
    return pin[nth];

  uint bnpb = fv->superBlock.nBytesPerBlock / fv->superBlock.iWidth;
  uint iIndirectOne = iDirect + bnpb;
  if (nth < iIndirectOne)
    return getBlockNumberSingleIndirect(pin[iDirect], nth - iDirect);

  uint iIndirectTwo = iIndirectOne + bnpb * bnpb;
  if (nth < iIndirectTwo)
    return getBlockNumberDoubleIndirect(pin[iDirect+1], nth - iIndirectOne);

  uint iIndirectThree = iIndirectTwo + bnpb * bnpb * bnpb;
  if (nth < iIndirectThree)
    return getBlockNumberTripleIndirect(pin[iDirect+2], nth - iIndirectTwo);

  return 0;
}

/* pre:: inode numbered in is in-use;; post:: Release the inode in and
 * all the blocks of file associated with inode in, and update the
 * disk copy of this inode. ;; */

uint Inodes::setFree(uint in)
{
  uint nu = 0;
  getInode(in, &nu);
  for (uint i = nu; i > 0;) {
    uint bn = getBlockNumber(in, --i);
    fv->fbvBlocks.setBit(bn, 1);
  }
  fv->fbvInodes.setBit(in, 1);
  return nu;
}

/* pre:: inode numbered in is in-use;; post:: Print out the contents
 * of inode numbered in a readble manner. */

uint Inodes::show(uint in)
{
  uint *pin = getInode(in, 0), x = 0;
  printf("inode #%d == [", in);
  for (; x < xFileSize; x++) {
    printf(" %d:%d", x, pin[x]);
  }
  printf(" size=%d", pin[x]);
  printf(" type=%d", pin[x-1]);
  printf(" count=%d]\n", pin[x-2]);
  return 1;
}

// -eof-
