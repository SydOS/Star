/*
 * File: speaker.c
 * 
 * Copyright (c) 2017-2018 Sydney Erickson, John Davis
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

#include <main.h>
#include <tools.h>
#include <io.h>
#include <driver/speaker.h>
#include <driver/pit.h>

// https://wiki.osdev.org/PC_Speaker

// Starts the speaker playing a tone.
void speaker_start_tone(uint32_t freq)
{
    // Configure the PIT.
    pit_startcounter(freq, PIT_CMD_COUNTER2, PIT_CMD_MODE_SQUAREWAVEGEN);

    // Enable speaker.
    uint8_t result = inb(SPEAKER_PORT);
    if (result != (result | 3))
        outb(SPEAKER_PORT, result | 3);
}

// Stops the speaker.
void speaker_stop()
{
    // Stop speaker.
    uint8_t result = inb(SPEAKER_PORT) & 0xFC;
    outb(SPEAKER_PORT, result);
}

// Plays a tone for the specified duration.
void speaker_play_tone(uint32_t freq, uint32_t ms)
{
    // Start tone, wait for duration, and stop.
    speaker_start_tone(freq);
    sleep(ms);
    speaker_stop();
}
