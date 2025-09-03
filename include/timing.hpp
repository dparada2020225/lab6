#pragma once
#include <ctime>

/**
 * Temporizador sencillo usando CLOCK_MONOTONIC
 * Retorna el tiempo actual en segundos como double
 */
inline double now_s() {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}