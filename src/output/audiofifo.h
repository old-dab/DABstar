/*
 * This file original taken from AbracaDABra and is adapted by Thomas Neder
 * (https://github.com/tomneda)
 * The original copyright information is preserved below and is acknowledged.
 */

/*
 * This file is part of the AbracaDABra project
 *
 * MIT License
 *
  * Copyright (c) 2019-2023 Petr Kopecký <xkejpi (at) gmail (dot) com>
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

#ifndef AUDIOFIFO_H
#define AUDIOFIFO_H

#include <QMutex>
#include <QWaitCondition>

#define AUDIO_FIFO_CHUNK_MS   (60)
#define AUDIO_FIFO_MS         (32 * AUDIO_FIFO_CHUNK_MS)
#define AUDIO_FIFO_SIZE       (48 * AUDIO_FIFO_MS * 2 * sizeof(int16_t))  // FS - 48kHz, stereo, int16_t samples


struct SAudioFifo
{
    uint32_t sampleRate;
    uint8_t numChannels;
    std::atomic<int64_t> count;
    int64_t head;
    int64_t tail;
    uint8_t buffer[AUDIO_FIFO_SIZE];
    // QWaitCondition countChanged;
    // QMutex mutex;
    void reset();
    void print() const;
    float get_fill_state_in_percent() const { return 100.0f * (float)count / AUDIO_FIFO_SIZE; }
};

//using SAudioFifo = struct AudioFifo;

#endif // AUDIOFIFO_H
