#ifndef _BEATBOX_H_
#define _BEATBOX_H_

enum sounds
{
    NONE,
    ROCK1,
    ROCK2
};

int Beatbox_currentlyPlayingSound;
pthread_mutex_t Beatbox_playbackMutex;

#endif