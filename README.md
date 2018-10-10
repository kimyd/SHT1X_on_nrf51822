## How to read SHT1x sensor value on nrf51822


 SHT1x is not supporting I2C!! It looks like I2C protocol but not. If you want to use I2C support sensor, please use other chip. 

This example is how to read sensor value on nrf51822. 

Please refer to **read_sensor_value** function in **sht1x_measure.c**. Feel free to comment on this code. 

You need to define the following variable in the **main.c** file as global variables,

    static int sensor_temp;
    static int sensor_humid;

    static float sensor_temp_f;
    static float sensor_humid_f;

