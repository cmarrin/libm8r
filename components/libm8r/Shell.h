/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Task.h"

namespace m8r {

class Shell : public Executable {
public:
    Shell();
    virtual ~Shell() { }

    virtual CallReturnValue execute() override;

private:
    void showPrompt() const;
    void processLine(const String&);
    
    bool _done = false;
    Vector<String> _lines;
    Map<String, String> _env;
    bool _needPrompt = true;
};
    
}
