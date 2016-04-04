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
    
    //NOT RIGHT
    BufMgr::~BufMgr()
    {
        std::cout<<"~BufMgr(): line 39\n";
        for (int i = 0; i < numBufs; i++)
        {
            //bit is dirty
            if (bufDescTable[i].dirty == true)
            {
                //flush page to disk
                std::cout<<"~BufMgr(): line 46\n";
                std::cout<<bufPool[i].page_number();
                std::cout<<"~BufMgr(): line 48\n";
                //bufDescTable[i].file->writePage(bufPool[i]);
                bufDescTable[i].dirty = false;
            }
        }
        std::cout<<"~BufMgr(): line 50\n";
        delete bufDescTable;
        delete bufPool;
    }
    
    void BufMgr::advanceClock()
    {
        clockHand = (clockHand + 1) % numBufs;
    }
    
    void BufMgr::allocBuf(FrameId & frame)
    {
        //variables
        bool frameFreed = false;
        int countPinned = 0;
        // the frame hasn't been set and not all frames are pinned
        while(frameFreed == false && countPinned < numBufs)
        {
            BufMgr::advanceClock();
            //Valid bit
            if (bufDescTable[clockHand].valid)
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
                        countPinned++;
                    }
                    else
                    {
                        //dirty bit
                        if (bufDescTable[clockHand].dirty == true)
                        {
                            //flush page to disk
                            bufDescTable[clockHand].file->writePage(bufPool[clockHand]);
                            bufDescTable[clockHand].dirty = false;
                        }
                        frameFreed = true;
                        frame = clockHand;
                        return;
                    }
                }
            }
            else
            {
                frameFreed = true;
                frame = clockHand;
                return;
            }
        }
        //if all pages are pinned throw excepton
        if(countPinned >= numBufs && frameFreed == false)
        {
            throw BufferExceededException();
        }
    }

    void BufMgr::readPage(File* file, const PageId pageNo, Page*& page)
    {
        try {
            hashTable->lookup(file, pageNo, clockHand);
            std::cout<<"readPage: line 124\n";
            //look up was successful
            bufDescTable[clockHand].refbit = true;
            bufDescTable[clockHand].pinCnt = bufDescTable[clockHand].pinCnt + 1;
            page = &bufPool[clockHand];
            
        } catch (HashNotFoundException e) {
            //look up was unsucessful
            std::cout<<"readPage: line 132\n";
            allocBuf(clockHand);
            Page p = file->readPage(pageNo);
            bufPool[clockHand] = p;
            hashTable->insert(file,p.page_number(),clockHand);
            bufDescTable[clockHand].Set(file, p.page_number());
            page = &bufPool[clockHand];
            
        }
    }
    
    
    void BufMgr::unPinPage(File* file, const PageId pageNo, const bool dirty)
    {
        try
        {
            hashTable->lookup(file,pageNo,clockHand);
            if (bufDescTable[clockHand].pinCnt > 0)
            {
                std::cout<<"unPinPage: line 145\n";
                bufDescTable[clockHand].pinCnt = bufDescTable[clockHand].pinCnt - 1;
                if (dirty == true)
                {
                    bufDescTable[clockHand].dirty = true;
                }
            }
            else
            {
                throw PageNotPinnedException(bufDescTable[clockHand].file->filename(), pageNo, clockHand);
            }
        }
        catch (HashNotFoundException e)
        {
            std::cout << "HashNotFoundException() in BufMgr::unPinPage()";
        }
    }
    
    void BufMgr::flushFile(const File* file)
    {
        //interate through bufdesctable
        for (int i = 0; i < numBufs; i++)
        {
            std::cout<<"flushFile: line 170\n";
            //file was found in bufdesctable
            std::cout<<bufDescTable[i].file->filename()<<std::endl;
            std::cout<<file->filename()<<std::endl;
            if (bufDescTable[i].file->filename() == file->filename())
            {
                std::cout<<"flushFile: line 173\n";
                //pinned
                if (bufDescTable[i].pinCnt > 0)
                {
                    throw PagePinnedException(bufDescTable[i].file->filename(), bufDescTable[i].pageNo, bufDescTable[i].frameNo);
                }
                //invalid
                else if (bufDescTable[i].valid == false)
                {
                    throw BadBufferException(bufDescTable[i].frameNo, bufDescTable[i].dirty, bufDescTable[i].valid, bufDescTable[i].refbit);
                }
                else
                {
                    //bit is dirty
                    if (bufDescTable[i].dirty == true)
                    {
                        //flush page to disk
                        bufDescTable[i].file->writePage(bufPool[i]);
                        bufDescTable[i].dirty = false;
                    }
                    //remove frame from hashtable
                    hashTable->remove(bufDescTable[i].file, bufDescTable[i].pageNo);
                    bufDescTable[i].Clear();
                }
            }
        }
    }
    
    void BufMgr::allocPage(File* file, PageId &pageNo, Page*& page)
    {
        Page p = file->allocatePage(); //returns the allocated page
        pageNo = p.page_number();
        BufMgr::allocBuf(clockHand); //returns frameId -> clockHand
        bufPool[clockHand] = p;
        hashTable->insert(file, pageNo, clockHand);
        bufDescTable[clockHand].Set(file, pageNo);
        page = &bufPool[clockHand];
    }
    
    
    void BufMgr::disposePage(File* file, const PageId PageNo)
    {
        //iterate through bufDescTable
        for (int i = 0; i < numBufs; i++)
        {
            //if file and pageNo are found
           if (bufDescTable[i].file->filename() == file->filename() && bufDescTable[i].pageNo == PageNo)
           {
               //delete from bufPool, hashTable, and BufDescTable
               //delete bufPool[i];
               //bufDescTable[i].file->deletePage(bufPool[i]);
               //*(bufPool + i) = nullptr;
               hashTable->remove(bufDescTable[i].file, bufDescTable[i].pageNo);
               bufDescTable[i].Clear();
               file->deletePage(PageNo);

           }
        }
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
