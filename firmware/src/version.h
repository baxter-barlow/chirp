/**
 * @file version.h
 * @brief Firmware version information
 */

#ifndef VERSION_H
#define VERSION_H

#define FIRMWARE_VERSION_MAJOR  0
#define FIRMWARE_VERSION_MINOR  1
#define FIRMWARE_VERSION_PATCH  0

#define FIRMWARE_VERSION_STRING "0.1.0"
#define FIRMWARE_BUILD_DATE     __DATE__
#define FIRMWARE_BUILD_TIME     __TIME__

/* Feature flags (compile-time) */
#define FIRMWARE_FEATURE_COMPLEX_RANGE_FFT  1
#define FIRMWARE_FEATURE_TARGET_IQ          0  /* Phase 2 */
#define FIRMWARE_FEATURE_PHASE_OUTPUT       0  /* Phase 2 */
#define FIRMWARE_FEATURE_MOTION_DETECT      0  /* Phase 2 */
#define FIRMWARE_FEATURE_PRESENCE           0  /* Phase 2 */
#define FIRMWARE_FEATURE_POWER_MGMT         0  /* Phase 3 */

#endif /* VERSION_H */
