/*
 * version.c
 *
 * BLE Tag Firmware Revision Number
 */

#include "stdio.h"

/**
 * @brief GuardRFID BLE FW Version -> Format 4 Bytes "a.b.c.d"
 * @details
 *     For Dev/Internal/QA Releases increments "d"
 *     For Production Releases increments the other bytes and leave "d" equals 0.
 */
const uint8_t firmware_revision[4] = { 0, 0, 0, 4 };
