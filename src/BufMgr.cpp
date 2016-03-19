#include "buffer.h"
#include <stdio.h>

class BufMgr
{
    private:
        FrameId clockHand; // clock hand for clock alg
        BufHashTbl *hashTable; // hash table mapping (File, page) to framed number
        BufDesc *bufDescTabe; // BufDesc objects, one per frame
        std::uint32_t numBufs; // Numer of frames in the buffer pool
        BufStats bufStats; // Statistics about buffer pool usage



    void allocBuf(FrameId & frame);
    void advanceClock(); //Advance clock to next frame in the buffer pool

    public:
       Page *bufPool;   // actual buffer pool

       BufMgr(std::uint32_t bufs); // Constructor
       ~BufMgr(); // Destructor
      
       void readPage(File* file, const PageId PageNo, Page*& page);
       void unPinPage(File* file, const PageId PageNo, const bool dirty);
       void allocPage(File* file, PageId& PageNo, Page*& page);
       void disposePage(File* file, const PageId pageNo);
       void flushFile(const File* file);


};
