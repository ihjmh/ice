// **********************************************************************
//
// Copyright (c) 2001
// MutableRealms, Inc.
// Huntsville, AL, USA
//
// All Rights Reserved
//
// **********************************************************************

#ifndef ICE_BUFFER_H
#define ICE_BUFFER_H

#include <Ice/Config.h>

namespace IceInternal
{

class ICE_API Buffer : public ::IceUtil::noncopyable
{
public:

    Buffer()
    {
	b.reserve(1000);
	i = b.begin();
    }

    typedef std::vector<Ice::Byte> Container;
    Container b;
    Container::iterator i;
};

}

#endif
