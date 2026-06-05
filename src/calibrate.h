#pragma once
#include "engine.h"

// Runs the full calibration sweep: refresh sweep, poll sweep, hardware fingerprint.
// Prints results to stdout.
void calibrate_run(ConsoleEngine* eng);

// Refresh rate sweep
void calibrate_refresh_sweep(ConsoleEngine* eng,
                              int* out_max_stable_refresh,
                              int* out_first_tearing_rate);

// Polling rate sweep
void calibrate_poll_sweep(ConsoleEngine* eng,
                           int* out_min_effective_poll,
                           int* out_first_latency_ceiling);

// Hardware fingerprint
void calibrate_hardware_fingerprint(ConsoleEngine* eng);

// Print final report
void calibrate_report(int max_stable_refresh,
                       int tearing_floor,
                       int min_effective_poll,
                       int latency_ceiling);
