#ifndef __SM30_DEVICE_H__
#define __SM30_DEVICE_H__

#define  POINT_NUM  3  /* accuracy of pressure data*/

#include <rtthread.h>
#include <rtdevice.h>
#include <rtdef.h>

#include "sensor.h"

struct sm30_device
{
    rt_sensor_t sensor_p; 
    uint8_t update;    /*the time for update (extern device)*/
};
typedef struct sm30_device* sm30_dev_t;



/* sm3041 device hardware info */
#define SM3041_ADDR             0x28 	          /* i2c slave address */

/* sm3041 register value */
#define SM3041_MAXCOUNT         14745
#define SM3041_MINCOUNT         1638
#define SM3041_I2C_BUS          "i2c1"            /* i2c linked */ 
#define SM3041_DEVICE_NAME      "sm3041_sensor"   /* register sensor device name*/   
    
/* sm3041 device info for RT-Thread sensor device*/
#define SM3041_PRESSURE_MAX     34474
#define SM3041_PRESSURE_MIN     0
#define SM3041_PRESSURE_PERIOD  100  /*10ms */	  /* read ten times in 1 second */
   


rt_err_t sm3041_device_init(const char* name, struct rt_sensor_config* cfg);


#endif 
