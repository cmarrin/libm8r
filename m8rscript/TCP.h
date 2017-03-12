/*-------------------------------------------------------------------------
This source file is a part of m8rscript

For the latest info, see http://www.marrin.org/

Copyright (c) 2016, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.
    
    - Redistributions in binary form must reproduce the above copyright 
    notice, this list of conditions and the following disclaimer in the 
    documentation and/or other materials provided with the distribution.
    
    - Neither the name of the <ORGANIZATION> nor the names of its 
    contributors may be used to endorse or promote products derived from 
    this software without specific prior written permission.
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

#pragma once

#include "TaskManager.h"
#include "IPAddr.h"
#include "UDP.h"
#include <cstdint>
#include <cstring>

namespace m8r {

class TCP;

class TCPDelegate {
public:
    enum class Event { Connected, Reconnected, Disconnected, ReceivedData, SentData };
    
    virtual void TCPevent(TCP*, Event, int16_t connectionId, const char* data = nullptr, uint16_t length = 0) = 0;
};

class TCP {
    friend class MyEventTask;
    
public:
    static constexpr int MaxConnections = 4;
    static constexpr uint32_t DefaultTimeout = 7200;
     
    static TCP* create(TCPDelegate*, uint16_t port);
    static TCP* create(TCPDelegate*, uint16_t port, IPAddr ip);
    virtual ~TCP() { }
    
    virtual void send(int16_t connectionId, char c) = 0;
    virtual void send(int16_t connectionId, const char* data, uint16_t length = 0) = 0;
    virtual void disconnect(int16_t connectionId) = 0;

protected:
    TCP(TCPDelegate* delegate, uint16_t port, IPAddr ip = IPAddr()) : _delegate(delegate), _ip(ip), _port(port) { }

    TCPDelegate* _delegate;
    IPAddr _ip;
    uint16_t _port;
};

}
