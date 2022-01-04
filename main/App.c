// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

// An example that implements the light bulb HomeKit profile. It can serve as a basic implementation for
// any platform. The accessory logic implementation is reduced to internal state updates and log output.
//
// This implementation is platform-independent.
//
// The code consists of multiple parts:
//
//   1. The definition of the accessory configuration and its internal state.
//
//   2. Helper functions to load and save the state of the accessory.
//
//   3. The definitions for the HomeKit attribute database.
//
//   4. The callbacks that implement the actual behavior of the accessory, in this
//      case here they merely access the global accessory state variable and write
//      to the log to make the behavior easily observable.
//
//   5. The initialization of the accessory state.
//
//   6. Callbacks that notify the server in case their associated value has changed.

#include "HAP.h"

#include "App.h"
#include "DB.h"

#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include "driver/timer.h"
#include <esp_event.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Domain used in the key value store for application data.
 *
 * Purged: On factory reset.
 */
#define kAppKeyValueStoreDomain_Configuration ((HAPPlatformKeyValueStoreDomain) 0x00)

/**
 * Key used in the key value store to store the configuration state.
 *
 * Purged: On factory reset.
 */
#define kAppKeyValueStoreKey_Configuration_State ((HAPPlatformKeyValueStoreDomain) 0x00)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define kLedGPIOPin 19

/**
 * Global accessory configuration.
 */
typedef struct {
    struct {
        bool lightBulbOn;
    } state;
    HAPAccessoryServerRef* server;
    HAPPlatformKeyValueStoreRef keyValueStore;
} AccessoryConfiguration;

static AccessoryConfiguration accessoryConfiguration;

//----------------------------------------------------------------------------------------------------------------------

/**
 * Load the accessory state from persistent memory.
 */
static void LoadAccessoryState(void) {
    HAPPrecondition(accessoryConfiguration.keyValueStore);

    HAPError err;

    // Load persistent state if available
    bool found;
    size_t numBytes;

    err = HAPPlatformKeyValueStoreGet(
            accessoryConfiguration.keyValueStore,
            kAppKeyValueStoreDomain_Configuration,
            kAppKeyValueStoreKey_Configuration_State,
            &accessoryConfiguration.state,
            sizeof accessoryConfiguration.state,
            &numBytes,
            &found);

    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
    if (!found || numBytes != sizeof accessoryConfiguration.state) {
        if (found) {
            HAPLogError(&kHAPLog_Default, "Unexpected app state found in key-value store. Resetting to default.");
        }
        HAPRawBufferZero(&accessoryConfiguration.state, sizeof accessoryConfiguration.state);
    } else {
        // here we should set the gpio to the correct value with a switch on stored value
        // when a garage door, set target and current to 1 instead of activating led and save accessory state.
        gpio_set_level(kLedGPIOPin, accessoryConfiguration.state.lightBulbOn);
    }
}

/**
 * Save the accessory state to persistent memory.
 */
