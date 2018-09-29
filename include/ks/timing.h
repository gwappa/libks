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

#ifndef __KS_TIMING_H__
#define __KS_TIMING_H__

#include <stdint.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <time.h>
#include <sys/time.h>
#endif

namespace ks {

    void sleep_seconds(uint16_t duration);

    void sleep_msec(unsigned int msec);

    const uint64_t NSEC_IN_SEC = 1000000000ULL;

    /**
    *   platform-specific wrapper for a real-time clock
    */
    class nanostamp
    {
    public:
        nanostamp();
        /**
        *   get the timestamp into `holder`
        *   somehow it did not work by returning a uint64_t value.
        */
        void get(uint64_t *holder);

        /**
        *   tells if the real-time clock is available on the platform
        */
        bool is_available();
    private:
        bool     supported_;
#ifdef _WIN32
        uint64_t freq_;
#endif
    };

#ifdef _WIN32
    /**
    *   the structure used for precision timer on Windows
    */
    typedef LARGE_INTEGER timerspec_t;
#else
    /**
    *   on *NIX, we only use the timespec as provided
    */
    typedef struct timespec timerspec_t;
#endif

    /**
    *   platform-specific wrapper for a presicion timer
    */
    class nanotimer
    {
    public:
        nanotimer() {}
        /**
        *   sets the value to sleep during each sleep() call.
        *   (actual sleep duration will be `value` nanosec at minimum)
        */
        void set_interval(uint64_t value);
        void sleep();
    private:
        timerspec_t spec_;
    };

}

#endif
