/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Object.h"

namespace m8r {

class Base64 : public ObjectFactory {
public:
    Base64(Program*, ObjectFactory* parent);

    static int encode(size_t in_len, const unsigned char *in, size_t out_len, char *out);
    static int decode(size_t in_len, const char *in, size_t out_len, unsigned char *out);

private:
    static CallReturnValue encodeFunc(ExecutionUnit*, Value thisValue, uint32_t nparams);
    static CallReturnValue decodeFunc(ExecutionUnit*, Value thisValue, uint32_t nparams);
};
    
}
