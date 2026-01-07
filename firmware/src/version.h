/**
 * @file version.h
 * @brief chirp firmware version information
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 */

#ifndef CHIRP_VERSION_H
#define CHIRP_VERSION_H

#define CHIRP_VERSION_MAJOR 0
#define CHIRP_VERSION_MINOR 1
#define CHIRP_VERSION_PATCH 0

#define CHIRP_VERSION_STRING "0.1.0"
#define CHIRP_BUILD_DATE     __DATE__
#define CHIRP_BUILD_TIME     __TIME__

/* Feature flags (compile-time) */
#define CHIRP_FEATURE_COMPLEX_RANGE_FFT 1
#define CHIRP_FEATURE_TARGET_IQ         0 /* Phase 2 */
#define CHIRP_FEATURE_PHASE_OUTPUT      0 /* Phase 2 */
#define CHIRP_FEATURE_MOTION_DETECT     0 /* Phase 2 */
#define CHIRP_FEATURE_PRESENCE          0 /* Phase 2 */
#define CHIRP_FEATURE_POWER_MGMT        0 /* Phase 3 */

#endif /* CHIRP_VERSION_H */
