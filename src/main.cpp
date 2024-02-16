/********************************************************************************
 * @file main.cpp
 * @authors Harrison Harding, maxon motor Australia
 * @brief Demo program for the EPOS4 Class
 * @version 1.0.0
 * @date 2024-02-15
 *
 * @copyright Copyright (c) 2022 - 2024
 *
 ********************************************************************************/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string>
#include <vector>
#include "esp_err.h"
#include "esp_log.h"

#include "EPOS4Class.hpp"
#include "main.hpp"

/**
 * The Node-ID of the EPOS4 to be controlled.
 * Can be configured using EPOS Studio or DIP switches. Refer to EPOS4 Hardware Reference.
 **/
const int motorNodeID = 1;

/**
 * The Node-ID used when broadcasting heartbeats.
 * Note that a higher Node-ID has lower priority on the CAN bus,
 * so using a large number on a busy network can increase jitter.
 **/
const int masterNodeID = 127;

EPOS4 motor(motorNodeID); /**< Construct the EPOS4 object. */

/**
 * Defining contracted names for the PDO configurations.
 **/
#define RxPDO_ControlWord_Synchronous "CWS"
#define RxPDO_Profile_Velocity "PV"

#define TxPDO_StatusWord_Position "SP"

/********************************************************************************
 * @brief Task responsible for passing incoming messages from the CAN bus to each EPOS4's reciever.
 ********************************************************************************/
static void receiverTask(void *pvParameters)
{

    static const char *TAG = "receiverTask";
    ESP_LOGI(TAG, "Starting Task");

    twai_message_t message;
    ERROR_CODE_t error_code;

    while (true)
    {
        if (twai_receive(&message, pdMS_TO_TICKS(1000)) == ESP_OK)
        {

            /**
             * The message should be given to all EPOS4 objects' receivers.
             **/
            error_code = motor.receiver(message);

            if (EPOS4::parseError(error_code) == 0b1)
            { // Master Error
              // Error reaction...
            }
            else if (EPOS4::parseError(error_code) == 0b10)
            { // SDO Error
              // Error reaction...
            }
            else if (EPOS4::parseError(error_code) & 0b01111100)
            { // EPOS Error
              // Error reaction...
            }
        }
    }
}

/********************************************************************************
 * @brief Function used to set the mapped objects of the Rx and Tx PDOs for the EPOS4.
 * This simplifies configuring multiple EPOS4's using the same PDO maps.
 *
 * @param node pass by reference to the class instance of the EPOS4 to configure
 * @return  ERROR_CODE_NOERROR or ERROR_CODE_GENERAL_ERROR
 ********************************************************************************/
ERROR_CODE_t PDOHelper(EPOS4 &node)
{

    node.resetNumPDOMapped(); /**< Reset any PDO map that may be in the EPOS4*/
    wait(100);
    PDO_MAPPING_t configuration;
    uint32_t ret = 0;

    /********************************************************************************
     * Receive parameters (Master -> EPOS4)
     *
     * - RXPDO1: ControlWord.
     * Synchronous, Only writes values to EPOS4 object dictionary after a SYNC object.
     * - RXPDO2: Profile Velocity.
     ********************************************************************************/
    configuration = {RXPDO1, PDO_TRANSMISSION_MODE_SYNC, {EPOS_OD_CONTROLWORD}, {}};
    ret |= node.configPDO(RxPDO_ControlWord_Synchronous, configuration);

    configuration = {RXPDO2, PDO_TRANSMISSION_MODE_ASYNC, {EPOS_OD_PROFILE_VELOCITY}, {}};
    ret |= node.configPDO(RxPDO_Profile_Velocity, configuration);

    /********************************************************************************
     * Transmit parameters (EPOS4 -> Master)
     *
     * - TXPDO1: StatusWord and Position.
     * Asynchronous, Transmitted from EPOS4 when a mapped value changes, with a defined minimum period.
     * Inhibit time of 10ms. (100 * 100us) Take care when lowering this time, as it can lead to CAN bus saturation.
     ********************************************************************************/
    configuration = {TXPDO1, PDO_TRANSMISSION_MODE_ASYNC, {EPOS_OD_STATUSWORD, EPOS_OD_POSITION_ACTUAL_VALUE}, {}};
    ret |= node.configPDO(TxPDO_StatusWord_Position, configuration);
    node.sendSDO(EPOS_OD_TRANSMIT_PDO_1_PARAMETER_INHIBIT_TIME_TXPDO_1, 100, 1);

    if (ret != 0)
    {
        return MASTER_ERROR_CODE_GENERIC_ERROR;
    }
    return ERROR_CODE_NOERROR;
}

