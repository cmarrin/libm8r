/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Timer.h"

#include "Application.h"
#include "GC.h"
#include "MStream.h"
#include "Parser.h"

#include <cassert>

using namespace m8r;

void Timer::start()
{
    yield(_duration);
}

void Timer::stop()
{
    terminate();
}

static StaticObject::StaticFunctionProperty RODATA2_ATTR _functionProps[] =
{
    { SA::constructor, TimerProto::constructor },
    { SA::start, TimerProto::start },
    { SA::stop, TimerProto::stop },
};

static StaticObject::StaticProperty _props[] =
{
    { SA::Once, Value(static_cast<int32_t>(Timer::Behavior::Once)) },
    { SA::Repeating, Value(static_cast<int32_t>(Timer::Behavior::Repeating)) },
};

TimerProto::TimerProto()
{
    setProperties(_functionProps, sizeof(_functionProps) / sizeof(StaticFunctionProperty));
    setProperties(_props, sizeof(_props) / sizeof(StaticProperty));
}

CallReturnValue TimerProto::constructor(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    // duration, {Once/Repeating}, callback
    //          or
    // duration, callback
    
    if (nparams < 2 || nparams > 3) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }

    Mad<Object> obj = thisValue.asObject();
    if (!obj.valid()) {
        return CallReturnValue(CallReturnValue::Error::MissingThis);
    }
    
    Duration duration(eu->stack().top(1 - nparams).toFloatValue(eu));
    
    bool repeating = false;
    Value func;
    
    if (nparams > 2) {
        repeating = eu->stack().top(2 - nparams).toBoolValue(eu);
        func = eu->stack().top(3 - nparams);
    } else {
        func = eu->stack().top(2 - nparams);
    }
    
    // Store func so it doesn't get gc'ed
    thisValue.setProperty(eu, Atom(SA::__object), func, Value::SetPropertyType::AddIfNeeded);
    
    Mad<Timer> timer = Mad<Timer>::create();
    obj->setProperty(Atom(SA::__nativeObject), Value::asValue(timer), Value::SetPropertyType::AlwaysAdd);

    timer->init(duration, repeating ? Timer::Behavior::Repeating : Timer::Behavior::Once, [timer, eu, func](TaskBase* task)
    {
        if (func) {
            Value arg(static_cast<int32_t>(task->error().code()));
            eu->fireEvent(func, Value(), &arg, 1);
            if (timer->repeating()) {
                timer->start();
            }
        }
    });

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue TimerProto::start(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams != 0) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    Mad<Timer> timer;
    CallReturnValue retval = getNative(timer, eu, thisValue);
    if (retval.error() != CallReturnValue::Error::Ok) {
        return retval;
    }

printf("***** Timer start:%p\n", timer.get());
    timer->start();

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}

CallReturnValue TimerProto::stop(ExecutionUnit* eu, Value thisValue, uint32_t nparams)
{
    if (nparams != 0) {
        return CallReturnValue(CallReturnValue::Error::WrongNumberOfParams);
    }
    
    Mad<Timer> timer;
    CallReturnValue retval = getNative(timer, eu, thisValue);
    if (retval.error() != CallReturnValue::Error::Ok) {
        return retval;
    }
    
    timer->stop();

    return CallReturnValue(CallReturnValue::Type::ReturnCount, 0);
}