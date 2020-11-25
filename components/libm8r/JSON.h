/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Error.h"
#include "MString.h"
#include "SharedPtr.h"

namespace m8r {

class Scanner;

class JSON : public Shared
{
public:
    class Value : public Shared
    {
    public:
        virtual ~Value() { }
        
        virtual String toString() const = 0;
    };
    
    class StringValue : public Value
    {
    public:
        StringValue(const String& v) : _value(v) { }
        
        virtual String toString() const { return _value; }
    
    private:
        String _value;
    };

    class NumberValue : public Value
    {
    public:
        NumberValue(float v) : _value(v) { }
        
        virtual String toString() const { return String(_value); }
    
    private:
        float _value;
    };

    class ObjectValue : public Value
    {
    public:
        ObjectValue() { }
        
        virtual String toString() const;
    
        Map<String, SharedPtr<Value>>& map() { return _value; }

    private:
        Map<String, SharedPtr<Value>> _value;
    };

    class ArrayValue : public Value
    {
    public:
        ArrayValue() { }
        
        virtual String toString() const;
        
        Vector<SharedPtr<Value>>& array() { return _value; }
    
    private:
        Vector<SharedPtr<Value>> _value;
    };

    class BooleanValue : public Value
    {
    public:
        BooleanValue(bool v) : _value(v) { }
        
        virtual String toString() const { return _value ? "true" : "false"; }
    
    private:
        bool _value;
    };

    class NullValue : public Value
    {
    public:
        NullValue() { }
        
        virtual String toString() const { return "null"; }
    };

    JSON() { }
    
    bool parse(const String& json, SharedPtr<Value>&);
    String stringify(const SharedPtr<Value>&);
    String stringify(const Vector<String>&);
    String stringify(const String&);

private:
    bool value(Scanner&, SharedPtr<Value>&);
    bool propertyAssignment(Scanner&, String& key, SharedPtr<Value>& value);
    
    Error _error = Error::Code::OK;
    uint32_t _errorLine = 0;
};

}
