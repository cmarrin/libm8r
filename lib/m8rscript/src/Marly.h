/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Atom.h"
#include "Containers.h"
#include "Float.h"
#include "MString.h"

/*
Marly:
    In the descriptions below TOS is to the right. So "X Y Z" is Z on TOS.

Literals:
    bool, int (32 bit), float (32 bit), String, Char, List, Object
    
Operators:
    false:      -> false
                Pushes the bool value false.
                
    true:       -> true
                Pushes the bool value true.
    
    <int>:      -> <int>
                Pushes <int>.
                
    <float>:    -> <float>
                Pushes <float>.

    "<chars>"   -> <string>
                Pushes <string>.
                
    '<char>'    -> <char>
                Pushes <char>.

    [ A0..An ]  -> [..]
                Pushes list.
                
    { <id> : <value> ... } -> { .. }
                Pushes an Object (AA)
                
    @<id>       X ->
                Store X at <id>.
                
    $<id>       -> X
                Push value at <id>.
    
    .<id>       X -> Y
                Push property <id> of X
                
    :<id>       X Y ->
                Store X in property <id> of Object Y
                
    <id>        .. -> ..
                Execute named function.
    
    new         X Y -> Z
                Use values in list X as params to Object Y and create a new instance of Y

    exec        [..] -> ..
                execute list on TOS

    pack        .. n -> [..]
                Makes n stack items into a list.

    unpack      [A0..An] -> A0 .. An
                Places each element from the list onto the stack.

    dup         X -> X X
                Pushes an extra copy of X onto stack.

    swap        X Y -> Y X
                Interchanges X and Y on top of the stack.
                
    pick        A0..Ai..An i -> A0..An Ai
                Remove the nth item on the stack and push it
                
    tuck        A0..A(i-1) Ai..An X i -> A0..A(i-1) X ai..An
                Insert X n locations down the stack

    pop         X ->
                Removes X from top of the stack.

    join        [ X ] [ Y ] -> [X Y]
                Combine 2 lists into one. 
                
    cat         X Y -> "XY"
                Concatenate X and Y. Can be any types which are converted to strings 
    
    remove      [ A0..Ai..An ] i -> [ A0..An ] Ai
                Remove value at index i and push it.

    insert      [ A0..A(i-1) Ai..An ] X i -> [ A0..A(i-1) X ai..An ]
                Insert X before A(i) in the list

    bor         X Y -> Z
                Z is the bitwise or of ints X and Y.

    bxor        X Y -> Z
                Z is the bitwise exclusive or of ints X and Y.

    band        X Y -> Z
                Z is the bitwise and of ints X and Y.

    bnot        X -> Y
                Y is the inverse of int X.
                
    or          X Y -> B
                B is true if either X or Y are true, false otherwise.
    
    and         X Y -> B
                B is true if both X and Y are true, false otherwise.
                
    not         X -> B
                B is true if X is false, false otherwise.
                
    neg         X -> Y
                Negate (2 complement) X
    
    lt          X Y -> B
                B = X < Y

    le          X Y -> B
                B = X <= Y

    eq          X Y -> B
                B = X == Y

    ne          X Y -> B
                B = X != Y

    ge          X Y -> B
                B = X >= Y

    gt          X Y -> B
                B = X > Y

    +           X Y -> Z
                Z = X + Y. Numbers can be int or float.

    -           X Y -> Z
                Z = X - Y. Numbers can be int or float.

    *           X Y -> Z
                Z = X times Y. Numbers can be int or float.

    /           X Y -> Z
                Z = X divided by Y. Numbers can be int or float.

    %           X Y -> Z
                Z = X modulo Y. Numbers can be int or float.

    inc         M -> N
                Increment M by 1.

    dec         M -> N
                Decrement M by 1.
                
    if          B [T] ->
                If B is true execute T othereise skip.
    
    ifte        B [T] [F] ->
                If B is true execute T otherwise execute F.
                
    while       B X ->
                Execute B. If it is true execute X and repeat.
                
    for         S B I X ->
                Execute B. If true execute X then I. Repeat

    fold        A V0 [P] -> V
                Starting with value V0, push each member of A and execute P to produce value V.
                
    map         A [P] -> B
                Executes P on each member of aggregate A, collects results in aggregate B.
                
    filter      A [B] -> A1
                Execute B on each element of A. If true that element is added to list A1

    import      "S" -> O
                import package S, pushing O which contains elements of S
*/

namespace m8r {

class Atom;
class Stream;

class Marly {
public:
    using Verb = std::function<void()>;
    Marly(const Stream&);
    
    const char* stringFromAtom(Atom atom) const { return _atomTable.stringFromAtom(atom); }
private:
    class SharedPtrBase;
    class Value;
    
    using ValueMap = Map<Atom, Value>;
    using ValueVector = Vector<Value>;

    class Shared
    {
    public:
        friend class Marly::SharedPtrBase;
        friend class Marly::Value;
        
