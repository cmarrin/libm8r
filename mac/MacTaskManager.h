/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "TaskManager.h"
#include <condition_variable>
#include <thread>

namespace m8r {

class MacTaskManager : public TaskManager {
public:
    MacTaskManager();
    virtual ~MacTaskManager();
    
    virtual void lock() override { _lockMutex.lock(); }
    virtual void unlock() override { _lockMutex.unlock(); }

private:
    virtual void runLoop() override;
    
    virtual void readyToExecuteNextTask() override;
    
    std::thread* _eventThread = nullptr;
    std::condition_variable _eventCondition;
    std::mutex _eventMutex;
    std::mutex _lockMutex;
    bool _terminating = false;
};

}
