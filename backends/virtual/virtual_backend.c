#include "virtual_backend.h"
#include "hal_flash.h"
#include "hal_clock.h"
#include "hal_power.h"
#include <string.h>
#include <math.h>

hal_status_t hal_gpio_init(hal_gpio_pin_t *p,hal_gpio_mode_t m){if(!p)return HAL_INVALID_PARAM;avhp_virtual_gpio_t *g=(avhp_virtual_gpio_t*)p;memset(g,0,sizeof(*g));g->mode=m;return HAL_OK;}
hal_status_t hal_gpio_write(hal_gpio_pin_t *p,bool l){if(!p)return HAL_INVALID_PARAM;avhp_virtual_gpio_t*g=(void*)p;if(g->mode!=HAL_GPIO_MODE_OUTPUT)return HAL_ERROR;g->level=l;return HAL_OK;}
hal_status_t hal_gpio_read(hal_gpio_pin_t *p,bool*out){if(!p||!out)return HAL_INVALID_PARAM;*out=((avhp_virtual_gpio_t*)p)->level;return HAL_OK;}
hal_status_t hal_gpio_toggle(hal_gpio_pin_t*p){if(!p)return HAL_INVALID_PARAM;avhp_virtual_gpio_t*g=(void*)p;return hal_gpio_write(p,!g->level);}
hal_status_t hal_gpio_set_mode(hal_gpio_pin_t*p,hal_gpio_mode_t m){if(!p)return HAL_INVALID_PARAM;((avhp_virtual_gpio_t*)p)->mode=m;return HAL_OK;}
hal_status_t hal_gpio_set_interrupt(hal_gpio_pin_t*p,hal_gpio_irq_cb_t cb,void*ctx){if(!p)return HAL_INVALID_PARAM;((avhp_virtual_gpio_t*)p)->cb=cb;((avhp_virtual_gpio_t*)p)->ctx=ctx;return HAL_OK;}
void avhp_virtual_gpio_trigger(avhp_virtual_gpio_t*g,bool l){if(!g)return;bool changed=g->level!=l;g->level=l;if(changed&&g->cb)g->cb((hal_gpio_pin_t*)g,g->ctx);}

hal_status_t hal_uart_init(hal_uart_t*p,const hal_uart_config_t*c){if(!p||!c||!c->baud_rate||c->data_bits<5||c->data_bits>9)return HAL_INVALID_PARAM;avhp_virtual_uart_t*u=(void*)p;memset(u,0,sizeof(*u));u->cfg=*c;return HAL_OK;}
static size_t uart_count(const avhp_virtual_uart_t*u){return (u->head-u->tail)%sizeof(u->rx);}
hal_status_t hal_uart_write(hal_uart_t*p,const uint8_t*d,size_t n){if(!p||(!d&&n))return HAL_INVALID_PARAM;return HAL_OK;}
hal_status_t hal_uart_read(hal_uart_t*p,uint8_t*d,size_t n,size_t*out){if(!p||(!d&&n)||!out)return HAL_INVALID_PARAM;avhp_virtual_uart_t*u=(void*)p;size_t k=0;while(k<n&&uart_count(u)){d[k++]=u->rx[u->tail%sizeof(u->rx)];u->tail++;}*out=k;return HAL_OK;}
hal_status_t hal_uart_write_async(hal_uart_t*p,const uint8_t*d,size_t n,hal_uart_tx_done_cb_t cb,void*ctx){hal_status_t s=hal_uart_write(p,d,n);if(cb)cb(p,s,ctx);return s;}
hal_status_t hal_uart_register_rx_callback(hal_uart_t*p,hal_uart_rx_cb_t cb,void*ctx){if(!p)return HAL_INVALID_PARAM;avhp_virtual_uart_t*u=(void*)p;u->rx_cb=cb;u->rx_ctx=ctx;return HAL_OK;}
void avhp_virtual_uart_feed(avhp_virtual_uart_t*u,const uint8_t*d,size_t n){if(!u||(!d&&n))return;for(size_t i=0;i<n;i++){if(uart_count(u)<sizeof(u->rx)-1U){u->rx[u->head%sizeof(u->rx)]=d[i];u->head++;}if(u->rx_cb)u->rx_cb((hal_uart_t*)u,d[i],u->rx_ctx);}}

