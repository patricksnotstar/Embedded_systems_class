#ifndef _HELPER_H_
#define _HELPER_H_

void Helper_sleep_thread(long seconds, long nanoseconds);
void Helper_writeToFile(const char *fileName, const char *value);
void Helper_changeVolume(char *direction);
void Helper_changeTempo(char *direction);

#endif