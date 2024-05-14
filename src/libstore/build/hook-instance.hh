#pragma once
///@file

#include "logging.hh"
#include "serialise.hh"
#include "processes.hh"

#ifdef _WIN32
#  include "windows-async-pipe.hh"
#endif

namespace nix {

struct HookInstance
{
    /**
     * Pipes for talking to the build hook.
     */
    Pipe toHook;

    /**
     * Pipe for the hook's standard output/error.
     */
#ifndef _WIN32
    Pipe
#else
    windows::AsyncPipe
#endif
        fromHook;

    /**
     * Pipe for the builder's standard output/error.
     */
#ifndef _WIN32
    Pipe
#else
    windows::AsyncPipe
#endif
        builderOut;

    /**
     * The process ID of the hook.
     */
    Pid pid;

    FdSink sink;

    std::map<ActivityId, Activity> activities;

    HookInstance(
#ifdef _WIN32
        HANDLE ioport
#endif
        );

    ~HookInstance();
};

}
