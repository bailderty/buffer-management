//
//  BufMgr.cpp
//  dbP3
//
//  Created by Brett Meyer on 3/19/16.
//  Copyright © 2016 BDM. All rights reserved.
//

#include <stdio.h>
#include "BufMgr.h"

BufMgr::BufMgr(std::uint32_t bufs)
{
    BufMgr::clockHand = 0;
    BufMgr::numBufs = bufs;
    BufMgr::hashTable = new badgerdb::BufHashTbl(bufs);
    BufMgr::bufStats = * new badgerdb::BufStats();
    //BufMgr::bufDescTable = new badgerdb::BufDesc();
    bufPool = new badgerdb::Page[bufs];
}
