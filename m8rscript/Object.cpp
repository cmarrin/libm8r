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

#include "Object.h"

#include "ExecutionUnit.h"
#include "Function.h"
#include "MStream.h"
#include "Program.h"

using namespace m8r;

Object::IdStore<StringId, String> Object::_stringStore;
Object::IdStore<ObjectId, Object> Object::_objectStore;
std::vector<ObjectId> Object::_staticObjects;

void Object::_gcMark(ExecutionUnit* eu)
{
    gcMark(eu, this);
    gcMark(eu, _proto);
}

void Object::gc(ExecutionUnit* eu)
{
    _stringStore.gcClear();
    _objectStore.gcClear();
    
    // Mark string 0 (the dummy string)
    _stringStore.gcMark(StringId(0));
    
    eu->gcMark();
    
    for (auto it = _staticObjects.begin(); it != _staticObjects.end(); ++it) {
        _objectStore.gcMark(*it);
    }
    
    _stringStore.gcSweep();
    _objectStore.gcSweep();
}

void Object::gcMark(ExecutionUnit* eu, Object* object)
{
    ObjectId id = object ? object->objectId() : ObjectId();
    if (id && !_objectStore.isGCMarked(id)) {
        _objectStore.gcMark(id);
        assert(obj(id));
        obj(id)->gcMark(eu);
    }
}

void Object::gcMark(ExecutionUnit* eu, const Value& value)
{
    StringId stringId = value.asStringIdValue();
    if (stringId) {
        _stringStore.gcMark(stringId);
        return;
    }
    
    gcMark(eu, value.asObjectId());
}

ObjectId Object::addObject(Object* obj, bool collectable)
{
    assert(!obj->objectId());
    obj->setCollectable(collectable);
    ObjectId id = _objectStore.add(obj);
    obj->setObjectId(id);
    return id;
}

void Object::removeObject(ObjectId id)
{
    if (_objectStore.ptr(id) == nullptr) {
        return;
    }
    assert(_objectStore.ptr(id)->objectId() == id);
    _objectStore.remove(id, false);
}

StringId Object::createString(const char* s, int32_t length)
{
    return _stringStore.add(new String(s, length));
}

StringId Object::createString(const String& s)
{
    return _stringStore.add(new String(s));
}

bool Object::serializeBuffer(Stream* stream, Error& error, ObjectDataType type, const uint8_t* buffer, size_t size) const
{
    if (!serializeWrite(stream, error, type)) {
        return error.setError(Error::Code::Write);
    }
    assert(size < 65536);
    uint16_t ssize = static_cast<uint16_t>(size);
    if (!serializeWrite(stream, error, ssize)) {
        return error.setError(Error::Code::Write);
    }
    for (uint16_t i = 0; i < ssize; ++i) {
        if (!serializeWrite(stream, error, buffer[i])) {
            return error.setError(Error::Code::Write);
        }
    }
    return true;
}

bool Object::deserializeBufferSize(Stream* stream, Error& error, ObjectDataType expectedType, uint16_t& size) const
{
    ObjectDataType type;
    if (!deserializeRead(stream, error, type) || type != expectedType) {
        return error.setError(Error::Code::Read);
    }
    if (!deserializeRead(stream, error, size)) {
        return error.setError(Error::Code::Read);
    }
    return true;
}

bool Object::deserializeBuffer(Stream* stream, Error& error, uint8_t* buffer, uint16_t size) const
{
    for (uint16_t i = 0; i < size; ++i) {
        if (!deserializeRead(stream, error, buffer[i])) {
            return error.setError(Error::Code::Read);
        }
    }
    return true;
}

bool Object::serializeWrite(Stream* stream, Error& error, ObjectDataType value) const
{
    uint8_t c = static_cast<uint8_t>(value);
    if (stream->write(c) != c) {
        return error.setError(Error::Code::Write);
    }
    return true;
}

bool Object::serializeWrite(Stream* stream, Error& error, uint8_t value) const
{
    if (stream->write(value) != value) {
        return error.setError(Error::Code::Write);
    }
    return true;
}

bool Object::serializeWrite(Stream* stream, Error& error, uint16_t value) const
{
    uint8_t c = static_cast<uint8_t>(value >> 8);
    if (stream->write(c) != c) {
        return error.setError(Error::Code::Write);
    }
    c = static_cast<uint8_t>(value);
    if (stream->write(c) != c) {
        return error.setError(Error::Code::Write);
    }
    return true;
}

