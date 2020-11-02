#include "sm30_device.h"

#define DBG_TAG  "sm30.dev"
#define DBG_LVL   DBG_LOG
#include <rtdbg.h>


static double pressure3041;             /* actual pressure */ 
static double temperature3041;          /* actual temperature */


/*******************************************************************************
Fuc: sm30 sensor read the register
*******************************************************************************/ 
static rt_err_t sm30_read_regs(rt_sensor_t psensor, rt_uint8_t *data, rt_uint8_t data_size)
{
    struct rt_i2c_msg msg[1];
    struct rt_i2c_bus_device  *i2c_bus = RT_NULL;
    rt_uint32_t slave_addr = 0;

    /* get i2c slave address */
    slave_addr = (rt_uint32_t)psensor->config.intf.user_data; 
    /* get i2c bus device */   
    i2c_bus = (struct rt_i2c_bus_device *)psensor->parent.user_data; /*void *user_data; device private data */
    
    
    msg[0].addr =(rt_uint8_t)slave_addr;
    msg[0].flags = RT_I2C_RD;
    msg[0].buf = data;    /*pointer for read the data buffer*/
    msg[0].len = data_size;
    
    /*return for the number of message if succeeded*/
    if(rt_i2c_transfer(i2c_bus, msg, 1) == 1) 
    {
        return RT_EOK;
    }
    else
    {
        LOG_E("i2c1 bus read failed!\r\n");
        return RT_ERROR;
    } 
}

/*******************************************************************************
Fuc: sm30 sensor read for original data
*******************************************************************************/  
static rt_err_t sm30_read_adc(rt_sensor_t psensor, double* p3041, double* t3041)
{
    rt_uint8_t buf30_read[4] = {0};
    rt_uint8_t status;
    rt_err_t ans;
    float dat = 0.0f;
    rt_int32_t pressure_adc30;       /* pressure adc */
    rt_int32_t temperature_adc30;    /* temperature adc */
    
    ans = sm30_read_regs(psensor, buf30_read, 4);  
    if(ans == RT_EOK)
    {
        status = buf30_read[0]>>6;
        if(status == 0)
        {
            pressure_adc30 = ((buf30_read[0]&0x3f)<<8) | buf30_read[1];
            temperature_adc30 = (buf30_read[2])<<3 | buf30_read[3]>>5;
       
            dat = ( psensor->info.range_max - psensor->info.range_min);
            *p3041 = (pressure_adc30 - SM3041_MINCOUNT)* dat/ (SM3041_MAXCOUNT - SM3041_MINCOUNT) + psensor->info.range_min;
            *t3041 = (float)temperature_adc30*200/2047 - 50;
            return RT_EOK;
        } 
    }
    return RT_ERROR;
}


/*******************************************************************************
Fuc: sm30 sensor read for actual P/T data
*******************************************************************************/ 
static rt_size_t sm30_fetch_data(struct rt_sensor_device *psensor, void *buf, rt_size_t len )
{
    RT_ASSERT(buf);
    RT_ASSERT(psensor);   
    struct rt_sensor_data *sensor_data = buf;   
   
    if(len != 0)
    {
         /* One shot only read a data */
        if(psensor->config.mode == RT_SENSOR_MODE_POLLING) 
        {
            if(psensor->info.type == RT_SENSOR_CLASS_BARO)
            {
                /* actual pressure */            
                if(sm30_read_adc(psensor, &pressure3041, &temperature3041) == RT_EOK)
                {
                    sensor_data->type = RT_SENSOR_CLASS_BARO;
                    if(len == 1)
                    {                         
                        if(POINT_NUM == 4)
                        {
                            sensor_data->data.baro = pressure3041*10000;
                        }
                        else if(POINT_NUM == 3)
                        {
                            sensor_data->data.baro = pressure3041*1000;
                        }
                        else if(POINT_NUM == 2)
                        {
                            sensor_data->data.baro = pressure3041*100;
                        }
                        sensor_data->timestamp = rt_sensor_get_ts();
                        //LOG_I("sm3041 fetch data finished! \r\n");
                    }
                    else if(len == 2)
                    {                     
                        if(POINT_NUM == 4)
                        {
                            sensor_data->data.baro = pressure3041*10000;
                        }
                        else if(POINT_NUM == 3)
                        {
                            sensor_data->data.baro = pressure3041*1000;
                        }
                        else if(POINT_NUM == 2)
                        {
                            sensor_data->data.baro = pressure3041*100;
                        }
                        sensor_data->timestamp = rt_sensor_get_ts();
                        
                        sensor_data++;
                        sensor_data->type = RT_SENSOR_CLASS_TEMP;                
                        sensor_data->data.temp = (temperature3041*100); 
                        sensor_data->timestamp = rt_sensor_get_ts();
                        //LOG_I("sm3041 fetch data finished! \r\n");
                    }   
                    else
                    {
                        LOG_E("input para 'len' illefal! \r\n");
                    }
                                    
                }
                else
                {
                    LOG_W("sm3041 fetch adc data wrong!\r\n");
                    return 0;
                }        
            }
        }       
    }
    else
    {
        LOG_W(" input 'len' cannot be empty! \r\n");
    }
    return 1;
}

