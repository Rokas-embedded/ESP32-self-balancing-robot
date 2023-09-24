#include "./mpu6050.h"

#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define MPU6050 0x68
#define I2C_MASTER_TIMEOUT_MS 1000
#define ID_REG 0x75
#define ID_VALUE 104
#define PWR_MGMT_REG 0x6B
#define ACCEL_XOUT_H_REG 0x3B
#define GYRO_XOUT_H_REG 0x43

volatile float m_accelerometer_correction[3] = {
    0,0,1
};

volatile float m_gyro_correction[3] = {
    0,0,0
};

volatile int64_t m_previous_time = 0;
volatile float m_complementary_ratio = 0.0;

bool init_mpu6050(uint scl_pin, uint sda_pin, bool initialize_i2c, bool apply_calibration, float accelerometer_correction[3], float gyro_correction[3], float complementary_ratio){

    if(initialize_i2c){
        i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = sda_pin,
            .scl_io_num = scl_pin,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = I2C_MASTER_FREQ_HZ,
        };

        i2c_param_config(I2C_MASTER_NUM, &conf);
        i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    }

    if(apply_calibration){
        // assign the correction for gyro
        for (uint i = 0; i < 3; i++){
            m_gyro_correction[i] = gyro_correction[i];
        }

        // assign the correction for accelerometer
        for (uint i = 0; i < 3; i++){
            m_accelerometer_correction[i] = accelerometer_correction[i];
        }
    }

    m_complementary_ratio = complementary_ratio;

        // Test the sensor by reading it's address register
    uint8_t test_register[] = {ID_REG};
    uint8_t test_data[] = {0};

    i2c_master_write_read_device(
        I2C_MASTER_NUM, 
        MPU6050, 
        test_register, 
        1, 
        test_data, 
        1, 
        I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS
    );

    // Check if the id value is as it should be
    if (test_data[0] != ID_VALUE)
    {
        printf("MPU6050 initialization failed: %d != %d\n", test_data[0], ID_VALUE);
        return false;
    }

    // reset it 
    uint8_t reset_device1[] = {PWR_MGMT_REG, PWR_RESET|PWR_TEMP_DIS|PWR_CLOCK_INTERNAL_8MHZ};
    i2c_master_write_to_device(
        I2C_MASTER_NUM, 
        MPU6050, 
        reset_device1, 
        2, 
        I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS
    );
    vTaskDelay(100 / portTICK_RATE_MS);
    
    uint8_t reset_device2[] = {PWR_MGMT_REG, PWR_CLOCK_INTERNAL_STOP};
    i2c_master_write_to_device(
        I2C_MASTER_NUM, 
        MPU6050, reset_device2, 
        2,
        I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS
    );
    vTaskDelay(100 / portTICK_RATE_MS);

    uint8_t reset_device3[] = {PWR_MGMT_REG, PWR_TEMP_DIS|PWR_CLOCK_INTERNAL_8MHZ};
    i2c_master_write_to_device(
        I2C_MASTER_NUM, 
        MPU6050, 
        reset_device3, 
        2, 
        I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS
    );

    printf("MPU6050 initialized\n");
    return true;
}

void mpu6050_get_accelerometer_readings_gravity(float* data){
    uint8_t data_register[] = {ACCEL_XOUT_H_REG};
    uint8_t retrieved_data[] = {0,0,0,0,0,0};

    i2c_master_write_read_device(
        I2C_MASTER_NUM, 
        MPU6050, 
        data_register, 
        1, 
        retrieved_data, 
        6, 
        I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS
    );

    int16_t X = ((int16_t)retrieved_data[0] << 8) | (int16_t)retrieved_data[1];
    int16_t Y = ((int16_t)retrieved_data[2] << 8) | (int16_t)retrieved_data[3];
    int16_t Z = ((int16_t)retrieved_data[4] << 8) | (int16_t)retrieved_data[5];
    
    float X_out = ((float)X) / 16384.0;
    float Y_out = ((float)Y) / 16384.0;
    float Z_out = ((float)Z) / 16384.0;

    data[0] = X_out - (m_accelerometer_correction[0]);
    data[1] = Y_out - (m_accelerometer_correction[1]);
    data[2] = Z_out - (m_accelerometer_correction[2] - 1);
}

void mpu6050_get_gyro_readings_dps(float* data){
    uint8_t data_register[] = {GYRO_XOUT_H_REG};
    uint8_t retrieved_data[] = {0,0,0,0,0,0};

    i2c_master_write_read_device(
        I2C_MASTER_NUM, 
        MPU6050, 
        data_register, 
        1, 
        retrieved_data, 
        6, 
        I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS
    );

    int16_t X = ((int16_t)retrieved_data[0] << 8) | (int16_t)retrieved_data[1];
    int16_t Y = ((int16_t)retrieved_data[2] << 8) | (int16_t)retrieved_data[3];
    int16_t Z = ((int16_t)retrieved_data[4] << 8) | (int16_t)retrieved_data[5];
    
    float X_out = ((float)X) / 131.0;
    float Y_out = ((float)Y) / 131.0;
    float Z_out = ((float)Z) / 131.0;

    data[0] = X_out - (m_gyro_correction[0]);
    data[1] = Y_out - (m_gyro_correction[1]);
    data[2] = Z_out - (m_gyro_correction[2]);
}

