// Copyright (C) 2019. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "cpu/x86/fp32/tensor_computing_fp32.h"

EE attention_fp32(U32 batch,
    U32 numHeads,
    I32 fromSequenceLength,
    I32 toSequenceLength,
    const F32 *input,
    F32 *output)
{
    if (nullptr == input || nullptr == output) {
        CHECK_STATUS(NULL_POINTER);
    }

    F32 mask_s = -10000.0;
    I32 count = array_sum_f32(input, toSequenceLength);
    I32 valid = UNI_MIN(count, fromSequenceLength);
    __m256 mask_v = _mm256_set1_ps(mask_s);
    __m256 one_v = _mm256_set1_ps(1.0);
    for (U32 n = 0; n < batch; n++) {
        for (U32 i = 0; i < numHeads; i++) {
            if (i == 0) {
                for (I32 j = 0; j < valid; j++) {
                    if (j == 0) {
                        I32 k = 0;
                        for (; k < toSequenceLength - 7; k += 8) {
                            __m256 in_v = _mm256_loadu_ps(input + k);
                            __m256 tmp_v = _mm256_sub_ps(one_v, in_v);
                            tmp_v = _mm256_mul_ps(tmp_v, mask_v);
                            _mm256_storeu_ps(output + k, tmp_v);
                        }
                        for (; k < toSequenceLength; k++) {
                            F32 value = (1 - input[k]) * mask_s;
                            output[k] = value;
                        }
                    } else {
                        memcpy(
                            output + j * toSequenceLength, output, toSequenceLength * sizeof(F32));
                    }
                }

                for (I32 j = valid; j < fromSequenceLength; j++) {
                    if (j == valid) {
                        I32 k = 0;
                        for (; k < toSequenceLength - 7; k += 8) {
                            _mm256_storeu_ps(output + j * toSequenceLength + k, mask_v);
                        }
                        for (; k < toSequenceLength; k++) {
                            output[j * toSequenceLength + k] = mask_s;
                        }
                    } else {
                        memcpy(output + j * toSequenceLength, output + valid * toSequenceLength,
                            toSequenceLength * sizeof(F32));
                    }
                }
            } else {
                memcpy(output + i * fromSequenceLength * toSequenceLength, output,
                    fromSequenceLength * toSequenceLength * sizeof(F32));
            }
        }

        input += toSequenceLength;
        output += numHeads * fromSequenceLength * toSequenceLength;
    }
    return SUCCESS;
}