hal_status_t hal_spi_init(hal_spi_t*p,const hal_spi_config_t*c){if(!p||!c||c->mode>3||!c->clock_hz)return HAL_INVALID_PARAM;avhp_virtual_spi_t*s=(void*)p;memset(s,0,sizeof(*s));s->cfg=*c;return HAL_OK;}
hal_status_t hal_spi_transfer(hal_spi_t*p,const uint8_t*tx,uint8_t*rx,size_t n){if(!p||(!tx&&n)||(!rx&&n))return HAL_INVALID_PARAM;avhp_virtual_spi_t*s=(void*)p;if(!s->cs)return HAL_BUSY;for(size_t i=0;i<n;i++)rx[i]=tx[i]^s->loopback_xor;return HAL_OK;}
hal_status_t hal_spi_transfer_async(hal_spi_t*p,const uint8_t*tx,uint8_t*rx,size_t n,hal_spi_transfer_done_cb_t cb,void*ctx){hal_status_t r=hal_spi_transfer(p,tx,rx,n);if(cb)cb(p,r,ctx);return r;}
hal_status_t hal_spi_set_cs(hal_spi_t*p,bool a){if(!p)return HAL_INVALID_PARAM;((avhp_virtual_spi_t*)p)->cs=a;return HAL_OK;}

hal_status_t hal_i2c_init(hal_i2c_t*p,const hal_i2c_config_t*c){if(!p||!c||!c->clock_hz)return HAL_INVALID_PARAM;avhp_virtual_i2c_t*i=(void*)p;memset(i,0,sizeof(*i));i->cfg=*c;return HAL_OK;}
hal_status_t hal_i2c_write(hal_i2c_t*p,uint8_t a,const uint8_t*d,size_t n){if(!p||(!d&&n)||a>=128)return HAL_INVALID_PARAM;if(n>256)return HAL_ERROR;memcpy(((avhp_virtual_i2c_t*)p)->memory[a],d,n);return HAL_OK;}
hal_status_t hal_i2c_read(hal_i2c_t*p,uint8_t a,uint8_t*d,size_t n){if(!p||(!d&&n)||a>=128||n>256)return HAL_INVALID_PARAM;memcpy(d,((avhp_virtual_i2c_t*)p)->memory[a],n);return HAL_OK;}
hal_status_t hal_i2c_mem_write(hal_i2c_t*p,uint8_t a,uint16_t m,const uint8_t*d,size_t n){if(!p||a>=128||m+n>256||(!d&&n))return HAL_INVALID_PARAM;memcpy(&((avhp_virtual_i2c_t*)p)->memory[a][m],d,n);return HAL_OK;}
hal_status_t hal_i2c_mem_read(hal_i2c_t*p,uint8_t a,uint16_t m,uint8_t*d,size_t n){if(!p||a>=128||m+n>256||(!d&&n))return HAL_INVALID_PARAM;memcpy(d,&((avhp_virtual_i2c_t*)p)->memory[a][m],n);return HAL_OK;}

hal_status_t hal_timer_init(hal_timer_t*p,uint32_t us){if(!p||!us)return HAL_INVALID_PARAM;avhp_virtual_timer_t*t=(void*)p;memset(t,0,sizeof(*t));t->period_us=us;return HAL_OK;}
hal_status_t hal_timer_start(hal_timer_t*p){if(!p)return HAL_INVALID_PARAM;((avhp_virtual_timer_t*)p)->running=true;return HAL_OK;}
hal_status_t hal_timer_stop(hal_timer_t*p){if(!p)return HAL_INVALID_PARAM;((avhp_virtual_timer_t*)p)->running=false;return HAL_OK;}
hal_status_t hal_timer_set_period(hal_timer_t*p,uint32_t us){if(!p||!us)return HAL_INVALID_PARAM;((avhp_virtual_timer_t*)p)->period_us=us;return HAL_OK;}
hal_status_t hal_timer_register_callback(hal_timer_t*p,hal_timer_cb_t cb,void*ctx){if(!p)return HAL_INVALID_PARAM;((avhp_virtual_timer_t*)p)->cb=cb;((avhp_virtual_timer_t*)p)->ctx=ctx;return HAL_OK;}
void avhp_virtual_timer_fire(avhp_virtual_timer_t*t){if(t&&t->running&&t->cb)t->cb((hal_timer_t*)t,t->ctx);}

