// **********************************************************************
//
// Copyright (c) 2001
// Mutable Realms, Inc.
// Huntsville, AL, USA
//
// All Rights Reserved
//
// **********************************************************************

#ifndef ICE_PACK_ACTIVATOR_ICE
#define ICE_PACK_ACTIVATOR_ICE

#include <IcePack/ServerManagerF.ice>

module IcePack
{

interface Activator
{
    /**
     *
     * Activate a server and returns the pid of the server or 0 if the
     * server couldn't be activated.
     *
     * @param server The server to activate.
     *
     **/
    int activate(Server* server);
};

};

#endif

