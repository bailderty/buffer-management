/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#include <memory>
#include <iostream>
#include "buffer.h"
#include "exceptions/buffer_exceeded_exception.h"
#include "exceptions/page_not_pinned_exception.h"
#include "exceptions/page_pinned_exception.h"
#include "exceptions/bad_buffer_exception.h"
#include "exceptions/hash_not_found_exception.h"

namespace badgerdb {
    
    BufMgr::BufMgr(std::uint32_t bufs) : numBufs(bufs) {
        bufDescTable = new BufDesc[bufs];
        
        for (FrameId i = 0; i < bufs; i++)
        {
            bufDescTable[i].frameNo = i;
            bufDescTable[i].valid = false;
        }
        
        bufPool = new Page[bufs];
        
        int htsize = ((((int) (bufs * 1.2))*2)/2)+1;
        hashTable = new BufHashTbl (htsize);  // allocate the buffer hash table
        
        clockHand = bufs - 1;
    }
    
    
    BufMgr::~BufMgr() {
    }
    
    void BufMgr::advanceClock()
    {
        clockHand = (clockHand + 1) % numBufs;
    }
    
    void BufMgr::allocBuf(FrameId & frame)
    {
        //variables
        bool frameFreed = false;
        int count = 0;
        
        // the frame hasn't been set and not all frames are pinned
        while(frameFreed == false && count < numBufs)
        {
            BufMgr::advanceClock();
            //Valid bit
            if (bufDescTable[clockHand].valid == true)
            {
                //Ref bit
                if (bufDescTable[clockHand].refbit == true)
                {
                    bufDescTable[clockHand].refbit = false;
                }
                else
                {
                    //pinned
                    if (bufDescTable[clockHand].pinCnt != 0)
                    {
                        count++;
                    }
                    else
                    {
                        //dirty bit
                        if (bufDescTable[clockHand].dirty == true)
                        {
                            //flush page to disk
                            bufDescTable[clockHand].file->writePage(bufPool[clockHand]);
                        }
                        //remove frame from hashtable
                        hashTable->remove(bufDescTable[clockHand].file, bufDescTable[clockHand].pageNo);
                        bufDescTable[clockHand].Clear();
                        frameFreed = true;
                        return;
                    }
                }
            }
            else
            {
                //remove frame from hashtable
                hashTable->remove(bufDescTable[clockHand].file, bufDescTable[clockHand].pageNo);
                bufDescTable[clockHand].Clear();
                frameFreed = true;
                return;
            }
        }
        //if all pages are pinned throw excepton
        if(count >= numBufs && frameFreed == false)
        {
            throw BufferExceededException();
        }
    }
    
    void BufMgr::readPage(File* file, const PageId pageNo, Page*& page)
    {
        
    }
    
    
    void BufMgr::unPinPage(File* file, const PageId pageNo, const bool dirty)
    {
        
    }
    
    void BufMgr::flushFile(const File* file)
    {
        
    }
    
    void BufMgr::allocPage(File* file, PageId &pageNo, Page*& page)
    {
        
    }
    
    void BufMgr::disposePage(File* file, const PageId PageNo)
    {
        
    }
    
    void BufMgr::printSelf(void)
    {
        BufDesc* tmpbuf;
        int validFrames = 0;
        
        for (std::uint32_t i = 0; i < numBufs; i++)
        {
            tmpbuf = &(bufDescTable[i]);
            std::cout << "FrameNo:" << i << " ";
            tmpbuf->Print();
            
            if (tmpbuf->valid == true)
                validFrames++;
        }
        
        std::cout << "Total Number of Valid Frames:" << validFrames << "\n";
    }
    
}
