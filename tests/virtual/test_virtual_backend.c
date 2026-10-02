#include "virtual_backend.h"
#include "hal_flash.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int hits;
static void gpio_cb(hal_gpio_pin_t*p,void*c){(void)p;(*(int*)c)++;}
static void timer_cb(hal_timer_t*t,void*c){(void)t;(*(int*)c)++;}
int main(void){
 avhp_virtual_gpio_t g;assert(hal_gpio_init((hal_gpio_pin_t*)&g,HAL_GPIO_MODE_OUTPUT)==HAL_OK);assert(hal_gpio_set_interrupt((hal_gpio_pin_t*)&g,gpio_cb,&hits)==HAL_OK);avhp_virtual_gpio_trigger(&g,true);assert(hits==1);
 avhp_virtual_uart_t u;hal_uart_config_t uc={115200,8,1,0};assert(hal_uart_init((hal_uart_t*)&u,&uc)==HAL_OK);uint8_t in[]={1,2,3},out[3];size_t n=0;avhp_virtual_uart_feed(&u,in,3);assert(hal_uart_read((hal_uart_t*)&u,out,3,&n)==HAL_OK&&n==3&&memcmp(in,out,3)==0);
 avhp_virtual_spi_t s;hal_spi_config_t sc={1000000,0,true};assert(hal_spi_init((hal_spi_t*)&s,&sc)==HAL_OK);assert(hal_spi_set_cs((hal_spi_t*)&s,true)==HAL_OK);uint8_t tx[]={0xAA,0x55},rx[2];assert(hal_spi_transfer((hal_spi_t*)&s,tx,rx,2)==HAL_OK&&memcmp(tx,rx,2)==0);
 avhp_virtual_i2c_t i;hal_i2c_config_t ic={400000};assert(hal_i2c_init((hal_i2c_t*)&i,&ic)==HAL_OK);assert(hal_i2c_mem_write((hal_i2c_t*)&i,0x1e,3,tx,2)==HAL_OK);assert(hal_i2c_mem_read((hal_i2c_t*)&i,0x1e,3,rx,2)==HAL_OK&&memcmp(tx,rx,2)==0);
 avhp_virtual_timer_t t;assert(hal_timer_init((hal_timer_t*)&t,1000)==HAL_OK);assert(hal_timer_register_callback((hal_timer_t*)&t,timer_cb,&hits)==HAL_OK);assert(hal_timer_start((hal_timer_t*)&t)==HAL_OK);avhp_virtual_timer_fire(&t);assert(hits==2);
 avhp_virtual_can_t can;hal_can_frame_t f={.id=7,.dlc=2,.data={9,8}};assert(hal_can_init((hal_can_t*)&can,500000)==HAL_OK);assert(hal_can_send((hal_can_t*)&can,&f)==HAL_OK);hal_can_frame_t got;assert(hal_can_receive((hal_can_t*)&can,&got)==HAL_OK&&got.id==7&&got.data[1]==8);
 uint8_t fd=0x5A,fr=0;assert(hal_flash_erase_sector(0)==HAL_OK);assert(hal_flash_write(0,&fd,1)==HAL_OK);assert(hal_flash_read(0,&fr,1)==HAL_OK&&fr==fd);
 puts("PASS: virtual peripheral HAL compliance smoke tests");return 0;}
