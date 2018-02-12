#define SPEAKER_PORT 0x61

extern void speaker_start_tone(uint32_t freq);
extern void speaker_stop();
extern void speaker_play_tone(uint32_t freq, uint32_t ms);
