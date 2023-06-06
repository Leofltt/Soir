enum ClockStatus { Playing, Paused, Stopped};

typedef struct Clock {
    int bpm;
    int beats_per_bar = 4;
};