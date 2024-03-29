// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

// The most basic HomeKit example: an accessory that represents a light bulb that
// only supports switching the light on and off. Actions are exposed as individual
// functions below.
//
// This header file is platform-independent.

#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Identify routine. Used to locate the accessory.
 */
HAP_RESULT_USE_CHECK
HAPError IdentifyAccessory(
        HAPAccessoryServerRef* server,
        const HAPAccessoryIdentifyRequest* request,
        void* _Nullable context);

/**
 * Handle read request to the 'Current Door State' characteristic of the Garage Door Opener service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleGarageDoorOpenerCurrentDoorStateRead(
        HAPAccessoryServerRef* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context);

/**
 * Handle read request to the 'Target Door State' characteristic of the Garage Door Opener service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleGarageDoorOpenerTargetDoorStateRead(
        HAPAccessoryServerRef* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context);

/**
 * Handle write request to the 'Target Door State' characteristic of the Garage Door Opener service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleGarageDoorOpenerTargetDoorStateWrite(
        HAPAccessoryServerRef* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context);

/**
 * Handle read request to the 'Obstruction Detected' characteristic of the Garage Door Opener service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleGarageDoorOpenerObstructionDetectedRead(
        HAPAccessoryServerRef* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context);

/**
 * Initialize the application.
 */
void AppCreate(HAPAccessoryServerRef* server, HAPPlatformKeyValueStoreRef keyValueStore);

/**
 * Deinitialize the application.
 */
void AppRelease(void);

/**
 * Start the accessory server for the app.
 */
void AppAccessoryServerStart(void);

/**
 * Handle the updated state of the Accessory Server.
 */
void AccessoryServerHandleUpdatedState(HAPAccessoryServerRef* server, void* _Nullable context);

void AccessoryServerHandleSessionAccept(HAPAccessoryServerRef* server, HAPSessionRef* session, void* _Nullable context);

void AccessoryServerHandleSessionInvalidate(
        HAPAccessoryServerRef* server,
        HAPSessionRef* session,
        void* _Nullable context);

/**
 * Restore platform specific factory settings.
 */
void RestorePlatformFactorySettings(void);

/**
 * Returns pointer to accessory information
 */
const HAPAccessory* AppGetAccessoryInfo();

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
