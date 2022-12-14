#include "FIFO.h"
//#include "ADC_Driver.h"
#include "EEPROM_Driver_EXT.h"
//#include "TIMER_Driver.h"
#include "CAN_Driver.h"

#include "calibration.h"
//#include "event.h"
//#include "led.h"

//
extern uint8 act_SensorID;
extern struct_Sensor Sensors[4];
extern struct_D_Sensor D_Sensor;
extern uint16 CSI_config_ID;
extern uint32 CSI_config_str;
extern uint16 CSI_config_version;
extern uint16 LED_timeout_counter;
//
uint8 SentOut_Config = FALSE;
uint8 SentOut_Config_Counter;
uint8 Reset_TIMER2 = FALSE;
uint8 Get_CFG_Sensor_MUX;


//converting the ADC values to voltage values
double ADC_convert(uint16_t ADC_value)
{
	double Voltage = 0;
    //volatile uint16* ADC16Ptr = &ADCBUF1;
 //   uint8_t counter;

    //12-bit ADC with a 3.3 Reference voltage for STM32F7 board
    Voltage = (double)(3.3/4096) * ADC_value;

    return Voltage;
}






//void HostCANmsg(struct_CAN_Message msg)
//{
//    if (msg.CAN_ID==ID_GET_config)
//    {
//        uint16 CSI_MUX;
//        uint16 Mode_MUX;
//        uint16 Sensor_MUX;
//
//        //struct_CAN_Message tmpMsg;
//        //tmpMsg.CAN_ID = 0x01;
//
//        CSI_MUX = (msg.CAN_Word1 & 0x000F);        //Bit 0-3
//        Mode_MUX = (msg.CAN_Word1 & 0x00F0) >> 4;  //Bit 4-7
//        Sensor_MUX = (msg.CAN_Word1 & 0x0F00) >> 8;//Bit 8-11
//
//        //tmpMsg.CAN_Word1 = CSI_MUX;
//        //tmpMsg.CAN_Word2 = Mode_MUX;
//        //tmpMsg.CAN_Word3 = Sensor_MUX;
//
//      //  send_CAN(tmpMsg);
//
//        if(CSI_MUX == CSI_ID)
//        {
//            switch (Mode_MUX)
//            {
//                case MODE_NORMAL:
//                    break;
//                case MODE_GET_CFG:
//
//                    if(Sensor_MUX < 4)
//                    {
//                        read_EEPROM(&Sensors[Sensor_MUX]);
//                    }
//                    else if (Sensor_MUX==4)
//                    {
//                        read_D_EEPROM(&D_Sensor);
//                    }
//                    else if (Sensor_MUX==5)
//                    {
//                        read_config_ID_EEPROM(&CSI_config_ID, &CSI_config_str, &CSI_config_version);
//                    }
//
//                    Get_CFG_Sensor_MUX = Sensor_MUX;
//                    Timer2_Start();
//                    break;
//                case MODE_SET_CFG:
//                    CSI_set_Config(msg);
//                    break;
//                case MODE_REQ_VOLTS:
//                    if (Sensor_MUX >= 0 && Sensor_MUX <= 3)
//                    {
//                        msg.CAN_ID = ID_REQ_VOLTS; //ID current voltage
//                        msg.CAN_Word1 = Sensor_MUX; //Sensor mux
//                        msg.CAN_Word2 = Sensors[Sensor_MUX].voltage; //analog digital wert
//                        msg.CAN_Word3 = 0x0;
//                        msg.CAN_Word4 = 0x0;
//
//                        send_CAN(msg);
//                    }
//                    break;
//                default:
//                    break;
//            }
//        }
//    } else if (msg.CAN_ID == ID_SWITCH_5V_SW) {
//        LED_timeout_counter = 0;
//        if(msg.CAN_Word1&0x01) {
//            //LED_CSI_ON();
//            LED_OFF(LED_SW_PRT, LED_SW); //inverse logic - p fet
//        } else {
//            //LED_CSI_OFF();
//            LED_ON(LED_SW_PRT, LED_SW);
//        }
//    }
//}

