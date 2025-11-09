#include "tm7711.h"
#include <wiringPi.h>
#include "ui/src/ui.h"

typedef enum __TM7711_CH{
	TM7711_CH1_10HZ = 0 ,
	TM7711_TEMP ,
	TM7711_CH1_40HZ,
}_TM7711_CHANNEL ;

#define	TM7711_CLK_PIN	27 // GPIO.2
#define	TM7711_CLK_OUT	pinMode(TM7711_CLK_PIN	, OUTPUT )
#define	TM7711_CLK_H	digitalWrite(TM7711_CLK_PIN , HIGH )
#define	TM7711_CLK_L	digitalWrite(TM7711_CLK_PIN , LOW )

#define	TM7711_SDA_PIN	17 // GPIO.0
#define	TM7711_SDA_IN	pinMode(TM7711_SDA_PIN	, INPUT )
#define	TM7711_SDA_T	digitalRead(TM7711_SDA_PIN )

#define	REF	2489
#define	R1	30
#define	R2	20000

#define	ADC_BUFF_MAX		20 
uint32_t adc_buff[ADC_BUFF_MAX+1]={0,} ;
uint32_t adc_da ;
uint32_t t1 ;

uint32_t tm7711_read(_TM7711_CHANNEL next_select);
void Maopao(uint32_t * buff ,  uint16_t size );

void tm7711_init(void)
{
	TM7711_CLK_OUT ;
	TM7711_SDA_IN ;
	TM7711_CLK_L ;
	delayMicroseconds(100 ) ; //100us

	// printf("TM7711 Init\r\n");

    adc_buff[0] = 1 ;
}

void tm7711_loop(void)
{
    if(TM7711_SDA_T == 0)
    {
        adc_buff[adc_buff[0]] = tm7711_read(TM7711_CH1_10HZ) ;
        adc_buff[0] += 1 ;
        if(adc_buff[0] >= ADC_BUFF_MAX)
        {
            Maopao(adc_buff+1 , adc_buff[0] -1 );
            adc_buff[0] = 1 ;
            adc_da = adc_buff[ADC_BUFF_MAX/2] ;
            adc_da >>= 8 ;
    
            t1=adc_da ;
            t1= (t1*REF) / 65535 ;
            t1= (t1*(R1+R2))/R1 ;
            t1 = t1 /128 ;
			t1 = t1 + 100;

			lv_label_set_text_fmt(ui_BatteryLabel, "Battery: %ld.%ld", t1/1000, t1%1000);
            // printf("battery is : 0X%x , %d, %d mV \r\n",adc_da,adc_da ,t1);
        }
    }
}

uint32_t tm7711_read(_TM7711_CHANNEL next_select)
{
	uint8_t f1 ;
	uint32_t re_da ;
	re_da = 0 ;
	for(f1=0 ; f1<24 ; f1++)
	{
		TM7711_CLK_H ;
		re_da <<= 1 ;
		delayMicroseconds(5) ; //5us
		TM7711_CLK_L ;
		if(TM7711_SDA_T) re_da |= 1 ;
		delayMicroseconds(5) ; //5us
	}
	switch(next_select)
	{
		case TM7711_CH1_40HZ :
			TM7711_CLK_H ;
			delayMicroseconds(5) ; //5us
			TM7711_CLK_L ;
			delayMicroseconds(5) ; //5us

		case TM7711_TEMP :
			TM7711_CLK_H ;
			delayMicroseconds(5) ; //5us
			TM7711_CLK_L ;
			delayMicroseconds(5) ; //5us
			
		case TM7711_CH1_10HZ :
			TM7711_CLK_H ;
			delayMicroseconds(5) ; //5us
			TM7711_CLK_L ;
			delayMicroseconds(5) ; //5us
			break ;
	}

	return re_da ;
}

void Maopao(uint32_t * buff ,  uint16_t size )
{
	uint16_t f1 , f2 ;
	uint32_t da_t;
	for(f1=0 ; f1<size ; f1++)
	{
		for(f2=f1 ; f2<size ; f2++)
		{
			if(buff[f1] > buff[f2])
			{
				da_t = buff[f1] ;
				buff[f1] = buff[f2] ;
				buff[f2] = da_t ;
			}
		}
	}
}




