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

/**
*   timing.cpp -- see timing.h for description
*/

#include "ks/timing.h"
#include "ks/utils.h"
#include <iostream>
#ifndef _WIN32
#include <unistd.h> // sleep, usleep
#endif

namespace ks {

    void sleep_seconds(uint16_t duration)
    {
#ifdef _WIN32
        Sleep(duration*1000);
#else
        sleep(duration);
#endif
    }

    void sleep_msec(unsigned int msec)
    {
#ifdef _WIN32
        Sleep(msec);
#else
        usleep(msec*1000);
#endif
    }

#ifdef _WIN32
    nanostamp::nanostamp()
    {
        LARGE_INTEGER freq;
        int supported = QueryPerformanceFrequency(&freq);
        supported_ = (supported == 0)? false: true;
        if (!supported_)
        {
            std::cerr << "***QPC clock is not available on this platform.";
            std::cerr << "disabling calculation of transaction latency." << std::endl;
            freq_ = 1;
        } else {
            freq_ = (uint64_t)(freq.QuadPart);
        }
    }

    void nanostamp::get(uint64_t *holder)
    {
        if (!supported_) {
            return;
        }

        LARGE_INTEGER count;
        if (QueryPerformanceCounter(&count) == 0) {
            std::cerr << "***failure to get QPC: " << error_message() << std::endl;
            std::cerr << "***disabling latency calculation." << std::endl;
            supported_ = false;
            return;
        }

        uint64_t ucount = (uint64_t)(count.QuadPart);
        *holder = (ucount*NSEC_IN_SEC)/freq_;
    }
#else
    nanostamp::nanostamp()
    {
        struct timespec test;
        if (clock_gettime(CLOCK_REALTIME, &test)) {
            std::cerr << "***real-time clock is not available on this platform.";
            std::cerr << "disabling calculation of transaction latency." << std::endl;
            supported_ = false;
        }
    }

    void nanostamp::get(uint64_t *holder)
    {
        if (!supported_) {
            *holder = 0;
        }

        struct timespec _clock;
        if (clock_gettime(CLOCK_REALTIME, &_clock)) {
            std::cerr << "***failure to get real-time clock: " << error_message() << std::endl;
            std::cerr << "***disabling latency calculation." << std::endl;
            supported_ = false;
            *holder = 0;
        }

        *holder = ((uint64_t)(_clock.tv_sec))*NSEC_IN_SEC + (uint64_t)(_clock.tv_nsec);
    }
#endif

    bool nanostamp::is_available() { return supported_; }

#ifdef _WIN32
    void nanotimer::set_interval(uint64_t value)
    {
        return; // right now we cannot use nanosleep(), and I have not found any alternatives.
    }

    void nanotimer::sleep()
    {
        return; // right now we cannot use nanosleep(), and I have not found any alternatives.
    }
#else
    void nanotimer::set_interval(uint64_t value)
    {
        if (value > NSEC_IN_SEC) {
            spec_.tv_sec  = value / NSEC_IN_SEC;
            spec_.tv_nsec = value % NSEC_IN_SEC;
        } else {
            spec_.tv_sec  = 0;
            spec_.tv_nsec = value;
        }
    }

    void nanotimer::sleep()
    {
        nanosleep(&spec_, NULL);
    }
#endif
}



