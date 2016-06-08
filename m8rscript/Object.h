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

#include <map>

namespace m8r {

class Object;

class Value {
public:
    enum class Type { Object };

    Value(const Value& other)
        : _value(other._value)
        , _type(other._type)
    { }
    
    Value(Object* obj)
        : _value(reinterpret_cast<void*>(obj))
        , _type(Type::Object)
    { }
    
    Type type() const { return _type; }
    Object* object() const { return (_type == Type::Object) ? reinterpret_cast<Object*>(_value) : nullptr; }

private:
    void* _value;
    Type _type;
};

class Object {
public:
    typedef std::map<Atom, Value> ValueMap;
    
    virtual ~Object() { }

    virtual const Atom* name() const { return nullptr; }
    
    virtual bool hasCode() const { return false; }
    virtual uint8_t codeAtIndex(uint32_t index) const { return 0; }
    virtual uint32_t codeSize() const { return 0; }
    virtual String stringFromCode(uint32_t index, uint32_t len) const { return String(); }
    virtual const ValueMap& values() const { return _values; }
    virtual Value* value(const Atom& s) { return &(_values.find(s)->second); }
    virtual void setValue(const Atom& s, const Value& v) { _values.emplace(s, v); }
    
private:
    ValueMap _values;
};
    
}
