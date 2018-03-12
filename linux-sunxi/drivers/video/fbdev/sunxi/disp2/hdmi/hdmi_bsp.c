#include "hdmi_bsp.h"
int bsp_hdmi_set_func(hdmi_bsp_func *func)
{
	return 0;
}

void bsp_hdmi_set_addr(uintptr_t base_addr)
{
	;
}

void bsp_hdmi_init(void)
{
	;
}
void bsp_hdmi_set_video_en(unsigned char enable)
{
	;
}

int bsp_hdmi_video(struct video_para *video)
{
	return 0;
}
int bsp_hdmi_audio(struct audio_para *audio)
{
	return 0;
}

int bsp_hdmi_ddc_read(char cmd, char pointer, char offset,
			int nbyte, char *pbuf)
{
	return 0;
}

unsigned int bsp_hdmi_get_hpd(void)
{
	return 0;
}
void bsp_hdmi_standby(void)
{
	;
}
void bsp_hdmi_hrst(void)
{
	;
}
void bsp_hdmi_hdl(void)
{
	;
}
/* @version: 0:A, 1:B, 2:C, 3:D */
void bsp_hdmi_set_version(unsigned int version)
{
	;
}
int bsp_hdmi_hdcp_err_check(void)
{
	return 0;
}
int bsp_hdmi_cec_get_simple_msg(unsigned char *msg)
{
	return 0;
}
int bsp_hdmi_cec_send(char *buf, unsigned char bytes)
{
	return 0;
}
void bsp_hdmi_cec_free_time_set(unsigned char value)
{
	;
}
int bsp_hdmi_cec_sta_check(void)
{
	return 0;
}
int bsp_hdmi_set_bias_source(unsigned int src)
{
	return 0;
}

#if defined(HDMI_ENABLE_DUMP_WRITE)
int bsp_hdmi_read(unsigned int addr, unsigned char *buf)
{
	return 0;
}
void bsp_hdmi_write(unsigned int addr, unsigned char data)
{
	;
}
#endif


