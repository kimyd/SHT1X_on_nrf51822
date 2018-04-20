
/**
* @dev print out temperature value from SHT1x
* @param p_temperature temperature value 
**/
static void cal_sth10_temp(float *p_temperature)
{ const float C1=-2.0468;           // for 12 Bit RH
  const float C2=+0.0367;           // for 12 Bit RH
  const float C3=-0.0000015955;     // for 12 Bit RH
  const float T1=+0.01;             // for 12 Bit RH
  const float T2=+0.00008;          // for 12 Bit RH	

  float t=sensor_temp;           // t:       Temperature [Ticks] 14 Bit
  float t_C;                        // t_C   :  Temperature 

  t_C=t*0.01 - 39.6;                //calc. temperature [캜] from 14 bit temp. ticks @ 5V
#ifdef DEBUG
//	printf("[cal_sth10_temp] ......%f--> %f \n",t, t_C);
#endif
  *p_temperature=t_C;               //return temperature 
}



/**
* @dev print out humidity value from SHT1x
* @param p_humidity humidity value
* @param t_C temperature value in Celsius
**/
static void cal_sth10_humid(float *p_humidity, float t_C)
{ const float C1=-2.0468;           // for 12 Bit RH
  const float C2=0.0367;           // for 12 Bit RH
  const float C3=-0.0000015955;     // for 12 Bit RH
  const float T1=0.01;             // for 12 Bit RH
  const float T2=0.00008;          // for 12 Bit RH	

  float rh=sensor_humid;             // rh:      Humidity [Ticks] 12 Bit 
  float rh_lin;                     // rh_lin:  Humidity linear
  float rh_true;                    // rh_true: Temperature compensated humidity


  rh_lin=C3*rh*rh + C2*rh + C1;     //calc. humidity from ticks to [%RH]
  rh_true=(t_C-25)*(T1+T2*rh)+rh_lin;   //calc. temperature compensated humidity [%RH]
  if(rh_true>99)rh_true=100;       //cut if the value is outside of
  if(rh_true<0.1)rh_true=0.1;       //the physical possible range

#ifdef DEBUG
//	printf("[cal_sth10_humid]....t_C:%f..%f--> %f \n",t_C, rh, rh_lin);
#endif
	*p_humidity=rh_true;              //return humidity[%RH]
}


/**
* @dev  writes a byte on the Sensibus and checks the acknowledge 
**/
static char s_write_byte(unsigned char value)
{ 
  unsigned char i,error=0;  
	nrf_gpio_cfg_output(SHT_DATA);
	
	//shift bit for masking
  for (i=0x80;i>0;i/=2){             
    if (i & value) { 
			nrf_gpio_pin_set(SHT_DATA); 
		} else {
			nrf_gpio_pin_clear(SHT_DATA);  
		}
    nrf_gpio_pin_set(SHT_SCK);   
		__NOP(); __NOP();
    nrf_gpio_pin_clear(SHT_SCK);
  }
	
  nrf_gpio_pin_set(SHT_DATA);     
  __NOP();                          //observe setup time
  nrf_gpio_pin_set(SHT_SCK);                            //clk #9 for ack 
	nrf_gpio_cfg_input(SHT_DATA, NRF_GPIO_PIN_PULLUP);
	

	error = nrf_gpio_pin_read(SHT_DATA);	
	nrf_gpio_pin_clear(SHT_SCK);  

	
  return error;                     //error=1 in case of no acknowledge
}



/**
* @dev  reads a byte form the Sensibus and gives an acknowledge in case of "ack=1" 
**/
static char s_read_byte(unsigned char ack)
{ 
  unsigned char i,val=0;
	nrf_gpio_cfg_output(SHT_DATA);
  nrf_gpio_pin_set(SHT_DATA);                           //release DATA-line
	nrf_gpio_cfg_input(SHT_DATA, NRF_GPIO_PIN_PULLUP);
	
  //shift bit for masking
	for (i=0x80;i>0;i/=2) {              
    nrf_gpio_pin_set(SHT_SCK);                          //clk for SENSI-BUS
    if (nrf_gpio_pin_read(SHT_DATA)) {
			val=(val | i);        //read bit
		}			
    nrf_gpio_pin_clear(SHT_SCK); 					 
  }
	
	// send ack or not
	nrf_gpio_cfg_output(SHT_DATA);
	if(ack){
		nrf_gpio_pin_clear(SHT_DATA);	
	} else {
		nrf_gpio_pin_set(SHT_DATA);	
	}		
  nrf_gpio_pin_set(SHT_SCK);                           
  __NOP();__NOP();__NOP();          //pulswith approx. 5 us 
	nrf_gpio_pin_clear(SHT_SCK);			    
  nrf_gpio_pin_set(SHT_DATA);                           //release DATA-line
	return val;
}

/**
* @dev  generates a transmission start 
* @dev      _____         ________
* @dev DATA:      |_______|
* @dev           ___     ___
* @dev SCK : ___|   |___|   |______
**/
void s_transstart(void)

