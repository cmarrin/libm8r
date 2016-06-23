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

#include "Parser.h"

#include "ParseEngine.h"

#ifndef USE_PARSE_ENGINE
#define USE_PARSE_ENGINE 1
#endif

using namespace m8r;

#if !USE_PARSE_ENGINE
extern int yyparse(Parser*);
#endif

uint32_t Parser::_nextLabelId = 1;

Parser::Parser(m8r::Stream* istream, void (*printer)(const char*))
    : _scanner(this, istream)
    , _program(new Program(printer))
    , _printer(printer)
{
    _currentFunction = _program->main();

#if USE_PARSE_ENGINE
    ParseEngine p(this);
    p.program();
#else
	yyparse(this);
#endif
}

void Parser::printError(const char* s)
{
    ++_nerrors;
    _printer("Error: ");
    _printer(s);
    _printer(" on line ");
    _printer(Value::toString(_scanner.lineno()).c_str());
    _printer("\n");
}

Label Parser::label()
{
    Label label;
    label.label = static_cast<int32_t>(_currentFunction->code()->size());
    label.uniqueID = _nextLabelId++;
    return label;
}

void Parser::addMatchedJump(Op op, Label& label)
{
    assert(op == Op::JMP || op == Op::JF || op == Op::JF);
    label.matchedAddr = static_cast<int32_t>(_currentFunction->code()->size());
    addCodeByte(op, 1);
    addCodeInt(0, 2);
}

void Parser::matchJump(Label& label)
{
    int32_t jumpAddr = static_cast<int32_t>(_currentFunction->code()->size()) - label.matchedAddr - 1;
    if (jumpAddr < -32767 || jumpAddr > 32767) {
        printError("JUMP ADDRESS TOO BIG TO EXIT LOOP. CODE WILL NOT WORK!\n");
        return;
    }
    _currentFunction->setCodeAtIndex(label.matchedAddr + 1, byteFromInt(jumpAddr, 1));
    _currentFunction->setCodeAtIndex(label.matchedAddr + 2, byteFromInt(jumpAddr, 0));
}

void Parser::jumpToLabel(Op op, Label& label)
{
    assert(op == Op::JMP || op == Op::JF || op == Op::JF);
    int32_t jumpAddr = label.label - static_cast<int32_t>(_currentFunction->code()->size()) - 1;
    if (jumpAddr >= -127 && jumpAddr <= 127) {
        addCodeByte(op);
        addCodeByte(static_cast<uint8_t>(jumpAddr));
    } else {
        if (jumpAddr < -32767 || jumpAddr > 32767) {
            printError("JUMP ADDRESS TOO BIG TO LOOP. CODE WILL NOT WORK!\n");
            return;
        }
        addCodeByte(op, 0x01);
        addCodeInt(jumpAddr, 2);
    }
}

void Parser::addCodeByte(uint8_t c)
{
    if (_deferred) {
        assert(_deferredCodeBlocks.size() > 0);
        _deferredCode.push_back(c);
    } else {
        _currentFunction->addCode(c);
    }
}

void Parser::emit(StringId s)
{
    uint32_t rawStringId = s.rawStringId();
    uint32_t sizeMask;
    if (rawStringId <= 255) {
        sizeMask = 0;
    } else if (rawStringId <= 65535) {
        sizeMask = 1;
    } else {
        sizeMask = 3;
    }
    addCodeByte(static_cast<uint8_t>(Op::PUSHSX) | sizeMask);
    addCodeInt(rawStringId, sizeMask + 1);
}

void Parser::emit(uint32_t value)
{
    uint32_t size;
    uint32_t mask;
    
    if (value <= 15) {
        addCodeByte(Op::PUSHI, value);
        return;
    }
    if (value <= 255) {
        size = 1;
        mask = 0;
    } else if (value <= 65535) {
        size = 2;
        mask = 0x01;
    } else {
        size = 4;
        mask = 0x03;
    }
    addCodeByte(Op::PUSHIX, mask);
    addCodeInt(value, size);
}

void Parser::emit(Float value)
{
    addCodeByte(Op::PUSHF);
    addCodeInt(static_cast<RawFloat>(value).raw(), 4);
}

void Parser::emitId(const Atom& atom, IdType type)
{
    if (type == IdType::MightBeLocal || type == IdType::MustBeLocal) {
        int32_t index = _currentFunction->localIndex(atom);
        if (index < 0 && type == IdType::MustBeLocal) {
            String s = "'";
            s += Program::stringFromAtom(atom);
            s += "' is not a local variable name";
            printError(s.c_str());
        }
        if (index >= 0) {
            if (index <= 15) {
                addCodeByte(Op::PUSHL, index);
            } else {
                assert(index < 65536);
                uint32_t size = (index < 256) ? 1 : 2;
                addCodeByte(Op::PUSHLX, size - 1);
                addCodeInt(index, size);
            }
            return;
        }
    }
        
    addCodeByte(Op::PUSHID);
    addCodeInt(atom.rawAtom(), 2);
}

void Parser::emit(Op value)
{
    addCodeByte(value);
}

void Parser::emit(Object* obj)
{
    addCodeByte(Op::PUSHO);
    addCodeInt(_program->addObject(obj).rawObjectId(), 4);
}

void Parser::addNamedFunction(Function* function, const Atom& name)
{
    assert(name.valid());
    int32_t index = _currentFunction->addProperty(name);
    assert(index >= 0);
    _currentFunction->setProperty(index, function);
}

void Parser::emitWithCount(Op value, uint32_t nparams)
{
    assert(nparams < 256);
    assert(value == Op::CALL || value == Op::NEW || value == Op::RET);
    
    if (nparams <= 15) {
        addCodeByte(value, nparams);
    } else {
        addCodeByte((value == Op::CALL) ? Op::CALLX : ((value == Op::NEW) ? Op::NEWX : Op::RETX));
        addCodeInt(nparams, 1);
    }
}

void Parser::emitDeferred()
{
    assert(!_deferred);
    assert(_deferredCodeBlocks.size() > 0);
    for (size_t i = _deferredCodeBlocks.back(); i < _deferredCode.size(); ++i) {
        _currentFunction->addCode(_deferredCode[i]);
    }
    _deferredCode.resize(_deferredCodeBlocks.back());
    _deferredCodeBlocks.pop_back();
}

void Parser::functionAddParam(const Atom& atom)
{
    if (_currentFunction->addLocal(atom) < 0) {
        m8r::String s = "param '";
        s += _program->stringFromAtom(atom);
        s += "' already exists";
        printError(s.c_str());
    }
}

void Parser::functionStart()
{
    _functions.push_back(_currentFunction);
    _currentFunction = new Function();
}

void Parser::functionParamsEnd()
{
    _currentFunction->markParamEnd();
}

Function* Parser::functionEnd()
{
    assert(_currentFunction && _functions.size());
    Function* function = _currentFunction;
    _currentFunction = _functions.back();
    _functions.pop_back();
    return function;
}

void Parser::programEnd()
{
    emit(Op::END);
}
