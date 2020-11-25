/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "JSON.h"

#include "Defines.h"
#include "StringStream.h"
#include "Scanner.h"
#include "SystemInterface.h"

using namespace m8r;

bool JSON::value(Scanner& scanner, SharedPtr<JSON::Value>& v)
{
    switch(scanner.getToken()) {
        case Token::Minus: 
            scanner.retireToken();
            switch(scanner.getToken()) {
                case Token::Float: v = SharedPtr<Value>(new NumberValue(-scanner.getTokenValue().number)); scanner.retireToken(); break;
                case Token::Integer: v = SharedPtr<Value>(new NumberValue(-static_cast<float>(scanner.getTokenValue().integer))); scanner.retireToken(); break;
                default: return false;
            }
            break;
        case Token::Float: v = SharedPtr<Value>(new NumberValue(scanner.getTokenValue().number)); scanner.retireToken(); break;
        case Token::Integer: v = SharedPtr<Value>(new NumberValue(static_cast<float>(scanner.getTokenValue().integer))); scanner.retireToken(); break;
        case Token::String: v = SharedPtr<Value>(new StringValue(scanner.getTokenValue().str)); scanner.retireToken(); break;
        case Token::True: v = SharedPtr<Value>(new BooleanValue(true)); scanner.retireToken(); break;
        case Token::False: v = SharedPtr<Value>(new BooleanValue(false)); scanner.retireToken(); break;
        case Token::Null: v = SharedPtr<Value>(new NullValue()); scanner.retireToken(); break;;
        case Token::LBracket: {
            scanner.retireToken();
            auto array = SharedPtr<ArrayValue>(new ArrayValue());
            SharedPtr<Value> elementValue;
            if (value(scanner, elementValue)) {
                array->array().push_back(elementValue);
                while (scanner.getToken() == Token::Comma) {
                    scanner.retireToken();
                    if (!value(scanner, elementValue)) {
                        _error = Error::Code::RuntimeError;
                        _errorLine = scanner.lineno();
                        return false;
                    }
                    array->array().push_back(elementValue);
                }
            }
            if (scanner.getToken() != Token::RBracket) {
                _error = Error::Code::RuntimeError;
                _errorLine = scanner.lineno();
                return false;
            }
            scanner.retireToken();
            v = array;
            break;
        }
        case Token::LBrace: {
            scanner.retireToken();
            auto object = SharedPtr<ObjectValue>(new ObjectValue());

            String propertyKey;
            SharedPtr<Value> propertyValue;
            if (propertyAssignment(scanner, propertyKey, propertyValue)) {
                object->map().emplace(propertyKey, propertyValue);
                while (scanner.getToken() == Token::Comma) {
                    scanner.retireToken();
                    if (!propertyAssignment(scanner, propertyKey, propertyValue)) {
                        break;
                    }
                    if (_error != Error::Code::OK) {
                        return false;
                    }

                    object->map().emplace(propertyKey, propertyValue);
                }
            }
            if (scanner.getToken() != Token::RBrace) {
                _error = Error::Code::RuntimeError;
                _errorLine = scanner.lineno();
                return false;
            }
            scanner.retireToken();
            v = object;
            break;
        }
        default:
            _error = Error::Code::RuntimeError;
            _errorLine = scanner.lineno();
            return false;
    }
    
    return v;
}

bool JSON::propertyAssignment(Scanner& scanner, String& key, SharedPtr<Value>& v)
{
    if (scanner.getToken() != Token::String) {
        _error = Error::Code::RuntimeError;
        _errorLine = scanner.lineno();
        return false;
    }
    
    key = scanner.getTokenValue().str;
    scanner.retireToken();
    if (scanner.getToken() != Token::Colon) {
        _error = Error::Code::RuntimeError;
        _errorLine = scanner.lineno();
        return false;
    }
    scanner.retireToken();
    return value(scanner, v);
}

bool JSON::parse(const String& json, SharedPtr<Value>& v)
{
    StringStream stream(json);
    Scanner scanner(&stream);
    if (!value(scanner, v)) {
        return false;
    }
    
    if (scanner.getToken() != Token::EndOfFile) {
        _error = Error::Code::RuntimeError;
        _errorLine = scanner.lineno();
        return false;
    }
    return true;
}

String JSON::stringify(const SharedPtr<Value>& v)
{
    return v->toString();
}

String JSON::stringify(const Vector<String>& v)
{
    String s("[ ");
    bool first = true;
    for (auto const& it : v) {
        if (!first) {
            s += ", ";
        }
        first = false;
        s += '"';
        s += it;
        s += '"';
    }
    s += " ]";
    return s;
}

String JSON::stringify(const String& v)
{
    String s;
    s += '"';
    s += v;
    s += '"';
    return s;
}

String JSON::ArrayValue::toString() const
{
    String s("[ ");
    bool first = true;
    for (auto const& it : _value) {
        if (!first) {
            s += ", ";
        }
        first = false;
        s += it->toString();
    }
    s += " ]";
    return s;
}

String JSON::ObjectValue::toString() const
{
    String s("{ ");
    bool first = true;
    for (auto const& it : _value) {
        if (!first) {
            s += ", ";
        }
        first = false;
        
        s += it.key;
        s += " : ";
        s += it.value->toString();
    }
    s += " }";
    return s;
}
