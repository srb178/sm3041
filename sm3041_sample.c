#include  "sm3041_sample.h"

#define  DBG_TAG     "sm3041.sample"
#ifdef   RT_I2C_DEBUG
#define  DBG_LVL     DBG_LOG
#else 
#define  DBG_LVL     DBG_INFO
#endif

static void sm3041_read_entry(void *parameter)
{    
    rt_device_t sm30_dev = RT_NULL;   
    struct rt_sensor_data sm30_data[2];   
    rt_size_t res30 = 0;
 
    float sm3041_p, sm3041_t; /* transform buf */
    
    /* find have been registered device*/
	sm30_dev = rt_device_find("baro_sm3041_sensor");     
    if (sm30_dev == RT_NULL)
    {
	  	LOG_E("not found any sm3041 device!\r\n");       
        return;
    }
    else
    {
        LOG_I("already found sm3041 device! \r\n");
    }
    
    /* open found sensor */
    if ( rt_device_open(sm30_dev, RT_DEVICE_FLAG_RDWR) != RT_EOK )       
    {
        LOG_E("open sm3041 device failed! \r\n");
        return;
    }
    else
    {
        LOG_I("open sm3041 device succeed! \r\n");
    }
	
    
    while(1)
    {
        /* sm3041 read data */
        res30 = rt_device_read(sm30_dev, 0, &sm30_data, 2);
        if(res30 == 0)
        {
            LOG_E("sm3041 read data failed! result is %d \n", res30);
            rt_device_close(sm30_dev);
            return;
        }
        else
        {       
            sm3041_p = sm30_data[0].data.baro;
            sm3041_t = sm30_data[1].data.temp;
            LOG_I("sm3041 fetch pressure is %f, %d\r\n",
                   sm3041_p / 1000,        /*POINT_NUM = 3*/ 
                   sm30_data[0].timestamp);
            LOG_I("sm3041 fetch temperature is %f, %d\r\n", 					          
                   sm3041_t / 100,
                   sm30_data[1].timestamp);
        }
        rt_thread_delay(500);        
    }
}


static int sm3041_read_sample(void)
{
    rt_thread_t sm3041_thread;

    sm3041_thread = rt_thread_create("sm30_tid",
                                      sm3041_read_entry,
                                      RT_NULL,
                                      1024,
                                      RT_THREAD_PRIORITY_MAX / 2,
                                      20);
    if (sm3041_thread != RT_NULL)
    {
        rt_thread_startup(sm3041_thread);
        rt_kprintf("sm3041 thread is running! \r\n");
    }

    return RT_EOK;
}
MSH_CMD_EXPORT(sm3041_read_sample, sm3041 read sample);