hal_status_t hal_pwm_init(hal_pwm_t*p,uint32_t f){if(!p||!f)return HAL_INVALID_PARAM;avhp_virtual_pwm_t*x=(void*)p;memset(x,0,sizeof(*x));x->frequency_hz=f;return HAL_OK;}
hal_status_t hal_pwm_set_duty(hal_pwm_t*p,float d){if(!p||!isfinite(d)||d<0.0f||d>100.0f)return HAL_INVALID_PARAM;((avhp_virtual_pwm_t*)p)->duty_percent=d;return HAL_OK;}
hal_status_t hal_pwm_start(hal_pwm_t*p){if(!p)return HAL_INVALID_PARAM;((avhp_virtual_pwm_t*)p)->running=true;return HAL_OK;}
hal_status_t hal_pwm_stop(hal_pwm_t*p){if(!p)return HAL_INVALID_PARAM;((avhp_virtual_pwm_t*)p)->running=false;return HAL_OK;}

hal_status_t hal_can_init(hal_can_t*p,uint32_t b){if(!p||!b)return HAL_INVALID_PARAM;avhp_virtual_can_t*c=(void*)p;memset(c,0,sizeof(*c));c->bitrate=b;return HAL_OK;}
static bool can_match(const avhp_virtual_can_t*c,const hal_can_frame_t*f){return !c->filter_set||((f->id&c->filter.id_mask)==(c->filter.id_filter&c->filter.id_mask));}
hal_status_t hal_can_send(hal_can_t*p,const hal_can_frame_t*f){if(!p||!f||f->dlc>8)return HAL_INVALID_PARAM;avhp_virtual_can_t*c=(void*)p;if(!can_match(c,f))return HAL_OK;size_t next=(c->head+1U)%16U;if(next==c->tail)return HAL_BUSY;c->queue[c->head]=*f;c->head=next;return HAL_OK;}
hal_status_t hal_can_receive(hal_can_t*p,hal_can_frame_t*f){if(!p||!f)return HAL_INVALID_PARAM;avhp_virtual_can_t*c=(void*)p;if(c->tail==c->head)return HAL_BUSY;*f=c->queue[c->tail];c->tail=(c->tail+1U)%16U;return HAL_OK;}
hal_status_t hal_can_set_filter(hal_can_t*p,const hal_can_filter_t*f){if(!p||!f)return HAL_INVALID_PARAM;avhp_virtual_can_t*c=(void*)p;c->filter=*f;c->filter_set=true;return HAL_OK;}

hal_status_t hal_dma_configure(hal_dma_channel_t*p,const hal_dma_config_t*c){if(!p||!c||(!c->src&&c->length)||(!c->dst&&c->length))return HAL_INVALID_PARAM;avhp_virtual_dma_t*d=(void*)p;d->cfg=*c;d->configured=true;d->running=false;d->status=HAL_OK;return HAL_OK;}
hal_status_t hal_dma_start(hal_dma_channel_t*p){if(!p)return HAL_INVALID_PARAM;avhp_virtual_dma_t*d=(void*)p;if(!d->configured)return HAL_NOT_INITIALIZED;memcpy(d->cfg.dst,d->cfg.src,d->cfg.length);d->running=false;d->status=HAL_OK;return HAL_OK;}
hal_status_t hal_dma_stop(hal_dma_channel_t*p){if(!p)return HAL_INVALID_PARAM;((avhp_virtual_dma_t*)p)->running=false;return HAL_OK;}
hal_status_t hal_dma_get_status(hal_dma_channel_t*p,hal_status_t*out){if(!p||!out)return HAL_INVALID_PARAM;*out=((avhp_virtual_dma_t*)p)->status;return HAL_OK;}

