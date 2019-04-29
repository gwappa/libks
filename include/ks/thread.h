/*
 * MIT License
 *
 * Copyright (c) 2018-2019 Keisuke Sehara
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __KS_THREAD_H__
#define __KS_THREAD_H__

#include <stdint.h>
#include <map>

#ifdef _WIN32
#include <winsock2.h> // instead of windows.h
#else
#include <pthread.h>
#endif

typedef uint64_t ks_thread_id;

namespace ks {

class Thread;

/**
 * _ThreadService class
 *
 * The global structure to monitor/manage all the threads in the program.
 *
 * Every Thread object is supposed to call put() when the Thread has been initialised
 * and when it is being destroyed.
 * This class also ensures that the main thread can be found from Thread::current(),
 * as the method internally uses _ThreadService::get() call.
 */
class _ThreadService
{
public:
    _ThreadService();
    virtual ~_ThreadService();
    void    put(ks_thread_id tid, Thread *tptr);
    Thread *get(ks_thread_id tid);
private:
    std::map<ks_thread_id, Thread *> pool_;
    Thread *main_;
};

#ifdef _WIN32
typedef HANDLE              ks_thread_handle_t;
typedef CRITICAL_SECTION    ks_mutex_t;
typedef CONDITION_VARIABLE  ks_cond_t;
#else
typedef pthread_t           ks_thread_handle_t;
typedef pthread_mutex_t     ks_mutex_t;
typedef pthread_cond_t      ks_cond_t;
#endif

/**
 * Thread class
 *
 * A tiny wrapper class that is expected to operate cross-platform.
 * (currently only POSIX threads are supported)
 *
 * You are supposed to inherit this class and override run() method,
 * in order to use the threading functionality.
 * You can use Thread::start(), and Thread::join() instance methods
 * as well as Thread::current() and Thread::exit() static methods
 */
class Thread
{
public:
    Thread();
    explicit Thread(ks_thread_handle_t t); // creation from the handle from a Thread that is already running
    virtual ~Thread();
    void start(); // throws std::runtime_error
    void join();

    static Thread *current(); // returns the current thread
    static ks_thread_id id(); // returns the current thread id
    static void exit(int code); // used from within the thread execution
protected:
    virtual void run();
    void exit_(int code);

private:
    static _ThreadService service_;
    void         run_();

    ks_thread_handle_t handle_;
#ifdef _WIN32
    static int   run_static_return(Thread *t); // for WIN32 thread system

    DWORD          id_;
#else
    static void *run_static(Thread *t);        // for POSIX thread system

    pthread_attr_t threadattr_;
    int *exitcoderef_;
#endif
    bool running_;
    int exitcode_;
};

/**
 * @brief The lockableobject class -- a base class for handling any 'lockable' object.
 * actually this class holds the ks_mutex_t object. any subclass can obtain its reference by the protected mutex() method.
 */
class lockableobject
{
public:
    lockableobject();
    explicit lockableobject(lockableobject &ref); // cannot copy
    virtual ~lockableobject();

    void lock();
    bool tryLock(); // returns true if the thread can/does own the lock; false otherwise
    void unlock();

protected:
    ks_mutex_t *mutex();

private:
    ks_mutex_t mutex_;
};

/**
 * @brief The Mutex class -- a wrapper for a lightweight Mutex object
 */
class Mutex: public lockableobject
{
public:
    Mutex();
    explicit Mutex(Mutex &ref); // cannot copy
};


/**
 * @brief The MutexLocker class -- for locking the mutex only within the scope of interest; a bad copy of QMutexLocker
 */
class MutexLocker
{
public:
    explicit MutexLocker(Mutex *ref);
    ~MutexLocker();

private:
    Mutex *ref_;
};

/**
 * @brief The Condition class -- a wrapper for a condition variable, with its associated Mutex object
 */
class Condition: public lockableobject
{
public:
    Condition();
    explicit Condition(Condition &ref); // cannot copy
    virtual ~Condition();

    virtual bool wait(long timeout_msec=-1); // true if condition is signaled
    void notify();
    void notifyAll();

private:
    ks_cond_t  cond_;
};

/**
 *  @brief Flag class -- a class that represents a flag, which remembers its boolean state
 */
class Flag: public Condition
{
public:
    Flag();
    explicit Flag(Flag &ref); // cannot copy

    // you must obtain lock() on this object when calling the following methods
    virtual bool wait(long timeout_msec=-1); // true when the state is/becomes true
    bool isset() { return state_; }
    void set(); // sets the state to true; internally it invokes notifyAll()
    void unset(); // sets the state to false; executes silently i.e. notify*() will not be invoked

private:
    bool        state_;
};

}

#endif // __KS_THREAD_H__
