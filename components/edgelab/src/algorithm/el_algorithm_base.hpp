/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Hongtai Liu (Seeed Technology Inc.)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef _EL_ALGORITHM_BASE_HPP_
#define _EL_ALGORITHM_BASE_HPP_

#include <forward_list>

#include "el_types.h"

namespace edgelab {
namespace algorithm {
namespace base {

template <typename InferenceEngine, typename InputType, typename OutputType> class Algorithm {
   private:
    uint32_t __preprocess_time;   // ms
    uint32_t __run_time;          // ms
    uint32_t __postprocess_time;  // ms

   protected:
    InferenceEngine* __p_engine;
    InputType*       __p_input;

    uint8_t __score_threshold;

    virtual EL_STA preprocess()  = 0;
    virtual EL_STA postprocess() = 0;

   public:
    Algorithm(InferenceEngine& engine, uint8_t score_threshold = 40);
    virtual ~Algorithm();

    virtual EL_STA init()   = 0;
    virtual EL_STA deinit() = 0;

    EL_STA run(InputType* input);

    uint32_t get_preprocess_time() const;
    uint32_t get_run_time() const;
    uint32_t get_postprocess_time() const;

    EL_STA  set_score_threshold(uint8_t threshold);
    uint8_t get_score_threshold() const;

    virtual const std::forward_list<OutputType>& get_results() = 0;
};

template <typename InferenceEngine, typename InputType, typename OutputType>
Algorithm<InferenceEngine, InputType, OutputType>::Algorithm(InferenceEngine& engine, uint8_t score_threshold)
    : __p_engine(&engine), __p_input(nullptr), __score_threshold(score_threshold) {
    __preprocess_time  = 0;
    __run_time         = 0;
    __postprocess_time = 0;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
Algorithm<InferenceEngine, InputType, OutputType>::~Algorithm() {
    __p_engine = nullptr;
    __p_input  = nullptr;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
EL_STA Algorithm<InferenceEngine, InputType, OutputType>::run(InputType* input) {
    EL_STA   ret        = EL_OK;
    uint32_t start_time = 0;
    uint32_t end_time   = 0;

    EL_ASSERT(__p_engine != nullptr);

    __p_input = input;

    // preprocess
    start_time        = el_get_time_ms();
    ret               = preprocess();
    end_time          = el_get_time_ms();
    __preprocess_time = end_time - start_time;

    if (ret != EL_OK) {
        return ret;
    }

    // run
    start_time = el_get_time_ms();
    ret        = __p_engine->run();
    end_time   = el_get_time_ms();
    __run_time = end_time - start_time;

    if (ret != EL_OK) {
        return ret;
    }

    // postprocess
    start_time         = el_get_time_ms();
    ret                = postprocess();
    end_time           = el_get_time_ms();
    __postprocess_time = end_time - start_time;

    return ret;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
uint32_t Algorithm<InferenceEngine, InputType, OutputType>::get_preprocess_time() const {
    return __preprocess_time;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
uint32_t Algorithm<InferenceEngine, InputType, OutputType>::get_run_time() const {
    return __run_time;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
uint32_t Algorithm<InferenceEngine, InputType, OutputType>::get_postprocess_time() const {
    return __postprocess_time;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
EL_STA Algorithm<InferenceEngine, InputType, OutputType>::set_score_threshold(uint8_t threshold) {
    __score_threshold = threshold;
    return EL_OK;
}

template <typename InferenceEngine, typename InputType, typename OutputType>
uint8_t Algorithm<InferenceEngine, InputType, OutputType>::get_score_threshold() const {
    return __score_threshold;
}

}  // namespace base
}  // namespace algorithm
}  // namespace edgelab

#endif