void CSI_get_Config()
{
    static uint8 GridPoint_MUX = 0;
    static struct_CAN_Message msg;

    msg.CAN_ID = ID_CONFIG_CSI;
    msg.CAN_Word1 = 0x0;
    msg.CAN_Word2 = 0x0;
    msg.CAN_Word3 = 0x0;
    msg.CAN_Word4 = 0x0;


    if (Get_CFG_Sensor_MUX < 4)
    {
        if(GridPoint_MUX < 32) //32 = 0x20
        {
            msg.CAN_Word1 = (Get_CFG_Sensor_MUX & 0xF) | ((GridPoint_MUX & 0xFF) << 4);
      //      msg.CAN_Word2 = Sensors[Get_CFG_Sensor_MUX].x_Values[GridPoint_MUX];
            msg.CAN_Word3 = Sensors[Get_CFG_Sensor_MUX].y_Values[GridPoint_MUX];
            msg.CAN_Word4 = Sensors[Get_CFG_Sensor_MUX].y_Values[GridPoint_MUX] >> 16;
            GridPoint_MUX++;
        }
        else if(GridPoint_MUX == 32)
        {
            msg.CAN_Word1 = (Get_CFG_Sensor_MUX & 0xF) | 0x200 | ((Sensors[Get_CFG_Sensor_MUX].CON_mode & 0x3) << 12) | ((Sensors[Get_CFG_Sensor_MUX].connected & 0x1) << 15);
            msg.CAN_Word2 = Sensors[Get_CFG_Sensor_MUX].CON_mode;
            msg.CAN_Word3 = Sensors[Get_CFG_Sensor_MUX].offset;
            msg.CAN_Word4 = Sensors[Get_CFG_Sensor_MUX].faktor;

            Timer2_Stop();
            GridPoint_MUX = 0;
        }
    }

    else if(Get_CFG_Sensor_MUX == 4)
    {
        msg.CAN_Word1 = 0x204 | ((D_Sensor.CON_mode & 0x3) << 12);
        msg.CAN_Word2 = D_Sensor.NumbTeeth & 0x00FF;
        msg.CAN_Word3 = D_Sensor.radius;
        msg.CAN_Word4 = 0x0;

        Timer2_Stop();
    }
    else if (Get_CFG_Sensor_MUX == 5)
    {
        msg.CAN_Word1 = 0x205;
        msg.CAN_Word2 = (CSI_config_str >> 16) & 0xFF;
        msg.CAN_Word3 = CSI_config_str & 0xFFFF;
        msg.CAN_Word4 = (CSI_config_ID & 0x0F) | ((CSI_config_version << 8)&0xFF00);
        Timer2_Stop();
    }
    send_CAN(msg);
}

//void CSI_set_Config(struct_CAN_Message msg, uint16_t ADC_val, S08 device)

void set_CSI_Config_EEPROM(S08 Device, uint8_t val, uint16_t dest_address)
{
	if(dest_address != 0){
		dest_address = dest_address + 0x10; //add 0x10
	}

	S_Write_EEPROM(&Device, dest_address, val);
}

uint8_t get_CSI_Config_EEPROM(S08 Device, uint16_t dest_address)
{
	uint8_t val;
	if(dest_address != 0){
		dest_address = dest_address + 0x10; //add 0x10
	}

	S_Read_EEPROM(&Device, dest_address, val);

	return val;
}

