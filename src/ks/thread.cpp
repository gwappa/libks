/*
 * MIT License
 * 
 * Copyright (c) 2018 Keisuke Sehara
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

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <errno.h>

#include "ks/thread.h"
#include "ks/log.h"

namespace ks {


/**
  * function ks_thread_id gettid(__native__ thread): convert from native thread ID to ks_thread_id
  */
#ifdef _WIN32
inline ks_thread_id gettid(HANDLE thread) {
    return (ks_thread_id)(GetThreadId(thread));
}
#else
inline ks_thread_id gettid(pthread_t ptid) {
    ks_thread_id threadId = 0;
    memcpy(&threadId, &ptid, std::min(sizeof(threadId), sizeof(ptid)));
    return threadId;
}
#endif

_ThreadService::_ThreadService(): pool_()
{
#ifdef _WIN32
    HANDLE main_t = GetCurrentThread();
#else
    pthread_t main_t = pthread_self();
#endif
    main_ = new Thread(main_t);
    pool_[gettid(main_t)] = main_;
}

_ThreadService::~_ThreadService()
{
    //ks::logger::info("threadtest") << "destroying _ThreadService" << ks::endl;

    delete main_;
    std::cerr << "done finalizing _ThreadService" << std::endl;
}

void _ThreadService::put(ks_thread_id tid, Thread *tptr)
{
    if( tptr == 0 ){
        pool_.erase(tid);
    } else {
        pool_[tid] = tptr;
    }
}

Thread *_ThreadService::get(ks_thread_id tid)
{
    std::map<ks_thread_id, Thread *>::iterator it = pool_.find(tid);
    if( it == pool_.end() ){
        return 0;
    } else {
        // returns the first occurrence
        return it->second;
    }
}

_ThreadService Thread::service_;


#ifdef _WIN32
Thread::Thread(): running_(false), exitcode_(0)
{
    handle_ = CreateThread(
                NULL,   // LPSECURITY_ATTRIBUTES lpThreadAttributes
                0,      // DWORD dwStackSize
                reinterpret_cast<LPTHREAD_START_ROUTINE>(Thread::run_static_return),      // LPTHREAD_START_ROUTINE lpStartAddress
                static_cast<LPVOID>(this),   // LPVOID lpParameter
                CREATE_SUSPENDED, // DWORD dwCreationFlags
                &id_    // LPDWORD lpThreadId
                );
}

#else
Thread::Thread(): handle_(0), exitcoderef_(0), running_(false), exitcode_(0)
{
    pthread_attr_init(&threadattr_);
    pthread_attr_setdetachstate(&threadattr_, PTHREAD_CREATE_JOINABLE);
}
#endif

Thread::Thread(ks_thread_handle_t t): handle_(t), running_(true), exitcode_(0)
{
    // do nothing
    // threadattr_ will not be used
#ifndef _WIN32
    exitcoderef_ = 0;
#endif
}

Thread::~Thread()
{
    if( running_ == true ){
        service_.put(gettid(handle_), 0);
    }
#ifdef _WIN32
    CloseHandle(handle_);
#else
    pthread_attr_destroy(&threadattr_);
#endif
}

void Thread::start()
{
    if( running_ == true ){
        return;
    }

    bool fail = false;
#ifdef _WIN32
    fail = ( ResumeThread(handle_) == -1 );
#else
    fail = ( pthread_create(&handle_, &threadattr_, reinterpret_cast<void *(*)(void *)>(&Thread::run_static), static_cast<void *>(this))
             != 0 );
#endif
    if( fail == true )
    {
        throw new std::runtime_error("Failed to create a Thread");
    } else {
        running_ = true;
        service_.put(gettid(handle_), this);
    }
}

#ifdef _WIN32
int   Thread::run_static_return(Thread *t)
{
    t->run_();
    return t->exitcode_;
}
#else
void *Thread::run_static(Thread *t)
{
    t->run_();
    return static_cast<void *>( &(t->exitcode_) );
}
#endif

void Thread::run_()
{
    this->run();
    Thread::exit(0);
}

void Thread::run()
{
    // do nothing
}

void Thread::join()
{
#ifdef _WIN32
    long rc = WaitForSingleObject(handle_, INFINITE);
#else
    int rc = pthread_join(handle_, reinterpret_cast<void **>(&exitcoderef_));
#endif
    if( rc ){
        std::stringstream ss;
        ss << "Could not join: pthread_join returned code " << rc;
        throw new std::runtime_error(ss.str());
    }
}

void Thread::exit_(int code)
{
    exitcode_ = code;
    service_.put(gettid(handle_), 0);
    running_ = false;
#ifdef _WIN32
    ExitThread(code);
#else
    pthread_exit(static_cast<void *>(&exitcode_));
#endif
}

// static
Thread *Thread::current()
{
    return service_.get(id());
}

// static
ks_thread_id Thread::id()
{
#ifdef _WIN32
    return gettid(GetCurrentThread());
#else
    return gettid(pthread_self());
#endif
}

// static
void Thread::exit(int code)
{
    current()->exit_(code);
}


