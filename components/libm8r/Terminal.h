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
#include "TCPServer.h"
#include "Telnet.h"

namespace m8r {

class Terminal : public TCPServer {
public:
    Terminal(uint16_t port, CreateTaskFunction);

private:
    Telnet _telnets[TCP::MaxConnections];
};
    
}
