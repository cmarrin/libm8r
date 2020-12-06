/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Shell.h"

using namespace m8r;

Shell::Shell()
{
    _env.emplace("PATH", "/sys/bin");
    _env.emplace("CWD", "/home");
    _env.emplace("HOME", "/home/cmarrin");
}

CallReturnValue Shell::execute()
{
    if (_needPrompt) {
        showPrompt();
        _needPrompt = false;
    }
    
    while (_lines.size()) {
        const String& line = _lines.front();
        _lines.pop_front();
        if (line.size()) {
            processLine(line);
        }
        _needPrompt = true;
    }
    
    return _done ? CallReturnValue(CallReturnValue::Type::Finished) :
                   CallReturnValue(CallReturnValue::Type::WaitForEvent);
}

void Shell::showPrompt() const
{
    printf("[%s] > ", _env.find("CWD")->value.c_str());
}

void Shell::processLine(const String&)
{
}
