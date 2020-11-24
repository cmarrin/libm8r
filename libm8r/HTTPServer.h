/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "MString.h"
#include <cstdint>

namespace m8r {

// HTTPServer
//
// Build on top of TCP class
// Adapted from https://github.com/konteck/wpp

class TCP;

class HTTPServer {
public:
    class Request;
    
    enum class Method { ANY, GET, PUT, POST, DELETE };
    using RequestFunction = std::function<String(const String& uri, const String& suffix, const Request&, int16_t connectionId)>;
    
    // Parsed HTTP Request
    //
    //  First line examples:
    //      GET /api/network/get_ssid_list HTTP/1.1                - Response is JSON list of available ssids
    //      POST /api/network/set_ssid?ssid=foo&pwd=bar HTTP/1.1   - Set ssid to name and password sent
    //
    struct Request
    {
        HTTPServer::Method method;      // First line: GET, POST, etc.
        String path;                    // First line: path without any params
        Map<String, String> params;     // First line: key/value pairs of params
        Map<String, String> headers;    // Next lines: each header as key/value pair 
        bool valid = false;
        
        String toString() const
        {
            String s("Method: ");
            switch(method) {
                case Method::ANY:       s += "ANY"; break;
                case Method::GET:       s += "GET"; break;
                case Method::PUT:       s += "PUT"; break;
                case Method::POST:      s += "POST"; break;
                case Method::DELETE:    s += "DELETE"; break;
            }
            s += "\nPath:'";
            s += path;
            s += "\nParams: { ";
            bool first = true;
            for (auto it : params) {
                if (!first) {
                    s += ", ";
                }
                first = false;
                s += String::format("'%s':'%s'", it.key.c_str(), it.value.c_str());
            }
            s += " }\n";
            return s;
        }
    };
    
    HTTPServer(uint16_t port, const char* rootDir, bool dirAccess = true);
    ~HTTPServer() { }

    void handleEvents();
    
    void on(const String& uri, RequestFunction);
    void on(const String& uri, const String& path, bool dirAccess = true);

private:
    static String dateString();
        
    void sendResponseHeader(int16_t connectionId, uint32_t size);
    
    struct RequestHandler
    {
        RequestHandler() { }
        
        RequestHandler(const String& uri, RequestFunction f)
            : _uri(uri)
            , _requestHandler(f)
        { }
        
        String _uri;
        RequestFunction _requestHandler;
    };
    
    Vector<RequestHandler> _requestHandlers;
    
    Mad<TCP> _socket;
    String _rootDir;
    bool _dirAccess;
};
    
}