hal_status_t hal_adc_init(hal_adc_channel_t*p,uint8_t r){if(!p||r<8||r>16)return HAL_INVALID_PARAM;avhp_virtual_adc_t*a=(void*)p;memset(a,0,sizeof(*a));a->resolution_bits=r;return HAL_OK;}
hal_status_t hal_adc_read(hal_adc_channel_t*p,uint16_t*out){if(!p||!out)return HAL_INVALID_PARAM;*out=((avhp_virtual_adc_t*)p)->value;return HAL_OK;}
hal_status_t hal_adc_start_continuous(hal_adc_channel_t*p,uint32_t hz){if(!p||!hz)return HAL_INVALID_PARAM;((avhp_virtual_adc_t*)p)->sample_rate_hz=hz;return HAL_OK;}
hal_status_t hal_adc_register_callback(hal_adc_channel_t*p,hal_adc_cb_t cb,void*ctx){if(!p)return HAL_INVALID_PARAM;((avhp_virtual_adc_t*)p)->cb=cb;((avhp_virtual_adc_t*)p)->ctx=ctx;return HAL_OK;}
void avhp_virtual_adc_set(avhp_virtual_adc_t*a,uint16_t v){if(!a)return;a->value=v;if(a->cb)a->cb((hal_adc_channel_t*)a,v,a->ctx);}

static uint8_t flash_mem[128U*1024U]; static bool flash_ready;
hal_status_t hal_flash_read(uint32_t a,uint8_t*d,size_t n){if(!d&&n)return HAL_INVALID_PARAM;if((uint64_t)a+n>sizeof(flash_mem))return HAL_ERROR;if(!flash_ready)memset(flash_mem,0xFF,sizeof(flash_mem)),flash_ready=true;memcpy(d,&flash_mem[a],n);return HAL_OK;}
hal_status_t hal_flash_write(uint32_t a,const uint8_t*d,size_t n){if(!d&&n)return HAL_INVALID_PARAM;if((uint64_t)a+n>sizeof(flash_mem))return HAL_ERROR;if(!flash_ready)memset(flash_mem,0xFF,sizeof(flash_mem)),flash_ready=true;for(size_t i=0;i<n;i++)flash_mem[a+i]&=d[i];return HAL_OK;}
hal_status_t hal_flash_erase_sector(uint32_t s){size_t off=(size_t)s*1024U;if(off+1024U>sizeof(flash_mem))return HAL_ERROR;if(!flash_ready)memset(flash_mem,0xFF,sizeof(flash_mem)),flash_ready=true;memset(&flash_mem[off],0xFF,1024U);return HAL_OK;}
hal_status_t hal_flash_get_status(hal_status_t*out){if(!out)return HAL_INVALID_PARAM;*out=HAL_OK;return HAL_OK;}

static uint64_t clock_ms,clock_us;hal_status_t hal_clock_init(void){clock_ms=clock_us=0;return HAL_OK;}uint64_t hal_clock_get_tick_ms(void){return clock_ms;}uint64_t hal_clock_get_tick_us(void){return clock_us;}void hal_clock_delay_ms(uint32_t ms){clock_ms+=ms;clock_us+=(uint64_t)ms*1000ULL;}
static float battery_v=12.0f,battery_i=0.5f;static hal_power_mode_t power_mode;
hal_status_t hal_power_get_battery_voltage(float*out){if(!out)return HAL_INVALID_PARAM;*out=battery_v;return HAL_OK;}hal_status_t hal_power_get_current(float*out){if(!out)return HAL_INVALID_PARAM;*out=battery_i;return HAL_OK;}hal_status_t hal_power_set_low_power_mode(hal_power_mode_t m){if(m>HAL_POWER_MODE_STANDBY)return HAL_INVALID_PARAM;power_mode=m;return HAL_OK;}

hal_status_t hal_sensor_init(hal_sensor_t*p){if(!p)return HAL_INVALID_PARAM;avhp_virtual_sensor_t*s=(void*)p;memset(s,0,sizeof(*s));s->health=HAL_SENSOR_HEALTH_OK;return HAL_OK;}
hal_status_t hal_sensor_read(hal_sensor_t*p,void*out,size_t n){if(!p||(!out&&n))return HAL_INVALID_PARAM;avhp_virtual_sensor_t*s=(void*)p;if(n>s->len)return HAL_ERROR;memcpy(out,s->data,n);return HAL_OK;}
hal_status_t hal_sensor_subscribe(hal_sensor_t*p,hal_sensor_cb_t cb,void*ctx){if(!p)return HAL_INVALID_PARAM;avhp_virtual_sensor_t*s=(void*)p;s->cb=cb;s->ctx=ctx;return HAL_OK;}
hal_status_t hal_sensor_get_health_status(hal_sensor_t*p,hal_sensor_health_t*out){if(!p||!out)return HAL_INVALID_PARAM;*out=((avhp_virtual_sensor_t*)p)->health;return HAL_OK;}
