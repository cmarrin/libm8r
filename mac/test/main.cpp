/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Application.h"
#include "MacSystemInterface.h"
#include "MFS.h"

class Sample : public m8r::Executable
{
public:
    virtual m8r::CallReturnValue execute() override
    {
        print("***** Hello Native World!!!\n");
        return m8r::CallReturnValue(m8r::CallReturnValue::Type::Finished);
    }
};

int main(int argc, char * argv[])
{
    m8r::initMacSystemInterface("m8rFSFile", [](const char* s) { ::printf("%s", s); });
    m8r::Application application(23);
    application.runAutostartTask(m8r::SharedPtr<Sample>(new Sample()));
    
    while (true) {
        application.runOneIteration();
    }

    return 0;
}
