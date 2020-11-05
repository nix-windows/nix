#pragma once

#include "lock.hh"
#include "store-api.hh"
#include "goal.hh"

namespace nix {

class Worker;

class SubstitutionGoal : public Goal
{
    friend class Worker;

private:
    /* The store path that should be realised through a substitute. */
    StorePath storePath;

    /* The path the substituter refers to the path as. This will be
     * different when the stores have different names. */
    std::optional<StorePath> subPath;

    /* The remaining substituters. */
    std::list<ref<Store>> subs;

    /* The current substituter. */
    std::shared_ptr<Store> sub;

    /* Whether a substituter failed. */
    bool substituterFailed = false;

    /* Path info returned by the substituter's query info operation. */
    std::shared_ptr<const ValidPathInfo> info;

    /* Pipe for the substituter's standard output. */
#ifndef _WIN32
    Pipe outPipe;
#else
    AsyncPipe outPipe;
#endif
    /* The substituter thread. */
    std::thread thr;

    std::promise<void> promise;

    /* Whether to try to repair a valid path. */
    RepairFlag repair;

    /* Location where we're downloading the substitute.  Differs from
       storePath when doing a repair. */
    Path destPath;

    std::unique_ptr<MaintainCount<uint64_t>> maintainExpectedSubstitutions,
        maintainRunningSubstitutions, maintainExpectedNar, maintainExpectedDownload;

    typedef void (SubstitutionGoal::*GoalState)();
    GoalState state;

    /* Content address for recomputing store path */
    std::optional<ContentAddress> ca;

public:
    SubstitutionGoal(const StorePath & storePath, Worker & worker, RepairFlag repair = NoRepair, std::optional<ContentAddress> ca = std::nullopt);
    ~SubstitutionGoal();

    void timedOut(Error && ex) override { abort(); };

    string key() override
    {
        /* "a$" ensures substitution goals happen before derivation
           goals. */
        return "a$" + std::string(storePath.name()) + "$" + worker.store.printStorePath(storePath);
    }

    void work() override;

    /* The states. */
    void init();
    void tryNext();
    void gotInfo();
    void referencesValid();
    void tryToRun();
    void finished();

    /* Callback used by the worker to write to the log. */
#ifndef _WIN32
    void handleChildOutput(int fd, const string & data) override;
    virtual void handleEOF(int fd) override;
#else
    void handleChildOutput(HANDLE handle, const string & data) override;
    virtual void handleEOF(HANDLE handle) override;
#endif

    StorePath getStorePath() { return storePath; }
};

}