/********************************************************************************
 * @brief Task which broadcasts a heartbeat onto the CAN bus once a second.
 ********************************************************************************/
static void heartbeatTask(void *pvParameters)
{

    ESP_LOGI(__func__, "Starting Task");

    // Setting up variables for 1s delays
    TickType_t hbLastSendTime = xTaskGetTickCount();
    const TickType_t sendFrequency = configTICK_RATE_HZ;

    while (true)
    {

        if (EPOS4::sendHeartbeat(masterNodeID) != ERROR_CODE_NOERROR)
        {
            ESP_LOGI(__func__, "HEARTBEAT_SEND_ERROR");
        }

        xTaskDelayUntil(&hbLastSendTime, sendFrequency); // 1s delay
    }
}

/********************************************************************************
 * @brief Demo Main, **Motor will move**!
 * Safe Profile Velocity and Profile Acceleration/Deceleration should be configured in EPOS Studio.
 *
 * Start Receiver and Heartbeat Tasks
 * Reset EPOS4,
 * Configure PDOs,
 *
 *
 ********************************************************************************/
void app_main()
{

    esp_log_level_set("*", ESP_LOG_INFO);

    /********************************************************************************
     * Setup the ESP32's drivers and tasks.
     ********************************************************************************/
    EPOS4::TWAISetup(TWAI_TIMING_CONFIG_500KBITS()); // EPOS4 Default is 1Mbit/s

    xTaskCreate(&receiverTask, "receiverTask", 4096, NULL, 4, NULL);
    xTaskCreate(&heartbeatTask, "heartbeat", 4096, NULL, 4, NULL);

    motor.disable(); /**< Bring the EPOS4 into a known starting state */
    motor.clearError();

    if (EPOS4::changeNMTState(NMT_COMMAND_GOTO_PRE_OPERATIONAL) == ERROR_CODE_NOERROR)
    {

        ESP_LOGI("NMT", "Set to Pre-Operational");
        wait(1000);

        if (PDOHelper(motor) == ERROR_CODE_NOERROR)
        { /**< Configure PDOs */

            ESP_LOGI(__func__, "EPOS PDO CONFIGURATION FINISHED");

            motor.setHeartbeatConsumer(masterNodeID, 1500); /**< Configure EPOS4 to require heartbeats at least every 1500ms*/
            wait(1000);

            // Change to NMT operational
            if (EPOS4::changeNMTState(NMT_COMMAND_GOTO_OPERATIONAL) == ERROR_CODE_NOERROR)
            {

                ESP_LOGI("NMT", "Set to Operational");
                wait(1000);

                /********************************************************************************
                 * Profile Velocity Mode
                 ********************************************************************************/
                ESP_LOGI("Axis State", "Enabling...");
                motor.halt();
                motor.enable(); /**< Torque applied to hold position! */

                ESP_LOGI("Profile Velocity", "Changing Mode");
                motor.setModeOfOperation(EPOS_OPERATION_MODE_PVM);

                ESP_LOGI("Profile Velocity", "Starting Motion...");
                motor.moveToTargetVelocity(120); /**< Target velocity unit is RPM (before gearing) by default */
                wait(1000);

                motor.halt();
                ESP_LOGI("Profile Velocity", "Stopping");
                wait(1000);

                /********************************************************************************
                 * Profile Position Mode
                 ********************************************************************************/
                ESP_LOGI("Profile Position", "Changing Mode");
                motor.setModeOfOperation(EPOS_OPERATION_MODE_PPM);

                ESP_LOGI("Profile Position", "Starting Motion...");
                motor.moveToTargetPosition(1000, true, true); /**< Target position unit is encoder quad counts. (4x encoder CPT) */
                ESP_LOGI("Profile Position", "Motion Complete");

                wait(1000);

                /********************************************************************************
                 * Lower level motor operations using individual instructions.
                 * This section uses a Synchronous RxPDO,
                 * which allows for multiple motors to have their motions configured independently,
                 * but started simultaneously.
                 * Once the SYNC is broadcast onto the CAN bus, the motion will start.
                 ********************************************************************************/

                /** Save Previously configured profile for later... */
                uint8_t statusAll;
                uint8_t status;
                int32_t oldVel;
                int32_t oldAccel;
                int32_t oldDecel;
                std::tie(oldVel, status) = motor.getODpair(EPOS_OD_PROFILE_VELOCITY);
                statusAll = status;
                std::tie(oldAccel, status) = motor.getODpair(EPOS_OD_PROFILE_ACCELERATION);
                statusAll &= status;
                std::tie(oldDecel, status) = motor.getODpair(EPOS_OD_PROFILE_DECELERATION);
                statusAll &= status;

                /** Configure new profile */
                motor.sendRxPDO(RxPDO_Profile_Velocity, {120});     // 120 rpm
                motor.sendSDO(EPOS_OD_PROFILE_ACCELERATION, 60, 1); // 60 rpm per second
                motor.sendSDO(EPOS_OD_PROFILE_DECELERATION, 60, 1);

                motor.sendSDO(EPOS_OD_TARGET_POSITION, 4000, 1);

                /** set the new set point bit, configure other movement options... */
                motor.sendRxPDO(RxPDO_ControlWord_Synchronous,
                                {motor.setControlWordBits(
                                    {CW_BITS_NEW_SET_POINT, CW_BITS_ABS_OR_RELATIVE, CW_BITS_HALT},
                                    {true, true, false})});

                ESP_LOGI("SYNC Motion", "Move configured, waiting for Sync Object...");
                wait(5000);

                EPOS4::broadcastSync(); /**< broadcast SYNC onto CAN bus */
                ESP_LOGI("SYNC Motion", "Sent Sync, Motion Started");
                wait(50);
                motor.sendSDO(EPOS_OD_CONTROLWORD, motor.setControlWordBits({CW_BITS_NEW_SET_POINT}, {false})); /**< reset the 'new set point' bit */

                /** StatusWord is updated via asynchronous TxPDO. This bit is 1 when the motor has completed the motion */
                while (motor.getBitFromStatusWord(SW_BITS_TARGET_REACHED) != 1)
                {
                    motor.getODvalue(EPOS_OD_VELOCITY_ACTUAL_VALUES_VELOCITY_ACTUAL_VALUE_AVERAGED);
                    // Gets the velocity using SDO
                    motor.getODvalue(EPOS_OD_POSITION_ACTUAL_VALUE);
                    // As the position data arrives through an Async TxPDO, this command will not request it through SDO.
                    wait(100);
                    ESP_LOGI("SYNC Motion", "speed: %ld, position: %ld", motor.localOD(EPOS_OD_VELOCITY_ACTUAL_VALUE), motor.localOD(EPOS_OD_POSITION_ACTUAL_VALUE));
                }

                ESP_LOGI("SYNC Motion", "Motion Complete!");

                /** Return to previous profile, if the returned values were valid */
                if (statusAll & 0b1)
                {
                    ESP_LOGI("SYNC Motion", "Returning to Old Profile");
                    motor.sendRxPDO(RxPDO_Profile_Velocity, {oldVel});
                    motor.sendSDO(EPOS_OD_PROFILE_ACCELERATION, oldAccel, 1);
                    motor.sendSDO(EPOS_OD_PROFILE_DECELERATION, oldDecel, 1);
                }
                else
                {
                    ESP_LOGW("SYNC Motion", "Old Profile invalid");
                }

                /********************************************************************************
                 * Profile Position Mode Loop
                 ********************************************************************************/
                while (true)
                {
                    wait(3000);
                    ESP_LOGI("DEMO LOOP", "Moving...");
                    motor.moveToTargetPosition(500, true, true);
                    ESP_LOGI("DEMO LOOP", "Moved.");
                }
            }
        }
    }

    ESP_LOGE("DEMO", "Setup Failed!");

    while (true)
    {
        wait(1000)
    }
}