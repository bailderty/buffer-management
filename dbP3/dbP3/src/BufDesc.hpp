//
//  BufDesc.hpp
//  dbP3
//
//  Created by Brett Meyer on 3/19/16.
//  Copyright © 2016 BDM. All rights reserved.
//

#ifndef BufDesc_hpp
#define BufDesc_hpp

#include <stdio.h>
#include "buffer.h"

class BufDesc {
    friend class BufMgr ;
    private:
    badgerdb::File * file; // pointer to file object
    badgerdb::PageId pageNo; // page wi thin file
    badgerdb::FrameId frameNo; //buffer pool frame number
    int pinCnt; // number of times this page has been pinned
    bool dirty; // true if dirty; false otherwise
    bool valid; // true if page is valid
    bool refbit; // true if this buffer frame been referenced recently
    void Clear(); // initialize buffer frame
    void Set(badgerdb::File * filePtr, badgerdb::PageId pageNum); //set BufDesc member variable values
    void Print(); //Print values of member variables
    BufDesc(); //Constructor
} ;
#endif /* BufDesc_hpp */