bool Object::deserializeRead(Stream* stream, Error& error, ObjectDataType& value) const
{
    int c = stream->read();
    if (c < 0) {
        return error.setError(Error::Code::Read);
    }
    value = static_cast<ObjectDataType>(c);
    return true;
}

bool Object::deserializeRead(Stream* stream, Error& error, uint8_t& value) const
{
    int c = stream->read();
    if (c < 0) {
        return error.setError(Error::Code::Read);
    }
    value = static_cast<uint8_t>(c);
    return true;
}

bool Object::deserializeRead(Stream* stream, Error& error, uint16_t& value) const
{
    int c = stream->read();
    if (c < 0) {
        return error.setError(Error::Code::Read);
    }
    value = (static_cast<uint16_t>(c) & 0xff) << 8;
    c = stream->read();
    if (c < 0) {
        return error.setError(Error::Code::Read);
    }
    value |= static_cast<uint16_t>(c) & 0xff;
    return true;
}

bool Object::serializeObject(Stream* stream, Error& error, Program* program) const
{
    if (!serializeWrite(stream, error, ObjectDataType::Type)) {
        return error.setError(Error::Code::Write);
    }
    if (!serializeWrite(stream, error, static_cast<uint8_t>('m'))) {
        return error.setError(Error::Code::Write);
    }
    if (!serializeWrite(stream, error, static_cast<uint8_t>('8'))) {
        return error.setError(Error::Code::Write);
    }
    if (!serializeWrite(stream, error, static_cast<uint8_t>('r'))) {
        return error.setError(Error::Code::Write);
    }
    
    if (!serializeWrite(stream, error, ObjectDataType::Version)) {
        return error.setError(Error::Code::Write);
    }
    if (!serializeWrite(stream, error, MajorVersion)) {
        return error.setError(Error::Code::Write);
    }
    if (!serializeWrite(stream, error, MinorVersion)) {
        return error.setError(Error::Code::Write);
    }

    if (!serialize(stream, error, program)) {
        return false;
    }

    if (!serializeWrite(stream, error, ObjectDataType::End)) {
        return error.setError(Error::Code::Write);
    }
    return true;
}

bool Object::deserializeObject(Stream* stream, Error& error, Program* program, const AtomTable& atomTable, const std::vector<char>& stringTable)
{
    ObjectDataType type;
    if (!deserializeRead(stream, error, type)) {
        return false;
    }
    if (type != ObjectDataType::Type) {
        return error.setError(Error::Code::SerialHeader);
    }
    
    uint8_t c;
    if (!deserializeRead(stream, error, c)) {
        return false;
    }
    if (c != 'm') {
        return error.setError(Error::Code::SerialHeader);
    }
    if (!deserializeRead(stream, error, c)) {
        return false;
    }
    if (c != '8') {
        return error.setError(Error::Code::SerialHeader);
    }
    if (!deserializeRead(stream, error, c)) {
        return false;
    }
    if (c != 'r') {
        return error.setError(Error::Code::SerialHeader);
    }
    
    if (!deserializeRead(stream, error, type)) {
        return false;
    }
    if (type != ObjectDataType::Version) {
        return error.setError(Error::Code::SerialType);
    }
    
    uint8_t majorVersion, minorVersion;
    if (!deserializeRead(stream, error, majorVersion)) {
        return false;
    }
    if (majorVersion != MajorVersion) {
        return error.setError(Error::Code::SerialVersion);
    }
    if (!deserializeRead(stream, error, minorVersion)) {
        return false;
    }
    if (minorVersion != MinorVersion) {
        return error.setError(Error::Code::SerialVersion);
    }

    if (!deserialize(stream, error, program, atomTable, stringTable)) {
        return false;
    }
    
    if (!deserializeRead(stream, error, type)) {
        return false;
    }
    if (type != ObjectDataType::End) {
        return error.setError(Error::Code::SerialType);
    }
    return true;
}

MaterObject::MaterObject()
{
    Object::addObject(this, true);
}

MaterObject::~MaterObject()
{
    auto it = _properties.find(ATOM(__nativeObject));
    if (it != _properties.end()) {
        NativeObject* obj = it->value.asNativeObject();
        if (obj) {
            delete obj;
        }
    }
    removeNoncollectableObjects();
}

