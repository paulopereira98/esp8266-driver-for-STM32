# esp8266-driver-for-STM32
esp8266 AT command driver for stm32 and FreeRTOS.

&nbsp;
&nbsp;
&nbsp;

### [Open Documenttion](https://paulopereira98.github.io/esp8266-driver-for-STM32/esp8266_8h.html)
&nbsp;
### How to use:

##### Add the esp8266_CallBack() function to the usart callback: usart.c
```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
  if (huart->Instance == ESP8266_UART_HANDLER.Instance)
  {
    esp8266_CallBack(); 
  }
}
```

##### Call the esp8266_init() function on startup: main.c
```c
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */
  /* MCU Configuration--------------------------------------------------------*/
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  /* USER CODE BEGIN Init */
  /* USER CODE END Init */
  /* Configure the system clock */
  SystemClock_Config();
  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */
  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_UART4_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  
  printf("esp8266 test\r\n");

  esp8266_init();
  
  /* USER CODE END 2 */
  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init(); 
  /* Start scheduler */
  osKernelStart();
  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
```


##### Add a task with your code.
##### You can use this exampe as reference:
```c
/*freertos.c*/
void vWifi_taskFunction(void const * argument)
{
  
  while(1)  //setup, only exit if concluded correctly
  {
    esp8266_reset();
    osDelay(5000);    //wait for the esp3266 to wake up
    
    if( esp8266_send_command_ack("AT",200) == WIFI_FAIL )           //Sends AT command with timeout (Command for Achknowledgement)
      continue;
    if( esp8266_send_command_ack("AT+CWMODE=1",200) == WIFI_FAIL )  //Sends AT command with timeout (For setting mode of Wifi)
      continue;
    if( esp8266_send_command_ack("AT+RST", 5000) == WIFI_FAIL )     //Sends AT command with timeout (For RESETTING WIFI)
      continue;
    break; //everythink is ok, exit loop
  }
  
  for(;;)  //infinite loop
  {  
    esp8266_enable();
    
    do{
      if( wifi_establish_connection("mynetwork", "mypassword") == WIFI_FAIL )  //connect to a access point
        break;

      /* Your periodic code here - begin */

      esp8266_post("192.168.1.10", 22, "192.168.1.10/test/", "this is a POST request");


      /* Your periodic code here - end */
    }while(0);
    
    esp8266_disable();
    osDelay(1000*60*1);    //sleep 1 minutes to save energy

  }
  
}
```
