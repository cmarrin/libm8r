
/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Application.h"

static constexpr const char* WebServerRoot = "/sys/bin";
static m8r::Duration MainTaskSleepDuration = 10ms;

class Sample : public m8r::Executable
{
public:
    virtual m8r::CallReturnValue execute() override
    {
        print("***** Hello Native World!!!\n");
        return m8r::CallReturnValue(m8r::CallReturnValue::Type::Finished);
    }
};

void m8rmain()
{
    m8r::Application application(m8r::Application::HeartbeatType::Status, WebServerRoot, 23);
 
     // Upload files needed by web server
    m8r::Application::uploadFiles({ "web/index.html", "web/favicon.ico" }, WebServerRoot);

    application.runAutostartTask(m8r::SharedPtr<Sample>(new Sample()));

    while(1) {
        application.runOneIteration();
        MainTaskSleepDuration.sleep();
    }
}
