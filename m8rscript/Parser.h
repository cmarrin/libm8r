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

#include "ExecutionUnit.h"
#include "MStream.h"
#include "Scanner.h"
#include "Program.h"
#include "Array.h"

namespace m8r {

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Parser
//
//  
//
//////////////////////////////////////////////////////////////////////////////

class Parser  {
    friend class ParseEngine;
    friend class ParseStack;
    
public:
	Parser();
    
    ~Parser()
    {
    }
    
    void parse(m8r::Stream* stream);
    
	void printError(const char* s);
    void expectedError(Token token);
    void unknownError(Token token);

    uint32_t nerrors() const { return _nerrors; }
    Program* program() { return _program; }
    
    m8r::String stringFromAtom(const Atom& atom) const { return _program->stringFromAtom(atom); }
    Atom atomizeString(const char* s) const { return _program->atomizeString(s); }

    StringLiteral startString() { return _program->startStringLiteral(); }
    void addToString(char c) { _program->addToStringLiteral(c); }
    void endString() { _program->endStringLiteral(); }
    
private:    
    // The next 3 functions work together:
    //
    // Label has a current location which is filled in by the label() call,
    // and a match location which is filled in by the addMatchedJump() function.
    // addMatchedJump also adds the passed Op (which can be JMP, JT or JF)
    // with an empty jump address, to be filled in my matchJump().
    // 
    // When matchJump() is called it adds a JMP to the current location in
    // the Label and then fixed up the match location with the location just
    // past the JMP
    //
    Label label();
    void addMatchedJump(Op op, Label&);
    void matchJump(const Label& matchLabel)
    {
        int32_t jumpAddr = static_cast<int32_t>(currentFunction()->code()->size()) - matchLabel.matchedAddr;
        doMatchJump(matchLabel.matchedAddr, jumpAddr);
    }

    void matchJump(const Label& matchLabel, const Label& dstLabel)
    {
        int32_t jumpAddr = dstLabel.label - matchLabel.matchedAddr;
        doMatchJump(matchLabel.matchedAddr, jumpAddr);
    }
    
    void matchJump(const Label& matchLabel, int32_t dstAddr)
    {
        int32_t jumpAddr = dstAddr - matchLabel.matchedAddr;
        doMatchJump(matchLabel.matchedAddr, jumpAddr);
    }
    
    void doMatchJump(int32_t matchAddr, int32_t jumpAddr);
    void jumpToLabel(Op op, Label&);
    
    int32_t startDeferred()
    {
        assert(!_deferred);
        _deferred = true;
        _deferredCodeBlocks.push_back(_deferredCode.size());
        return static_cast<int32_t>(_deferredCode.size());
    }
    
    int32_t resumeDeferred()
    {
        assert(!_deferred);
        _deferred = true;
        return static_cast<int32_t>(_deferredCode.size());
    }
    
    void endDeferred() { assert(_deferred); _deferred = false; }
    int32_t emitDeferred();

    void functionAddParam(const Atom& atom);
    void functionStart();
    void functionParamsEnd();
    ObjectId functionEnd();
        
    void pushK(StringLiteral::Raw value);
    void pushK(const char* value);
    void pushK(uint32_t value);
    void pushK(Float value);
    void pushK(bool value);
    void pushK();
    void pushK(ObjectId function);
    
    enum class IdType : uint8_t { MustBeLocal, MightBeLocal, NotLocal };
    void emitId(const Atom& value, IdType);
    void emitId(const char*, IdType);

    void emitDup();
    void emitMove();
    
    enum class DerefType { Prop, Elt };
    uint32_t emitDeref(DerefType);
    void emitAppendElt();
    void emitAppendProp();
    void emitStoProp();
    void emitUnOp(Op op);
    void emitBinOp(Op op);
    void emitCaseTest();
    void emitLoadLit(bool array);
    void emitPush();
    void emitPop();
    void emitEnd();
    
    void addNamedFunction(ObjectId functionId, const Atom& name);
    void emitCallRet(Op value, int32_t thisReg, uint32_t count);
    void addVar(const Atom& name) { currentFunction()->addLocal(name); }
    
    void discardResult() { _parseStack.pop(); }
    
    Function* currentFunction() const { assert(_functions.size()); return _functions.back()._function; }
    
  	Token getToken(Scanner::TokenType& token) { return _scanner.getToken(token); }
    
    void addCode(Instruction);
    
    // Parse Stack manipulation and code generation
    
    void emitCodeRRR(Op, uint32_t ra = 0, uint32_t rb = 0, uint32_t rc = 0);
    void emitCodeRUN(Op, uint32_t rn, uint32_t n);
    void emitCodeRSN(Op, uint32_t rn, int32_t n);
    void emitCodeCall(Op, uint32_t rcall, uint32_t rthis, uint32_t nparams);
    
    void reconcileRegisters(Function*);

    class ParseStack {
    public:
        enum class Type { Unknown, Local, Constant, Register, RefK, PropRef, EltRef };
        
        ParseStack(Parser* parser) : _parser(parser) { }
        
        uint32_t push(Type, uint32_t reg = 0, uint32_t derefReg = 0);
        void pop();
        
        Type topType() const { return empty() ? Type::Unknown : _stack.top()._type; }
        uint32_t topReg() const { return empty() ? 0 : _stack.top()._reg; }
        uint32_t topDerefReg() const { return empty() ? 0 : _stack.top()._derefReg; }
        bool empty() const { return _stack.empty(); }
        void clear() { _stack.clear(); }
        
        uint32_t bake();
        void replaceTop(Type, uint32_t reg = 0, uint32_t derefReg = 0);
        
    private:
        struct Entry {
            
            Entry() { }
            Entry(Type type, uint32_t reg, uint32_t derefReg = 0)
                : _type(type)
                , _reg(reg)
                , _derefReg(derefReg)
            {
                if (_type == Type::Constant || _type == Type::RefK) {
                    _reg += MaxRegister;
                }
            }
            
            Type _type;
            uint32_t _reg;
            uint32_t _derefReg;
        };
        
        Stack<Entry> _stack;
        Parser* _parser;
    };
    
    ParseStack _parseStack;

    struct FunctionEntry {
        FunctionEntry(Function* function) : _function(function) { }
        Function* _function = nullptr;
        uint32_t _nextReg = 255;
        uint32_t _minReg = 256;
    };
        
    std::vector<FunctionEntry> _functions;

    Scanner _scanner;
    Program* _program;
    uint32_t _nerrors = 0;
    std::vector<size_t> _deferredCodeBlocks;
    Code _deferredCode;
    bool _deferred = false;

    static uint32_t _nextLabelId;
};

}
