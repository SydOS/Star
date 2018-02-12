#include <main.h>
#include <tools.h>
#include <io.h>
#include <driver/speaker.h>
#include <kernel/pit.h>

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
