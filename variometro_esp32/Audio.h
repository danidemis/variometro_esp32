#ifndef AUDIO_H
#define AUDIO_H

void audio_init();
void audio_update(float vario_mps);
void audio_beep_feedback(); // Nuova funzione per il bip dei tasti
void audio_stop();

#endif