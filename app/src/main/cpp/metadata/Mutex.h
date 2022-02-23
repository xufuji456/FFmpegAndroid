/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MUTEX_H
#define MUTEX_H

#include <stdint.h>
#include <sys/types.h>
#include <time.h>
# include <pthread.h>


class Mutex {
public:
    Mutex();
    ~Mutex();

    int32_t lock();
    void    unlock();

    class Autolock {
    public:
        inline Autolock(Mutex& mutex) : mLock(mutex)  { mLock.lock(); }
        inline Autolock(Mutex* mutex) : mLock(*mutex) { mLock.lock(); }
        inline ~Autolock() { mLock.unlock(); }
    private:
        Mutex& mLock;
    };

private:
    Mutex(const Mutex&);
    Mutex& operator = (const Mutex&);

    pthread_mutex_t mMutex;
};

inline Mutex::Mutex() {
    pthread_mutex_init(&mMutex, NULL);
}

inline Mutex::~Mutex() {
    pthread_mutex_destroy(&mMutex);
}

inline int32_t Mutex::lock() {
    return -pthread_mutex_lock(&mMutex);
}

inline void Mutex::unlock() {
    pthread_mutex_unlock(&mMutex);
}

#endif // MUTEX_H
