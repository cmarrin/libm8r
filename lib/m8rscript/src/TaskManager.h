/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Containers.h"
#include "Timer.h"
#include <cstdint>
#include <memory>

namespace m8r {

class Task;

class TaskManager {
public:
    using FinishCallback = std::function<void(Task*)>;
    
    TaskManager();
    ~TaskManager();
    
    bool runOneIteration();

    void run(const std::shared_ptr<Task>&, FinishCallback);
    void terminate(const std::shared_ptr<Task>&);

    void readyToExecuteNextTask();

    void addTimer(Timer*);
    void removeTimer(Timer*);
    
protected:
    static constexpr uint8_t MaxTasks = 8;

private:
    void startTimeSliceTimer();
    void stopTimeSliceTimer();
    void requestYield();
    
    void restartTimer();
    
    Vector<std::shared_ptr<Task>> _list;
    Vector<Timer*> _timerList;
    int8_t _timerId = -1;
    
    std::shared_ptr<Task> _currentTask;

    std::shared_ptr<Timer> _timeSliceTimer;
    bool _terminating = false;
};

}
