/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 nullptr (Seeed Technology Inc.)
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

#ifndef _EL_DATA_H_
#define _EL_DATA_H_

#include "el_data_model_loader.hpp"
#include "el_data_persistent_map.hpp"

#define CONFIG_EL_MODELS_PARTITION_NAME    "models"
#define CONFIG_EL_DATA_PERSISTENT_MAP_NAME "edgelab_db"
#define CONFIG_EL_DATA_PERSISTENT_MAP_PATH "kvdb0"

namespace edgelab {

using ModelLoader   = data::ModelLoader;
using PersistentMap = data::PersistentMap;

}  // namespace edgelab

// TODO: avoid expose the namespace to global
using namespace edgelab::data::traits;
using namespace edgelab::data::types;
using namespace edgelab::data::utility;

#endif