static void SaveAccessoryState(void) {
    HAPPrecondition(accessoryConfiguration.keyValueStore);

    HAPError err;
    err = HAPPlatformKeyValueStoreSet(
            accessoryConfiguration.keyValueStore,
            kAppKeyValueStoreDomain_Configuration,
            kAppKeyValueStoreKey_Configuration_State,
            &accessoryConfiguration.state,
            sizeof accessoryConfiguration.state);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * HomeKit accessory that provides the Light Bulb service.
 *
 * Note: Not constant to enable BCT Manual Name Change.
 */
static HAPAccessory accessory = { .aid = 1,
                                  .category = kHAPAccessoryCategory_Lighting,
                                  .name = "Light Bulb",
                                  .manufacturer = "Manufacturer",
                                  .model = "LightBulb1,1",
                                  .serialNumber = "099DB48E9E28",
                                  .firmwareVersion = "1",
                                  .hardwareVersion = "1",
                                  .services = (const HAPService* const[]) { &accessoryInformationService,
                                                                            &hapProtocolInformationService,
                                                                            &pairingService,
                                                                            &lightBulbService,
                                                                            NULL },
                                  .callbacks = { .identify = IdentifyAccessory } };

//----------------------------------------------------------------------------------------------------------------------

void AccessoryNotification(
        const HAPAccessory* accessory,
        const HAPService* service,
        const HAPCharacteristic* characteristic,
        void* ctx) {
    HAPLogInfo(&kHAPLog_Default, "Accessory Notification");

    HAPAccessoryServerRaiseEvent(accessoryConfiguration.server, characteristic, service, accessory);
}

/**
 * Context for the HAP run loop callback that raises a characteristic changed event
 * with the specified data in the context.
 */
typedef struct {
    HAPAccessory* accessory;
    HAPService* service;
    HAPCharacteristic* characteristic;
} AccessoryNotificationCallbackContext;

void run_loop_callback(void* context, size_t context_size) {
    AccessoryNotificationCallbackContext* c = (AccessoryNotificationCallbackContext *) context;
    HAPAccessoryServerRaiseEvent(accessoryConfiguration.server, c->characteristic, c->service, c->accessory);
}

void ScheduleAccessoryNotificationInRunLoop(
        const HAPAccessory* accessory,
        const HAPService* service,
        const HAPCharacteristic* characteristic) {
    AccessoryNotificationCallbackContext context = {
        .accessory = accessory,
        .service = service,
        .characteristic = characteristic,
    };
    HAPPlatformRunLoopScheduleCallback(run_loop_callback, &context, sizeof context);
}

void AppCreate(HAPAccessoryServerRef* server, HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(server);
    HAPPrecondition(keyValueStore);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPRawBufferZero(&accessoryConfiguration, sizeof accessoryConfiguration);
    accessoryConfiguration.server = server;
    accessoryConfiguration.keyValueStore = keyValueStore;
    LoadAccessoryState();
}

void AppRelease(void) {
}

void AppAccessoryServerStart(void) {
    HAPAccessoryServerStart(accessoryConfiguration.server, &accessory);
}                                

//----------------------------------------------------------------------------------------------------------------------

void *switch_off_handler_task_ptr;


static bool IRAM_ATTR switch_off_timer_callback(void *args){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(switch_off_handler_task_ptr, &xHigherPriorityTaskWoken);
    return xHigherPriorityTaskWoken == pdTRUE;
}


void switch_off_handler(void *args){
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    for(;;){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        timer_pause(TIMER_GROUP_0, TIMER_0); // todo: deinit timer by handle instead of hardcoding it here
        if (accessoryConfiguration.state.lightBulbOn) {
            HAPLog(&kHAPLog_Default, "");
            accessoryConfiguration.state.lightBulbOn = false;
            gpio_set_level(kLedGPIOPin, false);
            SaveAccessoryState();
            ScheduleAccessoryNotificationInRunLoop(&accessory, &lightBulbService, &lightBulbOnCharacteristic);
        }
    }
}




//----------------------------------------------------------------------------------------------------------------------

#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

void set_timer(uint64_t timer_duration_in_ms, timer_isr_t isr_handler) {
    /* Select and initialize basic parameters of the timer */
        timer_config_t config = {
            .divider = TIMER_DIVIDER,
            .counter_dir = TIMER_COUNT_UP,
            .counter_en = TIMER_PAUSE,
            .alarm_en = TIMER_ALARM_EN,
            .auto_reload = false,
        }; // default clock source is APB

        // ESP_LOGI(LOGNAME, "created config\n");

        timer_init(TIMER_GROUP_0, TIMER_0, &config);
        // ESP_LOGI(LOGNAME, "timer init\n");
        HAPLogInfo(&kHAPLog_Default, "Timer initialized");

        /* Timer's counter will initially start from value below.
        Also, if auto_reload is set, this value will be automatically reload on alarm */
        timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
        // ESP_LOGI(LOGNAME, "timer counter value set to 0\n");

        /* Configure the alarm value and the interrupt on alarm. */
        timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, timer_duration_in_ms / 1000 * TIMER_SCALE);
        // ESP_LOGI(LOGNAME, "timer value set to 5 seconds\n");

        timer_enable_intr(TIMER_GROUP_0, TIMER_0);
        // ESP_LOGI(LOGNAME, "timer enabled interruption \n");

        timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, isr_handler, NULL, 0);

        // ESP_LOGI(LOGNAME, "timer callback added\n");
        
        timer_start(TIMER_GROUP_0, TIMER_0);
        HAPLogInfo(&kHAPLog_Default, "Timer started");
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError IdentifyAccessory(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPAccessoryIdentifyRequest* request HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleLightBulbOnRead(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.lightBulbOn;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, *value ? "true" : "false");

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleLightBulbOnWrite(
        HAPAccessoryServerRef* server,
        const HAPBoolCharacteristicWriteRequest* request,
        bool value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, value ? "true" : "false");
    if (accessoryConfiguration.state.lightBulbOn != value) {
        accessoryConfiguration.state.lightBulbOn = value;
        // this should be a switch in the future or with a helper function for mapping the value and notifying current/target state
        gpio_set_level(kLedGPIOPin, value);

        SaveAccessoryState();

        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
        set_timer(5000, switch_off_timer_callback);
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

void AccessoryServerHandleUpdatedState(HAPAccessoryServerRef* server, void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(!context);

    switch (HAPAccessoryServerGetState(server)) {
        case kHAPAccessoryServerState_Idle: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Idle.");
            return;
        }
        case kHAPAccessoryServerState_Running: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Running.");
            return;
        }
        case kHAPAccessoryServerState_Stopping: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Stopping.");
            return;
        }
    }
    HAPFatalError();
}

const HAPAccessory* AppGetAccessoryInfo() {
    return &accessory;
}

void AppInitialize(
        HAPAccessoryServerOptions* hapAccessoryServerOptions,
        HAPPlatform* hapPlatform,
        HAPAccessoryServerCallbacks* hapAccessoryServerCallbacks) {
    HAPLogInfo(&kHAPLog_Default, "Initializing app and GPIO pin.");
    gpio_pad_select_gpio(kLedGPIOPin);
    gpio_set_direction(kLedGPIOPin, GPIO_MODE_OUTPUT);
    gpio_set_level(kLedGPIOPin, 0);
    xTaskCreate(switch_off_handler, "switch_off_handler", 4 * 1024, NULL, 10, &switch_off_handler_task_ptr);
}

void AppDeinitialize() {
    /*no-op*/
}