{  
	unsigned char val;
	nrf_gpio_pin_set(SHT_DATA); nrf_gpio_pin_clear(SHT_SCK);                   //Initial state
	__NOP();
	nrf_gpio_pin_set(SHT_SCK);
	__NOP();
	nrf_gpio_pin_clear(SHT_DATA);
	__NOP();
	nrf_gpio_pin_clear(SHT_SCK);  
	__NOP();__NOP();__NOP();
	nrf_gpio_pin_set(SHT_SCK);
	__NOP();
	nrf_gpio_pin_set(SHT_DATA);		   
	__NOP();
	nrf_gpio_pin_clear(SHT_SCK);   
}


/**
* @dev  communication reset: DATA-line=1 and at least 9 SCK cycles followed by transstart
* @dev  DATA:                                                      |_______|
* @dev           _    _    _    _    _    _    _    _    _        ___     ___
* @dev  SCK : __| |__| |__| |__| |__| |__| |__| |__| |__| |______|   |___|   |______
**/
void s_connectionreset(void)
{  
  unsigned char i; 
  nrf_gpio_pin_set(SHT_DATA); nrf_gpio_pin_clear(SHT_SCK);                    //Initial state
  for(i=0;i<9;i++)                  //9 SCK cycles
  { nrf_gpio_pin_set(SHT_SCK);
    nrf_gpio_pin_clear(SHT_SCK);
  }
  s_transstart();                   //transmission start
}

/**
* @dev  makes a measurement (humidity/temperature) with checksum
**/
char s_measure(unsigned char *p_value, unsigned char *p_checksum, unsigned char mode)
{ 
  unsigned char error=0;
  unsigned int i, ack;
	
  s_transstart();                   //transmission start
  switch(mode){                     //send command to sensor
    case TEMP	: error+=s_write_byte(MEASURE_TEMP); break;
    case HUMI	: error+=s_write_byte(MEASURE_HUMI); break;
    default     : break;	 
  }
	//--------------- wait for result ----------
	nrf_gpio_cfg_input(SHT_DATA, NRF_GPIO_PIN_PULLUP);
	for (i=0 ; i<100 ; i++){
		nrf_delay_ms(10);
		ack = nrf_gpio_pin_read(SHT_DATA);
		if(ack == 0) break;
	}
	if(ack){
#ifdef DEBUG
//		printf("[s_measure] Time out error\n");
#endif	
	}

  *(p_value)  =s_read_byte(ACK);    //read the first byte (MSB)
  *(p_value+1)=s_read_byte(ACK);    //read the second byte (LSB)
  *p_checksum =s_read_byte(noACK);  //read checksum

	switch(mode){
		case TEMP : 
			sensor_temp = *(p_value)*256;
			sensor_temp |= *(p_value+1);
			break;
		case HUMI : 
			sensor_humid = *(p_value)*256;
			sensor_humid |= *(p_value+1);
			break;
	}
	printf("[s_measure] p_value: [%02x %02x] error return value : %02x\n", *(p_value), *(p_value+1), error);
	printf("[s_measure] checksum : %02x \n", *p_checksum);
  return error;
}

/**
* @dev  writes the status register with checksum (8-bit)
**/
static char s_write_statusreg(unsigned char *p_value)
{ 
  unsigned char error=0;
  s_transstart();                   //transmission start
  error+=s_write_byte(STATUS_REG_W);//send command to sensor
  error+=s_write_byte(*p_value);    //send value of status register
  return error;                     //error>=1 in case of no response form the sensor
}


/**
* @dev reads the status register with checksum (8-bit)
**/
static char s_read_statusreg(unsigned char *p_value, unsigned char *p_checksum)
{ 
  unsigned char error=0;
  s_transstart();                   //transmission start
  error=s_write_byte(STATUS_REG_R); //send command to sensor
  *p_value=s_read_byte(ACK);        //read status register (8-bit)
  *p_checksum=s_read_byte(noACK);   //read checksum (8-bit)  
  return error;                     //error=1 in case of no response form the sensor
}


/**
* @dev resets the sensor by a softreset 
**/
static char s_softreset(void)
{ 
  unsigned char error=0;  
  s_connectionreset();              //reset communication
  error+=s_write_byte(RESET);       //send RESET-command to sensor
  return error;                     //error=1 in case of no response form the sensor
}


/**
* @dev read temperature and humidity from SHT1x sensor
**/
static char read_sensor_value(void)
{ 
  value humi_val,temp_val;
  float dew_point;
  unsigned char error,checksum;
  unsigned int i;

  s_connectionreset();
  
  { error=0;
    error+=s_measure((unsigned char*) &humi_val.i,&checksum,HUMI);  //measure humidity
    error+=s_measure((unsigned char*) &temp_val.i,&checksum,TEMP);  //measure temperature
    if(error!=0) { 			
			s_connectionreset();                 //in case of an error: connection reset
    } else { 
			humi_val.f=(float)sensor_temp;                   //converts integer to float
			temp_val.f=(float)temp_val.i;                   //converts integer to float
			cal_sth10_temp(&temp_val.f);
			cal_sth10_humid(&humi_val.f, temp_val.f);
#ifdef DEBUG			
      printf("temp:%5.1fC humi:%5.1f%% \n",temp_val.f,humi_val.f);
#endif
    }
    //----------wait approx. 0.8s to avoid heating up SHTxx------------------------------      
//    nrf_delay_ms(700);
    //-----------------------------------------------------------------------------------                       
  }
	
	sensor_temp_f = temp_val.f;
	sensor_humid_f = humi_val.f;
}