/*******************************************************************************
Fuc: sm30 sensor control
*******************************************************************************/ 
static rt_err_t sm30_control(struct rt_sensor_device *psensor, int cmd, void *args)
{
    rt_err_t ret = RT_EOK;

    return ret;      
}

static struct rt_sensor_ops  sm30_sensor_ops =
{
    sm30_fetch_data,
    sm30_control,
};


/*******************************************************************************
Fuc: initialize sm3041 sensor
*******************************************************************************/
rt_err_t  sm3041_device_init(const char* name, struct rt_sensor_config *  cfg )
{
    rt_err_t ret = RT_EOK;
    rt_sensor_t  sensor_temp = RT_NULL; /*apply for the memory of sensor class*/           
    struct rt_i2c_bus_device *sm30_i2c_bus = RT_NULL;
    
    sm30_i2c_bus = rt_i2c_bus_device_find(cfg->intf.dev_name);
    if(sm30_i2c_bus == RT_NULL)
    {
        LOG_E("i2c1 bus device %s not found! \r\n", cfg->intf.dev_name);
        ret = -RT_ERROR;
        goto _exit;        
    }

    /*sm3041 sensor register*/
    {
        sensor_temp = rt_calloc(1, sizeof(struct rt_sensor_device));
        if(sensor_temp == RT_NULL)
        {
            goto _exit;
        }
        rt_memset(sensor_temp, 0x0, sizeof(struct rt_sensor_device));
        
        sensor_temp->info.type = RT_SENSOR_CLASS_BARO;
        sensor_temp->info.vendor = RT_SENSOR_VENDOR_UNKNOWN;
        sensor_temp->info.model = "sm3041";
        sensor_temp->info.unit = RT_SENSOR_UNIT_PA;
        sensor_temp->info.intf_type = RT_SENSOR_INTF_I2C;
        sensor_temp->info.range_max = SM3041_PRESSURE_MAX;
        sensor_temp->info.range_min = SM3041_PRESSURE_MIN;
        
        /* read [1000/SM3041_PRESSURE_PERIOD] times in 1 second */
        sensor_temp->info.period_min = SM3041_PRESSURE_PERIOD;
     
        rt_memcpy( &sensor_temp->config, cfg, sizeof(struct rt_sensor_config) );
        sensor_temp->ops = &sm30_sensor_ops;
        
        /* underlying register */
        ret = rt_hw_sensor_register(sensor_temp, name, RT_DEVICE_FLAG_RDWR, (void *)sm30_i2c_bus/* private data*/);
        if(ret != RT_EOK)
        {
            LOG_E( "sm3041 device register err code: %d", ret );
            goto _exit;
        }
        else
        {
            LOG_I("sm3041 register succeeded! \r\n");
        }
    }
    
    return RT_EOK;
    
    _exit:
        if(sensor_temp)
        {
            rt_free(sensor_temp);
        }                    
        return ret;
}


#ifdef   PKG_USING_SM30_SENSOR
static int rt_hw_sm3041_port(void)
{
    struct rt_sensor_config cfg;
        
    cfg.intf.dev_name = SM3041_I2C_BUS;            /* i2c bus */
    cfg.intf.user_data = (void *)SM3041_ADDR;      /* i2c slave addr */
    sm3041_device_init(SM3041_DEVICE_NAME, &cfg);  /* sm3041 */

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_hw_sm3041_port);  
#endif   /*PKG_USING_SM30_SENSOR*/
