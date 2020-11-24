/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "SystemInterface.h"
#include "Task.h"
#include "Terminal.h"

namespace m8r {

class Error;
class FS;
class HTTPServer;

class Application {
public:
    enum class HeartbeatType { None, Constant, Status };
    
    Application(HeartbeatType = HeartbeatType::None, const char* webServerRoot = nullptr, uint16_t shellPort = 0);
    
    ~Application();
    
    void runAutostartTask(const SharedPtr<Executable>& exec);
    void runAutostartTask(const char* filename);
    bool runOneIteration();
    
    void receivedData(const String& data, KeyAction action)
    {
        if (_autostartTask) {
            _autostartTask->receivedData(data, action);
        }
    }

    enum class NameValidationType { Ok, BadLength, InvalidChar };
    NameValidationType validateBonjourName(const char* name);

    bool mountFileSystem();
    
    static void uploadFiles(const Vector<const char*>& files, const char* destPath);
    
    static SystemInterface* system() { assert(_system); return _system; }

private:
    void runAutostartTaskHelper(const SharedPtr<Task>&);
    void startNetworkServers();
    
    SharedPtr<Task> _autostartTask;
    std::unique_ptr<Terminal> _terminal;
    std::unique_ptr<HTTPServer> _webServer;

    String _webServerRoot;
    uint16_t _shellPort = 0;
    
    static SystemInterface* _system;
};
    
}
