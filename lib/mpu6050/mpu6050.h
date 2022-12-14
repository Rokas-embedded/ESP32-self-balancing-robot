#pragma once

#include <stdio.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/uart.h>
#include <string.h>
#include "driver/i2c.h"
#include <math.h>
// Needs 3.6V
// Needs to be connected to i2c 

void init_mpu6050(uint scl_pin, uint sda_pin, bool initialize_i2c, bool apply_calibration, float accelerometer_correction_temp[3], float gyro_correction_temp[3]);
// void init_mpu6050(uint scl_pin, uint sda_pin, bool initialize_i2c);
void mpu6050_accelerometer_readings_float(float* data);
void mpu6050_gyro_readings_float(float* data);
void calculate_pitch_and_roll(float* data, float *roll, float *pitch);
void find_accelerometer_error(uint sample_size);
void find_gyro_error(uint sample_size);