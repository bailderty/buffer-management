#include "buffer.h"
#include <stdio.h>

class BufMgr
{
    private:
        badgerdb::FrameId clockHand; // clock hand for clock alg
        badgerdb::BufHashTbl *hashTable; // hash table mapping (File, page) to framed number
        badgerdb::BufDesc *bufDescTabe; // BufDesc objects, one per frame
        std::uint32_t numBufs; // Numer of frames in the buffer pool
        badgerdb::BufStats bufStats; // Statistics about buffer pool usage



    void allocBuf(badgerdb::FrameId & frame);
    void advanceClock(); //Advance clock to next frame in the buffer pool

    public:
       badgerdb::Page *bufPool;   // actual buffer pool

       BufMgr(std::uint32_t bufs); // Constructor
       ~BufMgr(); // Destructor
      
       void readPage(badgerdb::File* file, const badgerdb::PageId PageNo, badgerdb::Page*& page);
       void unPinPage(badgerdb::File* file, const badgerdb::PageId PageNo, const bool dirty);
       void allocPage(badgerdb::File* file, badgerdb::PageId& PageNo, badgerdb::Page*& page);
       void disposePage(badgerdb::File* file, const badgerdb::PageId pageNo);
       void flushFile(const badgerdb::File* file);


};
