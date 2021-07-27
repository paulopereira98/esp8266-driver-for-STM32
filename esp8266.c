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
 * @file esp8266.c
 * @brief esp8266 AT command driver for stm32 and FreeRTOS.
 */

#include "rtc.h"
#include "usart.h"
#include "string.h"
#include "stdlib.h"
#include "cmsis_os.h"
#include "esp8266.h"


static char wifi_RXbuffer[512]={0};
static volatile uint8_t wifi_RXbuffer_index=0;

static SemaphoreHandle_t xWifiOkSemaphore;


/**
  * @brief  init function for the wifi module
  * @retval None
  */
void esp8266_init(void)
{
	esp8266_disable();
	xWifiOkSemaphore = xSemaphoreCreateBinary();
}

/**
  * @brief  turn on the ESP8266
  * @retval None
  */
void esp8266_enable(void)         
{
	HAL_UART_Receive_IT(&ESP8266_UART_HANDLER, (uint8_t*)&wifi_RXbuffer[wifi_RXbuffer_index], 1);	
	__HAL_UART_FLUSH_DRREGISTER(&ESP8266_UART_HANDLER);
	HAL_UART_Receive_IT(&ESP8266_UART_HANDLER, (uint8_t*)&wifi_RXbuffer[wifi_RXbuffer_index], 1);	
	
	HAL_GPIO_WritePin(ESP8266_RST_GPIO_Port, ESP8266_RST_Pin, GPIO_PIN_SET);
	osDelay(5000); //give time to wake up
}

/**
  * @brief  turn off the ESP8266
  * @retval None
  */
void esp8266_disable(void)         
{
	HAL_GPIO_WritePin(ESP8266_RST_GPIO_Port, ESP8266_RST_Pin, GPIO_PIN_RESET);
}

/**
  * @brief  hardware reset the ESP8266
  * @retval None
  */
void esp8266_reset(void)         
{
	esp8266_disable();
	osDelay(200);
	esp8266_enable();
}

/**
  * @brief  send AT command to the ESP8266
  * @param	cmd command string
  * @retval None
  */
void esp8266_send_command(const char* cmd)         
{
		HAL_UART_Transmit(&ESP8266_UART_HANDLER, (uint8_t*)cmd, strlen(cmd), 100);
		HAL_UART_Transmit(&ESP8266_UART_HANDLER, (uint8_t*)"\r\n", 2, 100);
}

/**
  * @brief  send AT command to the ESP8266 and wait for "OK"
  * @param	cmd			command string
  * @param	timeout	timeout waiting for "OK"
  * @retval WIFI_SUCESS or WIFI_FAIL
  */
uint8_t esp8266_send_command_ack(const char* cmd, uint32_t timeout)         
{
	uint8_t cnt = 0;
	
	  while(cnt++ < WIFI_MAX_ERRORS)
	  {
		HAL_UART_Transmit(&ESP8266_UART_HANDLER, (uint8_t*)cmd, strlen(cmd), 100);
		xQueueReset(xWifiOkSemaphore);
		HAL_UART_Transmit(&ESP8266_UART_HANDLER, (uint8_t*)"\r\n", 2, 100);
		osDelay(200);//delay to prevent another command too fast
		
		if( xSemaphoreTake(xWifiOkSemaphore, pdMS_TO_TICKS(timeout)) == pdTRUE  )	//wait ok
			return WIFI_SUCESS;																											//ok found
	  }
	
	return WIFI_FAIL;
}

/**
  * @brief  start TCP connection to the remote server
  * @param ip IP or URL of the server
  * @param port port of the server
  * @retval WIFI_SUCESS or WIFI_FAIL
  */
uint8_t esp8266_start_tcp(char* ip, uint8_t port)         
{
	char buf[64];

	sprintf(buf, "AT+CIPSTART=\"TCP\",\"%s\",%d", ip, port);
	
	return esp8266_send_command_ack(buf,4000); 
}

/**
  * @brief  send TCP packet to the remote server
  * @param	packet TCP payload
  * @retval None
  */
void esp8266_send_tcp(char* packet)         
{
	char buf[20];
	
	sprintf(buf, "AT+CIPSEND=%d\r\n", strlen(packet););
	
	HAL_UART_Transmit(&ESP8266_UART_HANDLER, (uint8_t*)buf, strlen(buf), 500);
	osDelay(200);
	HAL_UART_Transmit(&ESP8266_UART_HANDLER, (uint8_t*)packet, length, 5000);
}

/**
  * @brief  close TCP connection
  * @retval None
  */
inline void esp8266_close_tcp(void)         
{
	esp8266_send_command("AT+CIPCLOSE"); 
}

/**
  * @brief	connect to a access point
  * @param	ssid ssid of the network to connect
  * @param	pass password of the network
  * @retval WIFI_SUCESS or WIFI_FAIL
  */
uint8_t esp8266_establish_connection(char* ssid, char* pass)
{
	char buf[64];

	sprintf(buf, "AT+CWJAP=\"%s\",\"%s\"", ssid, pass);

	return esp8266_send_command_ack(buf,20000); 
}