String MaterObject::toString(ExecutionUnit* eu, bool typeOnly) const
{
    String typeName = eu->program()->stringFromAtom(property(eu, ATOM(__typeName)).asIdValue());
    
    if (typeOnly) {
        return typeName.empty() ? String("Object") : typeName;
    }
    
    // FIXME: Pretty print
    String s = "{ ";
    bool first = true;
    for (auto prop : _properties) {
        if (!first) {
            s += ", ";
        } else {
            first = false;
        }
        s += eu->program()->stringFromAtom(prop.key);
        s += " : ";
        
        // Avoid loops
        if (prop.value.isObject()) {
            s += Object::toString(eu);
        } else {
            s += prop.value.toStringValue(eu);
        }
    }
    s += " }";
    return s;
}

void MaterObject::gcMark(ExecutionUnit* eu)
{
    Object::gcMark(eu);
    for (auto entry : _properties) {
        entry.value.gcMark(eu);
    }
    auto it = _properties.find(ATOM(__nativeObject));
    if (it != _properties.end()) {
        NativeObject* obj = it->value.asNativeObject();
        if (obj) {
            obj->gcMark(eu);
        }
    }
}

CallReturnValue MaterObject::callProperty(ExecutionUnit* eu, Atom prop, uint32_t nparams)
{
    Value callee = property(eu, prop);
    return callee.call(eu, Value(objectId()), nparams, false);
}

const Value MaterObject::property(ExecutionUnit* eu, const Atom& prop) const
{
    auto it = _properties.find(prop);
    if (it == _properties.end()) {
        return proto() ? proto()->property(eu, prop) : Value();
    }
    return it->value;
}

bool MaterObject::setProperty(ExecutionUnit* eu, const Atom& prop, const Value& v, Value::SetPropertyType type)
{
    Value oldValue = property(nullptr, prop);
    
    if (oldValue && type == Value::SetPropertyType::AlwaysAdd) {
        return false;
    }
    if (!oldValue && type == Value::SetPropertyType::NeverAdd) {
        return false;
    }
    
    auto it = _properties.find(prop);
    if (it == _properties.end()) {
        auto ret = _properties.emplace(prop, Value());
        assert(ret.second);
        ret.first->value = v;
        return true;
    }
    
    it->value = v;
    return true;
}

CallReturnValue MaterObject::call(ExecutionUnit* eu, Value thisValue, uint32_t nparams, bool ctor)
{
    if (!ctor) {
        // FIXME: Do we want to handle calling an object as a function, like JavaScript does?
        return CallReturnValue(CallReturnValue::Type::Error);
    }
    
    MaterObject* obj = new MaterObject();
    Value objectValue(obj);
    obj->setProto(objectId());

    auto it = _properties.find(ATOM(constructor));
    if (it != _properties.end()) {
        CallReturnValue retval = it->value.call(eu, objectValue, nparams, true);
        if (!retval.isReturnCount() || retval.returnCount() > 0) {
            return retval;
        }
    }
    eu->stack().push(objectValue);
    return CallReturnValue(CallReturnValue::Type::ReturnCount, 1);
}

void MaterObject::removeNoncollectableObjects()
{
    for (auto it : _properties) {
        Object* obj = it.value.asObjectId();
        if (obj && !obj->collectable()) {
            Object::removeObject(it.value.asObjectId());
        }
    }
}

NativeFunction::NativeFunction(Func func)
    : _func(func)
{
    addObject(this, true);
}

ObjectFactory::ObjectFactory(Program* program, const char* name)
{
    _obj._collectable = false;
    if (name) {
        _obj.setProperty(nullptr, ATOM(__typeName), Value(program->atomizeString(name)), Value::SetPropertyType::AlwaysAdd);
    }
}

ObjectFactory::~ObjectFactory()
{
    Object::removeObject(_obj.objectId());
}

void ObjectFactory::addProperty(Program* program, Atom prop, Object* obj)
{
    assert(obj && obj->objectId());
    obj->_collectable = false;
    addProperty(program, prop, Value(obj));
}

void ObjectFactory::addProperty(Program* program, Atom prop, const Value& value)
{
    _obj.setProperty(nullptr, prop, value, Value::SetPropertyType::AlwaysAdd);
}

ObjectId ObjectFactory::create(Atom objectName, ExecutionUnit* eu, uint32_t nparams)
{
    Value objectValue = eu->program()->global()->property(eu, objectName);
    if (!objectValue) {
        return ObjectId();
    }
    
    Object* object = objectValue.asObjectId();
    if (!object) {
        return ObjectId();
    }

    CallReturnValue r = object->call(eu, Value(), nparams, true);
    Value retValue;
    if (r.isReturnCount() && r.returnCount() > 0) {
        retValue = eu->stack().top(1 - r.returnCount());
        eu->stack().pop(r.returnCount());
    }    
    return retValue.asObjectId();
}
