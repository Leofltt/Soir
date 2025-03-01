#ifndef AUDIOCHANNEL_H
#define AUDIOCHANNEL_H

#include <3ds/types.h>

typedef struct {
    int id;
    u16 format;
    float samplerate;
} AudioChannel;



#endif // AUDIOCHANNEL_H