/**
  * @brief  POST HTTP packet 
  * @param	ip		IP or URL of the server
  * @param	port	Port of the server
  * @param	addr		Address to POST to
  * @param	payload	Data to POST
  * @retval WIFI_SUCESS or WIFI_FAIL
  */
uint8_t esp8266_HTTP_post(char* ip, uint8_t port, char* addr, char* payload)
{	
	char buf[20];
	if( esp8266_start_tcp(ip, port) == WIFI_FAIL)
		return WIFI_FAIL;
	
	//send tcp
	sprintf(buf, "AT+CIPSEND=%d", strlen(addr)+strlen(payload)+16 );
	HAL_UART_Transmit(&ESP8266_UART_HANDLER, (uint8_t*)buf, strlen(buf), 100);
	HAL_UART_Transmit(&ESP8266_UART_HANDLER, (uint8_t*)"POST ", 5, 100);
	HAL_UART_Transmit(&ESP8266_UART_HANDLER, (uint8_t*)addr, strlen(addr), 100);
	HAL_UART_Transmit(&ESP8266_UART_HANDLER, (uint8_t*)" HTTP/1.1\r\n", 11, 100);
	HAL_UART_Transmit(&ESP8266_UART_HANDLER, (uint8_t*)payload, strlen(payload), 500);
	HAL_UART_Transmit(&ESP8266_UART_HANDLER, (uint8_t*)"\r\n", 2, 100);
	
	osDelay(3000);
	esp8266_close_tcp();
	
	return WIFI_SUCESS;
}

/**
  * @brief  GET HTTP packet
  * @param  ip		IP or URL of the server
  * @param  port	port of the server
  * @param  addr	Address to POST to
  * @param  payload	Data to POST
  * @param  buffer	buffer to receive data
  * @param  size	Maximum amount of data to be received
  * @param  timeout 	Timeout duration
  * @retval WIFI_SUCESS or WIFI_FAIL
  */
uint8_t esp8266_HTTP_get(char* ip, uint8_t port, char* addr, char* payload, char* buffer, uint32_t size, uint32_t timeout)
{
	char buf[20];
	uint8_t cnt = 0;

	sprintf(buf, "AT+CIPSEND=%d", strlen(addr)+strlen(payload)+15 );

	if( esp8266_start_tcp(ip, port) == WIFI_FAIL )		 //start tcp connection
		return WIFI_FAIL;

	//send request
	HAL_UART_Transmit(&ESP8266_UART_HANDLER, (uint8_t*)buf, strlen(buf), 100);
	HAL_UART_Transmit(&ESP8266_UART_HANDLER, (uint8_t*)"GET ", 4, 100);
	HAL_UART_Transmit(&ESP8266_UART_HANDLER, (uint8_t*)addr, strlen(addr), 100);
	HAL_UART_Transmit(&ESP8266_UART_HANDLER, (uint8_t*)" HTTP/1.1\r\n", 11, 100);
	if(payload)
		HAL_UART_Transmit(&ESP8266_UART_HANDLER, (uint8_t*)payload, strlen(payload), 500);
	HAL_UART_Transmit(&ESP8266_UART_HANDLER, (uint8_t*)"\r\n", 2, 100);


	HAL_UART_AbortReceive_IT(&ESP8266_UART_HANDLER);	//stop wifi uart interrupts
	__HAL_UART_FLUSH_DRREGISTER(&ESP8266_UART_HANDLER);
	HAL_UART_Receive(&ESP8266_UART_HANDLER, (uint8_t*)buffer, size, timeout);  //receive data in poling mode
	
	HAL_UART_Receive_IT(&ESP8266_UART_HANDLER, (uint8_t*)&wifi_RXbuffer[wifi_RXbuffer_index], 1);//enable wifi uart interrupts again
	
	esp8266_close_tcp();
	
	return WIFI_SUCESS;
}

/**
  * @brief  wifi uart callback routine
  * @retval None
  */
void esp8266_CallBack(void) 
{
	static uint8_t aux;
	aux = wifi_RXbuffer[wifi_RXbuffer_index];
	
	if(wifi_RXbuffer[wifi_RXbuffer_index] == '\n')		//end of line
	{
		wifi_RXbuffer[wifi_RXbuffer_index+1] = '\0';
		wifi_RXbuffer_index = 0;
		
	}
	else if(wifi_RXbuffer[wifi_RXbuffer_index] == '\r');
	else																								//new char
	{
		wifi_RXbuffer_index++;
		wifi_RXbuffer_index &= ~(1<<8); //keep inside the limits
		
		if( wifi_RXbuffer_index == 2 )
			if( !strncmp(wifi_RXbuffer, "OK", 2) )			//signal 'OK'
				xSemaphoreGiveFromISR( xWifiOkSemaphore, NULL);
		
	}
	//set the interrups for UART Rx again
	HAL_UART_Receive_IT(&ESP8266_UART_HANDLER, (uint8_t*)&wifi_RXbuffer[wifi_RXbuffer_index], 1);
}
