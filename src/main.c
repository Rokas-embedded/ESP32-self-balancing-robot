#include <stdio.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "../lib/esp_01/esp_01.h"
#include "../lib/adxl345/adxl345.h"
#include "../lib/TB6612/TB6612.h"

// ESP32 DevkitC v4 // ESP-WROOM-32D
// 160 Mhz
// Watchdog for tasks is disabled. 

// char *wifi_name = "Stofa70521";
// char *wifi_password = "bis56lage63";
// char *server_ip = "192.168.87.178";
// char *server_port = "3500";

char *wifi_name = "ESP32_wifi";
char *wifi_password = "1234567890";
char *server_ip = "192.168.8.1";
char *server_port = "3500";

void init_lights(){
        gpio_set_direction(GPIO_NUM_32, GPIO_MODE_OUTPUT);
        gpio_set_direction(GPIO_NUM_33, GPIO_MODE_OUTPUT);
        gpio_set_direction(GPIO_NUM_25, GPIO_MODE_OUTPUT);
        gpio_set_direction(GPIO_NUM_26, GPIO_MODE_OUTPUT);
}

void extract_request_values(char *request, uint request_size, uint *x, uint *y){
    uint x_index = 1;
    uint x_end_index = 0;

    for(uint i = 1; i < request_size; i++){
        if(request[i] == '/'){
            x_end_index = i;
            break;
        }
    }

    uint y_index = x_end_index+1;
    uint y_end_index = request_size-1;

    uint x_length = x_end_index - x_index;
    char x_substring[x_length+1];
    strncpy(x_substring, &request[x_index], x_length);
    x_substring[x_length] = '\0';
    // printf("X IS : %s\n", x_substring);
    *x = atoi(x_substring);

    uint y_length = y_end_index - y_index+1;
    char y_substring[y_length+1];
    strncpy(y_substring, &request[y_index], y_length);
    y_substring[y_length] = '\0';
    // printf("Y IS : %s\n", y_substring);
    *y = atoi(y_substring);

}

void manipulate_leds(uint x, uint y){

    // blue - 32 low y
    // red - 33 high x
    // yellow - 25 high y
    // green - 26 low x


    // 32 up 25 down
    if(x < 40){
        gpio_set_level(GPIO_NUM_26, 1);
        gpio_set_level(GPIO_NUM_33, 0);
    }else if(x > 60){
        gpio_set_level(GPIO_NUM_33, 1);
        gpio_set_level(GPIO_NUM_26, 0);
    }else{
        gpio_set_level(GPIO_NUM_26, 0);
        gpio_set_level(GPIO_NUM_33, 0);
    }

    // 33 left 26 right
    if(y < 40){
        gpio_set_level(GPIO_NUM_32, 1);
        gpio_set_level(GPIO_NUM_25, 0);
    }else if(y > 60){
        gpio_set_level(GPIO_NUM_25, 1);
        gpio_set_level(GPIO_NUM_32, 0);
    }else{
        gpio_set_level(GPIO_NUM_32, 0);
        gpio_set_level(GPIO_NUM_25, 0);
    }
}


void manipulate_motors(uint x, uint y){
    
    // printf("Value of y: %d\n", (int) y);
    // change_speed_motor_B(-60);
    if(y < 40){
        // printf("1 The value is: %d\n", ((int)y)*2*-1);
        change_speed_motor_B(((int)y-50)*2);
    }else if(y > 60){
        // printf("2 The value is: %d\n", (((int)y)-50)*2);
        change_speed_motor_B(((int)y-50)*2);
    }else{
        change_speed_motor_B(0);
        // printf("3 The value is: 0\n");
    }

    if(x < 40){
        change_speed_motor_A(((int)x-50)*2);
    }else if(x > 60){
        change_speed_motor_A(((int)x-50)*2);
    }else{
        change_speed_motor_A(0);
        // printf("The value is: 0");
    }
}

//
void app_main() {
    printf("STARTING PROGRAM\n");
    // status led
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_2, 1);

    // init_lights();

    init_esp_01_server(UART_NUM_2, GPIO_NUM_19, wifi_name, wifi_password, server_port, server_ip, false);

    init_adxl345(22,21);

    // gpio_set_direction(GPIO_NUM_26, GPIO_MODE_OUTPUT);
    // gpio_set_level(GPIO_NUM_26, 1);

    // gpio_set_direction(GPIO_NUM_27, GPIO_MODE_OUTPUT);
    // gpio_set_level(GPIO_NUM_27, 1);

    // init_TB6612(12, 14, 27, 18, 5, 4);
    // 32, 33, 25, 26, 27, 14
    init_TB6612(GPIO_NUM_32, GPIO_NUM_33, GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_14);
    change_speed_motor_A(-40);
    change_speed_motor_B(40);

    // int16_t data_int[] = {0,0,0};
    double data_float[] = {0,0,0};

    double roll = 0;
    double pitch = 0;

    // uint x = 50;
    // uint y = 50;
    
    
    uint x = 50;
    uint y = 50;
    
    printf("\n\n====START OF LOOP====\n\n");

    while (true){
        char buffer[1024];
        uint result = esp_01_server_IPD(UART_NUM_2, "HTTP/1.1", 1000, buffer, false);

        if(result > 0){
            uint connection_id = 999;
            uint request_size = 999;
            char *request = esp_01_trim_response(buffer, 1024, &connection_id, &request_size);
            esp_01_server_OK(UART_NUM_2, connection_id);
            extract_request_values(request, request_size, &x, &y);
            free(request);
            printf("Transmission x:%d y:%d\n", x, y);
        }
        // use the information to set the leds 
        // manipulate_leds(x, y);
        manipulate_motors(x, y);
        
        // accelerometer
        // adxl345_get_axis_readings_int(data_int);
        // printf("X= %d Y= %d Z= %d\n", data_int[0], data_int[1], data_int[2]);
        
        // adxl345_get_axis_readings_float(data_float);
        // printf("floats X= %.4f Y= %.4f Z= %.4f\n", data_float[0], data_float[1], data_float[2]);
        // calculate_pitch_and_roll(data_float, &roll, &pitch);
        // printf("Roll: %.2f   Pitch %.2f\n", roll, pitch);
        // printf("\n");
        printf("\n");

        // status led
        gpio_set_level(GPIO_NUM_2, 1);
        // gpio_set_level(GPIO_NUM_27, 1);
        // gpio_set_level(GPIO_NUM_26, 1);
        vTaskDelay(30 / portTICK_RATE_MS);
        gpio_set_level(GPIO_NUM_2, 0);
        // gpio_set_level(GPIO_NUM_27, 0);
        // gpio_set_level(GPIO_NUM_26, 0);
        vTaskDelay(30 / portTICK_RATE_MS);
    }
}