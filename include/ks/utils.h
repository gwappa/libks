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
*   utils.h -- generic utility definitions
*/
#ifndef __KS_UTILS_H__
#define __KS_UTILS_H__

#include <string>
#include <map>
#include <vector>

#define hasKeyInMap(key, mapobj) ( (!((mapobj).empty())) && (((mapobj).find((key))) != ((mapobj).end())))

namespace ks {
    /**
     * returns the OS-specific error message as string.
     */
    std::string error_message();

    enum ResultType { Success, Failure };

    template <typename T>
    class Result {
    private:
        ResultType        _type;
        T                 _value;
        const std::string _msg;

        explicit Result(ResultType type):
            _type(type) {}

    public:
        Result(ResultType type, const T& value):
            _type(type), _value(value) {}
        Result(ResultType type, const std::string& msg):
            _type(type), _msg(msg) {}

        const bool failed() const
        {
            return (_type == Failure);
        }

        const bool successful() const
        {
            return (_type == Success);
        }

        T& get()
        {
            return _value;
        }

        const std::string& what() const
        {
            return _msg;
        }

        static Result<T> success(const T& value)
        {
            return Result<T>(Success, value);
        }

        static Result<T> success()
        {
            return Result<T>(Success);
        }

        static Result<T> failure()
        {
            return Result<T>(Failure);
        }

        static Result<T> failure(const std::string& msg)
        {
            return Result<T>(Failure, msg);
        }
    };


    /**
     *  if finalize_element() involves erasing the key as well, put 'false' to erase_key
     */
    template<typename Cls, typename K, typename V>
    void clear_byvalue(Cls instance, std::map<K, V> mapobj, void (*finalize_element)(Cls, V), bool erase_key)
    {
        typedef typename std::map<K, V>::iterator tmpiter;
        K *keys = new K[mapobj.size()];
        V *values = new V[mapobj.size()];
        int sizeptr = 0;
        for( tmpiter it=mapobj.begin(); it != mapobj.end(); ++it, ++sizeptr ){
            *(keys+sizeptr) = static_cast<K>(it->first);
            *(values+sizeptr) = static_cast<V>(it->second);
        }
        // now sizeptr should point to mapobj.size()
        for(int idx=0; idx<sizeptr; idx++){
            finalize_element(instance, *(values+idx));
            if( erase_key == true ){
                mapobj.erase(*(keys+idx));
            }
        }
        delete[] keys;
        delete[] values;
    }

    template<typename Cls, typename K, typename V>
    void clear_bykey(Cls instance, std::map<K, V> mapobj, void (*finalize_element)(Cls, K), bool erase_key)
    {
        typedef typename std::map<K, V>::iterator tmpiter;
        K *keys = new K[mapobj.size()];
        int sizeptr = 0;
        for( tmpiter it=mapobj.begin(); it != mapobj.end(); ++it, ++sizeptr ){
            *(keys+sizeptr) = static_cast<K>(it->first);
        }

        // now sizeptr should point to mapobj.size()
        for(int idx=0; idx<sizeptr; idx++){
            finalize_element(instance, *(keys+idx));
            if( erase_key == true ){
                mapobj.erase(*(keys+idx));
            }
        }
        delete[] keys;
    }

    template<typename Cls, typename V>
    void clear_vector(Cls instance, std::vector<V> v, void (*finalize)(Cls, V))
    {
        typedef typename std::vector<V>::iterator iterator;
        V val;
        for(iterator it=v.begin(); it!=v.end(); ++it){
            val = static_cast<V>(*it);
            finalize(instance, val);
        }
        v.clear();
    }

    inline bool string_endswith(const std::string &str, char c)
    {
        return ( str.find(c, str.length()-1) != std::string::npos );
    }

    /**
    *   the template class used for averaging lots of samples.
    *   the sum will be reset at a certain limit to avoid overflow.
    */
    template <typename Val, typename Num>
    class averager
    {
    public:
        averager(Val limit): limit_(limit), sum_(0), num_(0) {}
        void add(Val v)
        {
            if (sum_ > limit_)
            {
                sum_ = v;
                num_ = 1;
            } else {
                sum_ += v;
                num_++;
            }
        }

        double get()
        {
            return ((double)sum_)/num_;
        }

        double get_inv(Val nom)
        {
            return ((double)nom)*num_/sum_;
        }

        Val sum() { return sum_; }
        Num num() { return num_; }

    private:
        Val   limit_;
        Val   sum_;
        Num   num_;
    };
}

#endif