lockableobject::lockableobject()
{
#ifdef _WIN32
    InitializeCriticalSectionAndSpinCount(&mutex_, 0x400); // try setting spin count this way for now
#else
    if( pthread_mutex_init(&mutex_, 0) ) // try setting mutexatttr this way for now
    {
        std::stringstream ss;
        ss << strerror(errno);
        ks::logger::error("coud not initialize mutex") << ss.str() << ks::endl;
        throw std::runtime_error(ss.str());
    }
#endif
}

lockableobject::~lockableobject()
{
#ifdef _WIN32
    DeleteCriticalSection(&mutex_);
#else
    if( pthread_mutex_destroy(&mutex_) )
    {
        ks::logger::warning("ks::Lockable") << "the following error occurred on pthread_mutex_destroy, but was ignored: "
                                                   << strerror(errno) << ks::endl;
    }
#endif
}

void lockableobject::lock()
{
#ifdef _WIN32
    EnterCriticalSection(&mutex_);
#else
    if( pthread_mutex_lock(&mutex_) )
    {
        std::stringstream ss;
        ss << strerror(errno);
        ks::logger::error("could not acquire mutex lock") << ss.str() << ks::endl;
        throw std::runtime_error(ss.str());
    }
#endif
}

bool lockableobject::tryLock()
{
#ifdef _WIN32
    return (TryEnterCriticalSection(&mutex_) != 0);
#else
    switch( pthread_mutex_trylock(&mutex_) )
    {
    case 0:
        // successful
        return true;
    case EBUSY:
        // already locked by another thread
        return false;
    case EINVAL:
    default:
        // invalid mutex
        std::stringstream ss;
        ss << strerror(errno);
        ks::logger::error("Lockable::tryLock failed") << ss.str();
        throw std::runtime_error(ss.str());
    }
#endif
}

void lockableobject::unlock()
{
#ifdef _WIN32
    LeaveCriticalSection(&mutex_);
#else
    if( pthread_mutex_unlock(&mutex_) )
    {
        std::stringstream ss;
        ss << strerror(errno);
        ks::logger::error("could not release lock") << ss.str() << ks::endl;
        throw std::runtime_error(ss.str());
    }
#endif
}

ks_mutex_t *lockableobject::mutex() { return &mutex_; }

const long million = 1000000;
const long billion = 1000000000;

Mutex::Mutex(): lockableobject() { }

MutexLocker::MutexLocker(Mutex *ref): ref_(ref)
{
    ref_->lock();
}

MutexLocker::~MutexLocker()
{
    ref_->unlock();
}

Condition::Condition(): lockableobject()
{
#ifdef _WIN32
    InitializeConditionVariable(&cond_);
#else
    if( pthread_cond_init(&cond_, 0) )
    {
        std::stringstream ss;
        ss << strerror(errno);
        ks::logger::error("could not initialize condition variable") << ss.str() << ks::endl;
        throw std::runtime_error(ss.str());
    }
#endif
}

Condition::~Condition()
{
#ifdef _WIN32
    // seems to need nothing for condition variable
#else
    if( pthread_cond_destroy(&cond_) )
    {
        ks::logger::warning("ks::thread module") << "the following error occurred on pthread_cond_destroy, but was ignored: "
                                                   << strerror(errno) << ks::endl;
    }
#endif
}

bool Condition::wait(long timeout_msec)
{
    bool ret;
#ifdef _WIN32
    ret = (SleepConditionVariableCS(&cond_, mutex(), (timeout_msec>=0)? timeout_msec: INFINITE) != 0);
#else
    int err;
    if( timeout_msec < 0 ){
        err = pthread_cond_wait(&cond_, mutex());
    } else {
        struct timespec timeout;
        timeout.tv_nsec = (timeout_msec * million) % billion;
        timeout.tv_sec = time(0) + ( timeout_msec - timeout.tv_nsec );
        err = pthread_cond_timedwait(&cond_, mutex(), &timeout);
    }
    if( err ){
        std::stringstream ss;
        ss << strerror(errno);
        ks::logger::error("pthread_cond_wait failed") << ss.str() << ks::endl;
        throw std::runtime_error(ss.str());
    }
    ret = (err == 0);
#endif
    return ret;
}

void Condition::notify()
{
#ifdef _WIN32
    WakeConditionVariable(&cond_);
#else
    if( pthread_cond_signal(&cond_) )
    {
        std::stringstream ss;
        ss << strerror(errno);
        ks::logger::error("pthread_cond_signal failed") << ss.str() << ks::endl;
        throw std::runtime_error(ss.str());
    }
#endif
}

void Condition::notifyAll()
{
#ifdef _WIN32
    WakeAllConditionVariable(&cond_);
#else
    if( pthread_cond_broadcast(&cond_) )
    {
        std::stringstream ss;
        ss << strerror(errno);
        ks::logger::error("pthread_cond_broadcast failed") << ss.str() << ks::endl;
        throw std::runtime_error(ss.str());
    }
#endif
}

Flag::Flag(): Condition(), state_(false) {}

bool Flag::wait(long timeout_msec)
{
    if( state_ )
        return true;

    return Condition::wait(timeout_msec);
}

void Flag::set()
{
    if( !state_ ){
        state_ = true;
        notifyAll();
    }
}

void Flag::unset()
{
    if( state_ ){
        state_ = false;
    }
}

}
