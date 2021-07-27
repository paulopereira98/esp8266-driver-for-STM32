/*
	esp8266 AT driver for stm32 and FreeRTOS
	Copyright (C) 2019  Paulo Pereira	(EEIC-UMinho)

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * @file esp8266.h
 * @brief esp8266 AT command driver for stm32 and FreeRTOS.
 */

#ifndef _ESP8266_H_
#define _ESP8266_H_

#include "stdint.h"
#include "gpio.h"


//Replace the reset pin with the one you are using
#define ESP8266_RST_Pin 		GPIO_PIN_3
#define ESP8266_RST_GPIO_Port	GPIOD

//Replace the uart handler with the one you are using
#define ESP8266_UART_HANDLER  	huart4

//number of attempts before back off
#define WIFI_MAX_ERRORS		5 


enum wifi_return 
{
	WIFI_FAIL=0, 
	WIFI_SUCESS
};


void esp8266_init(void);
void esp8266_enable(void);
void esp8266_disable(void);
void esp8266_reset(void);
void esp8266_send_command(const char* cmd);
uint8_t esp8266_send_command_ack(const char* cmd, uint32_t timeout);
uint8_t esp8266_start_tcp(char* ip, uint8_t port);
void esp8266_send_tcp(char* packet);
void esp8266_close_tcp(void);
uint8_t esp8266_establish_connection(char* ssid, char* pass);
uint8_t esp8266_HTTP_post(char* ip, uint8_t port, char* addr, char* payload);
uint8_t esp8266_HTTP_get(char* ip, uint8_t port, char* addr, char* payload, char* buffer, uint32_t size, uint32_t timeout);
extern void esp8266_CallBack(void);


#endif /*_ESP8266_H_*/