void calculate_pitch_and_roll(float* data, float *roll, float *pitch){
    float x = data[0];
    float y = data[1];
    float z = data[2];

    float acc_vector_length = sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
    x = x / acc_vector_length;
    y = y / acc_vector_length;
    z = z / acc_vector_length;

    // rotation around the x axis
    *roll = atan2f(y, z) * (180 / M_PI);

    // rotation around the y axis
    *pitch = -(asinf(x) * (180 / M_PI));
    // i put a minus on the pitch calculation as i have found that the pitch has opposite values of actual
}

void calculate_degrees_x_y(float *data, float *rotation_around_x, float *rotation_around_y)
{
    float x = data[0];
    float y = data[1];
    float z = data[2];

    *rotation_around_x = atan2f(y, sqrtf(x * x + z * z)) * (180.0 / M_PI);
    // added minus to match actual dps direction the values are supposed to go
    *rotation_around_y = -(atan2f(x, sqrtf(y * y + z * z)) * (180.0 / M_PI));
}

void find_accelerometer_error(uint64_t sample_size)
{
    float x_sum = 0, y_sum = 0, z_sum = 0;
    float data[] = {0, 0, 0};

    for (uint64_t i = 0; i < sample_size; i++)
    {
        mpu6050_get_accelerometer_readings_gravity(data);
        x_sum += data[0];
        y_sum += data[1];
        z_sum += data[2];
        // It can sample stuff at 1KHz
        // but 0.5Khz is just to be safe
        vTaskDelay(2 / portTICK_RATE_MS);
    }

    printf(
        "ACCELEROMETER errors:  X,Y,Z  %f, %f, %f\n",
        x_sum / sample_size,
        y_sum / sample_size,
        (z_sum / sample_size) -1 );
}

void find_gyro_error(uint64_t sample_size)
{

    float x_sum = 0, y_sum = 0, z_sum = 0;
    float data[] = {0, 0, 0};

    for (uint64_t i = 0; i < sample_size; i++)
    {
        mpu6050_get_gyro_readings_dps(data);

        x_sum += data[0];
        y_sum += data[1];
        z_sum += data[2];
        // It can sample stuff at 1KHz
        // but 0.5Khz is just to be safe
        vTaskDelay(2 / portTICK_RATE_MS);
    }

    printf(
        "GYRO errors:  X,Y,Z  %f, %f, %f\n",
        x_sum / sample_size,
        y_sum / sample_size,
        z_sum / sample_size);
}

void convert_angular_rotation_to_degrees(float* gyro_angular, float* gyro_degrees, float rotation_around_x, float rotation_around_y, float rotation_around_z, int64_t time){
    // Convert angular velocity to actual degrees that it moved and add it to the integral (dead reckoning not PID)


    if(m_previous_time == 0){
        m_previous_time = time;
        return;
    }

    double time_passed_proportion = (((double)time/1000.0)-((double)m_previous_time/1000.0)) / 1000.0;
    m_previous_time = time;

    // printf(" time %6.10f ",time_passed_proportion);

    gyro_degrees[0] += (gyro_angular[0] * time_passed_proportion);
    gyro_degrees[1] += (gyro_angular[1] * time_passed_proportion);
    gyro_degrees[2] += (gyro_angular[2] * time_passed_proportion);

    // Apply complimentary filter
    // I have no idea why this version works. I tried the actually implementation and it was
    // not working good. This one though works very good.
    gyro_degrees[0] = -(-gyro_degrees[0] * (1.0 - m_complementary_ratio) - (m_complementary_ratio * rotation_around_x));
    gyro_degrees[1] = -(-gyro_degrees[1] * (1.0 - m_complementary_ratio) - (m_complementary_ratio * rotation_around_y));
    gyro_degrees[2] = -(-gyro_degrees[2] * (1.0 - m_complementary_ratio) - (m_complementary_ratio * rotation_around_z));

    // I dont want to track how many times the degrees went over the 360 degree mark, no point.
    while (gyro_degrees[0] > 180.0) {
        gyro_degrees[0] -= 360.0;
    }
    while (gyro_degrees[0] < -180.0) {
        gyro_degrees[0] += 360.0;
    }

    while (gyro_degrees[1] > 180.0) {
        gyro_degrees[1] -= 360.0;
    }
    while (gyro_degrees[1] < -180.0) {
        gyro_degrees[1] += 360.0;
    }

    while (gyro_degrees[2] > 180.0) {
        gyro_degrees[2] -= 360.0;
    }
    while (gyro_degrees[2] < -180.0) {
        gyro_degrees[2] += 360.0;
    }
}