    private:
        uint16_t _count = 0;
    };
    
    class SharedPtrBase
    {
    public:
        uint16_t& count(Shared* ptr) { return ptr->_count; }
    };
    
    template<typename T>
    class SharedPtr : private SharedPtrBase
    {
    public:
        explicit SharedPtr(T* p = nullptr) { reset(p); }
        
        SharedPtr(const SharedPtr<T>& other) { reset(other); }
        SharedPtr(SharedPtr<T>&& other) { _ptr = other._ptr; other._ptr = nullptr; }
        
        ~SharedPtr() { reset(); }
        
        SharedPtr& operator=(const SharedPtr& other) { reset(other._ptr); return *this; }
        SharedPtr& operator=(SharedPtr&& other) { _ptr = other._ptr; other._ptr = nullptr; return *this; }

        void reset(T* p = nullptr)
        {
            if (_ptr) {
                assert(count(_ptr) > 0);
                if (--count(_ptr) == 0) {
                    delete _ptr;
                    _ptr = nullptr;
                }
            }
            if (p) {
                count(p)++;
                _ptr = p;
            }
        }

        void reset(const SharedPtr<T>& p) { reset(p.get()); }
        
        T& operator*() { return *_ptr; }
        T* operator->() { return _ptr; }
        
        T* get() { return _ptr; }
        const T* get() const { return _ptr; }
        
        operator bool() { return _ptr != nullptr; }
    
    private:
        T* _ptr = nullptr;
    };
    
    class ObjectBase : public Shared
    {
    public:
        virtual Value property(Atom) const { return Value(); }
        virtual void setProperty(Atom, const Value&) { }
    };

    class Object : public ObjectBase, public ValueMap
    {
    public:
        virtual Value property(Atom) const override { return Value(); }
        virtual void setProperty(Atom, const Value&) override { }
    };

    class List : public ObjectBase, public ValueVector
    {
    public:
        virtual Value property(Atom) const override { return Value(); }
        virtual void setProperty(Atom, const Value&) override { }
    };

    class String : public ObjectBase, public m8r::String
    {
    public:
        String(const char* s) { *this += s; }

        virtual Value property(Atom) const override { return Value(); }
        virtual void setProperty(Atom, const Value&) override { }
    };

    class Value
    {
    public:
        enum class Type : uint8_t {
            // Built-in verbs are first so they can
            // have the same values as the shared atoms
            print = int(SA::print),
            cat = int(SA::cat),

            Verb = ExternalAtomOffset,
            Bool, Null, Undefined, 
            Int, Float, Char,
            String, List, Object,
            
            // Built-in operators
            Load, Store, LoadProp, StoreProp
        };
        
        Value() { _type = Type::Undefined; _int = 0; }
        Value(bool b) { _type = Type::Bool; _bool = b; }
        Value(Type t) { _type = t; }
        Value(const char* s)
        {
            _type = Type::String;
            String* str = new String(s);
            str->_count++;
            _ptr = str;
        }
        
        Value(SharedPtr<List>& list) { setValue(Type::List, list.get()); }
        Value(List* list) { setValue(Type::List, list); }
        Value(SharedPtr<String>& list) { setValue(Type::String, list.get()); }
        Value(String* list) { setValue(Type::String, list); }
        Value(SharedPtr<Object>& list) { setValue(Type::Object, list.get()); }
        Value(Object* list) { setValue(Type::Object, list); }
        
        Value(int32_t i, Type type = Type::Int)
        {
            _type = type;
            switch(type) {
                case Type::Bool: _bool = i != 0; break;
                case Type::Verb:
                case Type::Int: _int = i; 
                case Type::Float: _float = Float(i).raw(); break;
                case Type::Char: _char = i; break;
                default: assert(0); _type = Type::Undefined; 
            }
        }
        
        Type type() const {return _type; }
        
        const char* string() const
        {
            switch(_type) {
                case Type::String: return reinterpret_cast<String*>(_ptr)->c_str();
                default: return "** Unimplemented **";
            }
        }
        
        ObjectBase* obj() const
        {
            assert(_type == Type::List || _type == Type::Object || _type == Type::String);
            return reinterpret_cast<ObjectBase*>(_ptr);
        }
        
        int32_t integer() const { return _int; }
        
    private:
        void setValue(Type type, ObjectBase* obj)
        {
            _type = type;
            obj->_count++;
            _ptr = obj;
        }
        
        Type _type;
        union {
            bool _bool;
            int32_t _int;
            Float::value_type _float;
            char _char;
            void* _ptr;
        };
    };

    //static_assert(sizeof(Value) == sizeof(intptr_t * 3), "Size of Value must be 2 pointers");
    
    void executeCode();
    
    ValueMap _vars;
    Stack<Value> _stack;
    Stack<SharedPtr<List>> _codeStack;
    AtomTable _atomTable;
    Map<Atom, Verb> _verbs;
};    

}