/*
void CSI_set_Config(struct_CAN_Message msg, uint16_t ADC_val)
{
	//initialize_EEPROM(S08 * device, I2C_HandleTypeDef *i2cHandle1);
	//S_Write_EEPROM(S08 *device, uint32_t dest_address, uint8_t data);

    uint8_t sensor_MUX = (msg.CAN_Word1>>8) & 0xF;//Bit 8-11
    uint8 GridPoint_MUX = (msg.CAN_Word1 & 0xF000)>>8 | (msg.CAN_Word2&0xF); //Bit 12-19

        if(sensor_MUX < 4) //analog sensor
        {

//        	 CSI_config_ID = msg.CAN_Word4 & 0x000F;
//        	 CSI_config_str = msg.CAN_Word3 | ((uint32)(msg.CAN_Word2 & 0xFF00) << 8);
//        	 CSI_config_version = (msg.CAN_Word4 >> 8) & 0xFF;

        		//gives the LSB
        	uint8_t vr_LSB  = (uint8_t) (ADC_val & 0x00FF);
        	//gives the MSB
        	uint8_t vr_MSB = (uint8_t) ((ADC_val >> 8)& 0x00FF);

        	 msg.CAN_ID = 0x10;
        	 //test.CAN_Word1 = 0x10;
        	 msg.CAN_Word1 = vr_LSB;
        	 msg.CAN_Word2 = vr_MSB;


        	// S_Write_EEPROM(S08 *device, uint32_t dest_address, uint8_t data);
        	 //TxData[0] = 0xff;
        	 //TxData[1] = 0x01;
        	// send_CAN(test);



        	//Sensors[Sensor_MUX].x_Values[GridPoint_MUX] = msg.CAN_Word2>>4;
        //	Sensors[Sensor_MUX].y_Values[GridPoint_MUX] = msg.CAN_Word3 | (((sint32)msg.CAN_Word4)<<16);
        	 //return msg;
        	 send_CAN(msg);
        }


*******
    if(sensor_MUX < 4) //analog sensor
    {
    	Sensors[Sensor_MUX].x_Values[GridPoint_MUX] = msg.CAN_Word2>>4;
    	Sensors[Sensor_MUX].y_Values[GridPoint_MUX] = msg.CAN_Word3 | (((sint32)msg.CAN_Word4)<<16);
    }
******


    if(Sensor_MUX < 4) //analog sensor
    {
        if(GridPoint_MUX == 32) //Offset, Factor, CON-Mode
        {
            Sensors[Sensor_MUX].CON_mode = (msg.CAN_Word2>>4) & 0xF;
            Sensors[Sensor_MUX].offset = msg.CAN_Word3;
            Sensors[Sensor_MUX].faktor = msg.CAN_Word4;

            write_EEPROM(&Sensors[Sensor_MUX]);
        }
        else
        {
            //check that next x value is higher or equal to previous x value
            //if((msg.CAN_Word2>>4) < Sensors[Sensor_MUX].x_Values[GridPoint_MUX-1]) {
            //    Sensors[Sensor_MUX].x_Values[GridPoint_MUX] = Sensors[Sensor_MUX].x_Values[GridPoint_MUX-1];
            //} else {
            Sensors[Sensor_MUX].x_Values[GridPoint_MUX] = msg.CAN_Word2>>4;
            //}
            Sensors[Sensor_MUX].y_Values[GridPoint_MUX] = msg.CAN_Word3 | (((sint32)msg.CAN_Word4)<<16);
        }
    }
    else if (Sensor_MUX == 4) //digital Sensor
    {
        D_Sensor.CON_mode = (msg.CAN_Word2>>4) & 0xF;
        D_Sensor.NumbTeeth = msg.CAN_Word3 & 0xFF;
        D_Sensor.radius = msg.CAN_Word4;

        write_D_EEPROM(&D_Sensor);
        struct_CAN_Message echo;
            echo.CAN_ID = 0x03;
            echo.CAN_Word3 = Sensor_MUX;
            echo.CAN_Word4 = GridPoint_MUX;
            send_CAN(echo);

    }
    else if (Sensor_MUX == 5) //set CSI config and write eeprom
    {
        CSI_config_ID = msg.CAN_Word4 & 0x000F;
        CSI_config_str = msg.CAN_Word3 | ((uint32)(msg.CAN_Word2 & 0xFF00) << 8);
        CSI_config_version = (msg.CAN_Word4 >> 8) & 0xFF;
	  write_config_ID_EEPROM(&CSI_config_ID, &CSI_config_str, &CSI_config_version);

    }


}
*/
