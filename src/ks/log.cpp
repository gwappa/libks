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
#include <iostream>
#include <algorithm>
#include "ks/log.h"
#include "ks/utils.h"

//#define DEBUG_KS_LOG

namespace ks {

LogHandler::LogHandler(const LogLevel &level): loggedlevel_(level) {}
LogHandler::~LogHandler() {}

void LogHandler::seLoggedLevel(const LogLevel &level)
{
    loggedlevel_ = level;
}

LogLevel LogHandler::loggedLevel() const
{
    return loggedlevel_;
}

LogHandlerManager::LogHandlerManager() {}

void LogHandlerManager::addHandler(LogHandler *handler)
{
    handlers.push_back(handler);
}

void LogHandlerManager::removeHandler(LogHandler *handler)
{
    handlers.erase(std::find(handlers.begin(), handlers.end(), handler));
}

void LogHandlerManager::dispatch(logger *msg)
{
#ifdef DEBUG_KS_LOG
    // std::cerr << "LogService::dispatch" << std::endl;
#endif
    std::vector<LogHandler *>::iterator it;
    LogHandler *h;
    for(it=handlers.begin(); it!=handlers.end(); it++){
        h = static_cast<LogHandler *>(*it);
        h->handleLog(msg);
    }
    this->clean(msg);
}

LogService::LogService(): LogHandler(Info), LogHandlerManager()
{
    addHandler(this);
}

LogService::~LogService()
{
    removeHandler(this);
#ifdef DEBUG_KS_LOG
    std::cerr << "cleaning up LogService..." << std::endl;
#endif
    clean();
#ifdef DEBUG_KS_LOG
    std::cerr << "LogService successfully finalized" << std::endl;
#endif
}

void LogService::handleLog(logger *msg)
{
    if( msg->level() >= loggedLevel() ){
        if( msg->level() >= Warning ){
            std::cerr << "***";
            writeTitle(std::cerr, msg->title());
            writeContent(std::cerr, msg->buffer().str());
        } else {
            writeTitle(std::cout, msg->title());
            writeContent(std::cout, msg->buffer().str());
        }
    }
}

void LogService::writeTitle(std::ostream &out, const std::string &title) const
{
    if( title.length() > 0 ){
        out << title << ": ";
    }
}

void LogService::writeContent(std::ostream &out, const std::string &text) const
{
    out << text;
    if( string_endswith(text, '\n') ){
        out << std::flush;
        //std::cerr << "(newline found)" << std::endl;
    }
}

logger &LogService::get(ks_thread_id id, std::string title, LogLevel level, const bool &autoflush)
{
    //std::cerr << "LogService::get" << std::endl;
    if( pool_.find(id) == pool_.end() ){
        pool_[id] = std::map<LogLevel, logger *>();
    }
    std::map<LogLevel, logger *> tlogger = pool_[id];

    if( hasKeyInMap(level, tlogger) ){
        if( (title.length() == 0) || (tlogger[level]->title() == title) ){
            return *(tlogger[level]);
        }

        // dispatch and clean up the existing logger
        this->dispatch(tlogger[level]);
    }

    //std::cerr << "creating a new logger" << std::endl;
    logger *newlogger = new logger(id, title, level, autoflush);
    pool_[id][level] = newlogger;
    return *(newlogger);
}

void LogService::dispatch_static(LogService *service, logger *msg)
{
    service->dispatch(msg);
}

void LogService::clean(logger *msg)
{
    //std::cerr << "LogService::clean(logger)" << std::endl;
    if( hasKeyInMap(msg->thread(), pool_) &&
            hasKeyInMap(msg->level(), pool_[msg->thread()]) &&
                (msg->title() == pool_[msg->thread()][msg->level()]->title()) )
    {
        pool_[msg->thread()].erase(msg->level());
        delete msg;
    } else {
        throw std::runtime_error("clean() got a logger that LogService does not manage");
    }
}

void LogService::clean(LogService *service, ks_thread_id id)
{
    service->clean(id);
}

void LogService::clean(ks_thread_id id)
{
#ifdef DEBUG_KS_LOG
    std::cerr << "LogService::clean(ks_thread_id)" << std::endl;
#endif
    if( hasKeyInMap(id, pool_) ){
        clear_byvalue(this, pool_[id], LogService::dispatch_static, false);
        pool_.erase(id);
    }
}

void LogService::clean()
{
    //std::cerr << "LogService::clean" << std::endl;
    clear_bykey(this, pool_, LogService::clean, false);
}


LogPool::LogPool(void (*addService)(LogHandler *), void (*removeService)(LogHandler *)):
    LogHandler(Debug),
    LogHandlerManager(),
    reg_(addService),
    unreg_(removeService)
{
    reg_(this);
    registered_ = true;
}

LogPool::~LogPool()
{
    unregister();
}

void LogPool::unregister()
{
    if( registered_ == true ){
        unreg_(this);
        registered_ = false;
    }
}

void LogPool::handleLog(logger *msg)
{
    logger *copy = new logger(msg->thread(), msg->title(), msg->level(), msg->autoflush());
    logs_.push_back(copy);
}

void LogPool::dispatchAll(bool unregister)
{
    clear_vector(this, logs_, LogPool::dispatch_static);
    if( unregister == true ){
        this->unregister();
    }
}

// static
void LogPool::dispatch_static(LogPool *pool, logger *msg)
{
    pool->dispatch(msg);
}

void LogPool::clean(logger *msg)
{
    std::vector<logger *>::iterator it = std::find(logs_.begin(), logs_.end(), msg);
    if( it != logs_.end() ){
        logs_.erase(it);
        delete msg;
    }
}


LogService logger::service_ = LogService();

//static
void logger::addHandler(LogHandler *handler) { service_.addHandler(handler); }
//static
void logger::removeHandler(LogHandler *handler) { service_.removeHandler(handler); }

//static
logger &logger::log(std::string title, LogLevel level, const bool &autoflush)
{
    return service_.get(Thread::id(), title, level, autoflush);
}
//static
logger &logger::error(std::string title, const bool &autoflush) { return log(title, Error, autoflush); }
//static
logger &logger::warning(std::string title, const bool &autoflush) { return log(title, Warning, autoflush); }
//static
logger &logger::info(std::string title, const bool &autoflush) { return log(title, Info, autoflush); }
//static
logger &logger::fine(std::string title, const bool &autoflush) { return log(title, Fine, autoflush); }
//static
logger &logger::debug(std::string title, const bool &autoflush) { return log(title, Debug, autoflush); }
//static
void logger::setLoggedLevel(const LogLevel &level){ service_.seLoggedLevel(level); }

logger::logger(ks_thread_id id, std::string title, LogLevel level, const bool &autoflush):
    thread_(id),
    title_(title),
    level_(level),
    autoflush_(autoflush)
{

}

void logger::dispatch()
{
    service_.dispatch(this);
}

ks_thread_id &logger::thread()     { return thread_; }
std::string &logger::title()      { return title_; }
LogLevel &logger::level()      { return level_; }
const bool        &logger::autoflush() const  { return autoflush_; }
std::stringstream &logger::buffer() { return buf_; }
std::string        logger::content() { return buf_.str(); }

template<>
logger &operator<<(logger &loggerobj, const LogMeta &val){
    //std::cerr << "logger<<LogMeta" << std::endl;
    if( val == endl ){
        loggerobj.buffer() << std::endl;
        if( loggerobj.autoflush() == true ){
            loggerobj.dispatch();
        }
    } else if( val == flush ){
        loggerobj.dispatch();
    }
    return loggerobj;
}

}
