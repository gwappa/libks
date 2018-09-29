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

#ifndef __KS_LOG_H__
#define __KS_LOG_H__

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include "ks/thread.h"

namespace ks {

enum LogMeta {
    endl,
    flush,
};

enum LogLevel {
    Error   = 5,    // fatal error, application will stop functioning well
    Warning = 4,    // not an error, but a sign of it
    Info    = 3,    // just information for a user
    Fine    = 2,    // finer information on what's happening
    Debug   = 1,    // for a debugging purpose
};

class logger;

class LogHandler
{
public:
    LogHandler(const LogLevel &level=Info);
    virtual ~LogHandler();

    virtual void handleLog(logger *msg)=0;

    void seLoggedLevel(const LogLevel &level);
    LogLevel loggedLevel() const;

private:
    LogLevel loggedlevel_;
};

class LogHandlerManager
{
public:
    LogHandlerManager();

    void addHandler(LogHandler *handler); // does not own 'handler' pointer
    void removeHandler(LogHandler *handler); // does not delete 'handler' pointer
    virtual void dispatch(logger *msg); // dispatches 'msg' to all the handlers and then call clean(msg)

protected:
    virtual void clean(logger *msg) = 0; // cleaning up 'msg'

    std::vector<LogHandler *> handlers;
};

class LogService: public LogHandler, public LogHandlerManager
{
public:
    LogService();
    ~LogService();
    logger &get(ks_thread_id id, std::string title, LogLevel level, const bool &autoflush);
    virtual void handleLog(logger *msg);

private:
    static void dispatch_static(LogService *service, logger *msg);
    static void clean(LogService *service, ks_thread_id id);

    void writeTitle(std::ostream &out, const std::string &title) const;
    void writeContent(std::ostream &out, const std::string &text) const;


    virtual void clean(logger *msg);
    void clean(ks_thread_id id);
    void clean();
    std::map<ks_thread_id, std::map<LogLevel, logger *> > pool_;
};

/**
 * @brief The LogPool class -- designed to 'pool' logs until the proper logger is active
 */
class LogPool: public LogHandler, public LogHandlerManager
{
public:
    LogPool(void (*addService)(LogHandler *), void (*removeService)(LogHandler *));
    virtual ~LogPool();
    virtual void handleLog(logger *msg);
    void         dispatchAll(bool unregister); // dispatches all the messages, and (optional) removes from the original service

private:
    static void dispatch_static(LogPool *pool, logger *msg);
    virtual void clean(logger *msg);
    void unregister(); // unregisters only if the instance is still registered to a log service

    bool registered_;
    void (*reg_)(LogHandler *);
    void (*unreg_)(LogHandler *);
    std::vector<logger *> logs_;
};

class logger
{
public:
    static logger &log(std::string title=std::string(), LogLevel level=Info, const bool &autoflush=true);

    static logger &error(std::string title=std::string(), const bool &autoflush=true);
    static logger &warning(std::string title=std::string(), const bool &autoflush=true);
    static logger &info(std::string title=std::string(), const bool &autoflush=true);
    static logger &fine(std::string title=std::string(), const bool &autoflush=true);
    static logger &debug(std::string title=std::string(), const bool &autoflush=true);
    static void setLoggedLevel(const LogLevel &level);

    static void addHandler(LogHandler *handler);
    static void removeHandler(LogHandler *handler);

    logger(ks_thread_id id, std::string title, LogLevel level, const bool &autoflush);
    void        dispatch();
    std::stringstream &buffer();
    std::string &title();
    std::string content();
    LogLevel    &level();
    ks_thread_id &thread();
    const bool        &autoflush() const;

private:
    static LogService service_;
    ks_thread_id  thread_;
    std::string title_;
    LogLevel    level_;
    bool        autoflush_;

    std::stringstream buf_;
};

template<typename T>
logger &operator<<(logger &loggerobj, const T &val){
    loggerobj.buffer() << val;
    return loggerobj;
}

template<>
logger &operator<<(logger &loggerobj, const LogMeta &val);

}

#endif // __KS_LOG_H__
