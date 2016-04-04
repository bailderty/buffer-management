/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 * edited by Brett Meyer 9067702986(bmeyer) and Alex Instefjord 906-376-7918 (instefjo)
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
    /**
     * Constructor of BufMgr class
     */
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
    /**
     * Destructor of BufMgr class
     */
    BufMgr::~BufMgr()
    {   //clear, write, and remove from bufDescTable and hashTable
        for (int i = 0; i < numBufs; i++)
        {
            if (!bufDescTable[clockHand].valid) {
                bufDescTable[clockHand].Clear();
                continue;
            }
            if (bufDescTable[clockHand].dirty) {
                bufDescTable[clockHand].file->writePage(bufPool[clockHand]);
            }
            hashTable->remove(bufDescTable[clockHand].file, bufDescTable[clockHand].pageNo);
            bufDescTable[clockHand].Clear();
        }
        //delete vars
        delete[] bufPool;
        delete[] bufDescTable;
        delete hashTable;
    }
    /**
     * Advance clock to next frame in the buffer pool
     */
    void BufMgr::advanceClock()
    {
        clockHand = (clockHand + 1) % numBufs;
    }
    /**
     * Allocate a free frame.
     *
     * @param frame   	Frame reference, frame ID of allocated frame returned via this variable
     * @throws BufferExceededException If no such buffer is found which can be allocated
     */
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
                        //dealloc
                        hashTable->remove(bufDescTable[clockHand].file, bufDescTable[clockHand].pageNo);
                        bufDescTable[clockHand].Clear();
                        frameFreed = true;
                        frame = clockHand;
                        return;
                    }
                }
            }
            else
            {
                //dealloc
                bufDescTable[clockHand].Clear();
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
    /**
     * Reads the given page from the file into a frame and returns the pointer to page.
     * If the requested page is already present in the buffer pool pointer to that frame is returned
     * otherwise a new frame is allocated from the buffer pool for reading the page.
     *
     * @param file   	File object
     * @param PageNo  Page number in the file to be read
     * @param page  	Reference to page pointer. Used to fetch the Page object in which requested page from file is read in.
     */
    void BufMgr::readPage(File* file, const PageId pageNo, Page*& page)
    {
        try {
            hashTable->lookup(file, pageNo, clockHand);
            //look up was successful
            bufDescTable[clockHand].refbit = true;
            bufDescTable[clockHand].pinCnt = bufDescTable[clockHand].pinCnt + 1;
            page = &bufPool[clockHand];
            
        } catch (HashNotFoundException e) {
            //look up was unsucessful
            allocBuf(clockHand);
            Page p = file->readPage(pageNo);
            bufPool[clockHand] = p;
            hashTable->insert(file,p.page_number(),clockHand);
            bufDescTable[clockHand].Set(file, p.page_number());
            page = &bufPool[clockHand];
            
        }
    }
    
    /**
     * Unpin a page from memory since it is no longer required for it to remain in memory.
     *
     * @param file   	File object
     * @param PageNo  Page number
     * @param dirty		True if the page to be unpinned needs to be marked dirty
     * @throws  PageNotPinnedException If the page is not already pinned
     */

    void BufMgr::unPinPage(File* file, const PageId pageNo, const bool dirty)
    {
        try
        {
            hashTable->lookup(file,pageNo,clockHand);
            if (bufDescTable[clockHand].pinCnt > 0)
            {
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
            //DO NOTHING
            //std::cout << "HashNotFoundException() in BufMgr::unPinPage()";
        }
    }
    /**
     * Writes out all dirty pages of the file to disk.
     * All the frames assigned to the file need to be unpinned from buffer pool before this function can be successfully called.
     * Otherwise Error returned.
     *
     * @param file   	File object
     * @throws  PagePinnedException If any page of the file is pinned in the buffer pool
     * @throws BadBufferException If any frame allocated to the file is found to be invalid
     */
    void BufMgr::flushFile(const File* file)
    {
        //interate through bufdesctable
        for (int i = 0; i < numBufs; i++)
        {
            //file was found in bufdesctable
            if (bufDescTable[i].file->filename() == file->filename())
            {
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
    /**
     * Allocates a new, empty page in the file and returns the Page object.
     * The newly allocated page is also assigned a frame in the buffer pool.
     *
     * @param file   	File object
     * @param PageNo  Page number. The number assigned to the page in the file is returned via this reference.
     * @param page  	Reference to page pointer. The newly allocated in-memory Page object is returned via this reference.
     */
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
    
    /**
     * Delete page from file and also from buffer pool if present.
     * Since the page is entirely deleted from file, its unnecessary to see if the page is dirty.
     *
     * @param file   	File object
     * @param PageNo  Page number
     */

    void BufMgr::disposePage(File* file, const PageId PageNo)
    {
        try
        {
            hashTable->lookup(file,PageNo,clockHand);
            if (!bufDescTable[clockHand].valid) {
                bufDescTable[clockHand].Clear();
            }
            if (bufDescTable[clockHand].dirty) {
                bufDescTable[clockHand].file->writePage(bufPool[clockHand]);
            }
            hashTable->remove(bufDescTable[clockHand].file, bufDescTable[clockHand].pageNo);
            bufDescTable[clockHand].Clear();
            file->deletePage(PageNo);
        }
        catch (HashNotFoundException e)
        {
            //do nothing
        }
    }
    
    /**
     * Print member variable values.
     */
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
