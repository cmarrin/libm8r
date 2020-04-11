/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "MFS.h"
#include "Containers.h"

namespace m8r {

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Stream
//
//
//
//////////////////////////////////////////////////////////////////////////////

class Stream {
public:
    virtual int read() const = 0;
    virtual int write(uint8_t) = 0;
	
private:
};

//////////////////////////////////////////////////////////////////////////////
//
//  Class: FileStream
//
//
//
//////////////////////////////////////////////////////////////////////////////

class FileStream : public m8r::Stream {
public:
	FileStream(Mad<File> file)
        : _file(file)
    { }
    
    ~FileStream() { }

    bool loaded()
    {
        return _file.valid() && _file->valid();
    }
    virtual int read() const override
    {
        if (!_file.valid()) {
            return -1;
        }
        char c;
        if (_file->read(&c, 1) != 1) {
            return -1;
        }
        return static_cast<uint8_t>(c);
    }
    virtual int write(uint8_t c) override
    {
        if (!_file.valid()) {
            return -1;
        }
        if (_file->write(reinterpret_cast<char*>(&c), 1) != 1) {
            return -1;
        }
        return c;
    }
	
private:
    Mad<File> _file;
};

//////////////////////////////////////////////////////////////////////////////
//
//  Class: StringStream
//
//  This class can take either a String or const char*. If it's a String
//  you can write (append) or read. Otherwise you can just read.
//
//////////////////////////////////////////////////////////////////////////////

class StringStream : public m8r::Stream {
public:
    StringStream() : _s(nullptr), _isString(false) { }
	StringStream(const String& s) : _string(s), _isString(true) { }
	StringStream(const char* s) : _s(s), _isString(false) { }
    
    virtual ~StringStream() { }
	
    bool loaded() { return true; }
    
    virtual int read() const override
    {
        if (_isString) {
            return (_index < _string.size()) ? _string[_index++] : -1;
        } else {
            if (!_s) {
                return -1;
            }
            return (_s[_index] == '\0') ? -1 : _s[_index++];
        }
    }
    
    virtual int write(uint8_t c) override
    {
        if (!_isString) {
            return -1;
        }
        
        // Only allow writing to the end of the string
        if (_index != _string.size()) {
            return -1;
        }
        _string += c;
        _index++;
        return c;
    }
	
private:
    union {
        String _string;
        const char* _s;
    };
    bool _isString = false;
    mutable uint32_t _index = 0;
};

//////////////////////////////////////////////////////////////////////////////
//
//  Class: VectorStream
//
//
//
//////////////////////////////////////////////////////////////////////////////

class VectorStream : public m8r::Stream {
public:
	VectorStream() : _index(0) { }
	VectorStream(const Vector<uint8_t>& vector) : _vector(vector), _index(0) { }
    
    virtual ~VectorStream() { }
	
    bool loaded() { return true; }
    virtual int read() const override
    {
        return (_index < _vector.size()) ? _vector[_index++] : -1;
    }
    virtual int write(uint8_t c) override
    {
        // Only allow writing to the end of the vector
        if (_index != _vector.size()) {
            return -1;
        }
        _vector.push_back(c);
        _index++;
        return c;
    }
    
    void swap(Vector<uint8_t>& vector) { std::swap(vector, _vector); }
	
private:
    Vector<uint8_t> _vector;
    mutable uint32_t _index;
};

}