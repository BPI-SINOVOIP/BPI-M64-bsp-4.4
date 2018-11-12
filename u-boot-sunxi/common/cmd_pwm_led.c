#include <common.h>
#include <command.h>
#include <sys_config.h>
#include <sys_config_old.h>
#include <sunxi_board.h>
#include <pwm.h>
#include <pwm_led.h>
#include <fdt_support.h>

#define PWM_LED_PERIOD (100000)
#define PWM_LED_GREEN (0)
#define PWM_LED_BLUE (1)
#define MAX_VALUE (255)

static __u32 gpio_led_hd;

int load_led_values_from_dtb(int* led_r_p, int* led_g_p, int* led_b_p)
{
	int node, ret;
	char main_name[20], sub_name[25];

	sprintf(main_name, "/pwm_leds");
	node = fdt_path_offset(working_fdt, main_name);
	if (node < 0) {
		printf ("error:fdt %s err returned %s\n", main_name, fdt_strerror(node));
		return 1;
	}

	sprintf(sub_name, "led_r");
	ret = fdt_getprop_u32(working_fdt, node, sub_name, (uint32_t*)led_r_p);
	if (ret < 0) {
		printf("fdt_getprop_u32 %s.%s fail\n", main_name, sub_name);
		return 1;
	}

	sprintf(sub_name, "led_g");
	ret = fdt_getprop_u32(working_fdt, node, sub_name, (uint32_t*)led_g_p);
	if (ret < 0) {
		printf("fdt_getprop_u32 %s.%s fail\n", main_name, sub_name);
		return 1;
	}

	sprintf(sub_name, "led_b");
	ret = fdt_getprop_u32(working_fdt, node, sub_name, (uint32_t*)led_b_p);
	if (ret < 0) {
		printf("fdt_getprop_u32 %s.%s fail\n", main_name, sub_name);
		return 1;
	}
	return 0;
}

int load_led_values_from_env(int* led_r_p, int* led_g_p, int* led_b_p)
{
	*led_r_p = getenv_ulong("led_r", 10, 0);
	*led_g_p = getenv_ulong("led_g", 10, 0);
	*led_b_p = getenv_ulong("led_b", 10, 0);
	return 0;
}

int pwm_led_load(int* led_r_p, int* led_g_p, int* led_b_p)
{
	int ret;
	ret = load_led_values_from_dtb(led_r_p, led_g_p, led_b_p);
	return ret;
}

int store_led_values_to_dtb(int led_r, int led_g, int led_b)
{
	do_fixup_by_path_u32(working_fdt, "/pwm_leds", "led_r", led_r, 1);
	do_fixup_by_path_u32(working_fdt, "/pwm_leds", "led_g", led_g, 1);
	do_fixup_by_path_u32(working_fdt, "/pwm_leds", "led_b", led_b, 1);
	return 0;
}

int store_led_values_to_env(int led_r, int led_g, int led_b)
{
	setenv_ulong("led_r", led_r);
	setenv_ulong("led_g", led_g);
	setenv_ulong("led_b", led_b);
	return 0;
}

int pwm_led_store(int led_r, int led_g, int led_b)
{
	int ret;
	ret = store_led_values_to_dtb(led_r, led_g, led_b);
	return ret;
}

int pwm_led_init(void)
{
	int ret, workmode;
	int led_r, led_g, led_b;

	workmode = get_boot_work_mode();
	if (workmode != WORK_MODE_BOOT)
		return 0;

	ret = pwm_led_load(&led_r, &led_g, &led_b);
	if(ret)
		return 0;

	user_gpio_set_t	gpio_init;
	memset(&gpio_init, 0, sizeof(user_gpio_set_t));
	ret = script_parser_fetch("led_para", "led1", (void *)&gpio_init, sizeof(user_gpio_set_t)>>2);
	if(!ret)
	{
		if(gpio_init.port)
		{
			gpio_led_hd = gpio_request(&gpio_init, 1);
			if(!gpio_led_hd)
			{
				printf("reuqest gpio for led failed\n");
				return 1;
			}
		}
	}

	pwm_init();
	pwm_request(PWM_LED_GREEN, "led_green");
	/* pwm_config(PWM_LED_GREEN, PWM_LED_PERIOD, PWM_LED_PERIOD); */
	/* pwm_enable(PWM_LED_GREEN); */

	pwm_request(PWM_LED_BLUE, "led_blue");
	/* pwm_config(PWM_LED_BLUE, PWM_LED_PERIOD, PWM_LED_PERIOD); */
	/* pwm_enable(PWM_LED_BLUE); */

	printf("init led: r:%d, g:%d b:%d\n", led_r, led_g, led_b);
	pwm_led_set(led_r, led_g, led_b);

	printf("pwm_led_init end\n");
	return 0;
}


int pwm_led_set(int red, int green, int blue)
{
	unsigned int duty_ns, period_ns, level;

	if (green != 0)
	{
		period_ns = PWM_LED_PERIOD;
		level = MAX_VALUE - green;
		if(level == 0)  /* for pwm, duty_ns shoule not be zero */
			level = 1;
		duty_ns = (period_ns * level) / MAX_VALUE;
		pwm_config(PWM_LED_GREEN, duty_ns, period_ns);
		pwm_enable(PWM_LED_GREEN);
	}
	else
	{
		pwm_disable(PWM_LED_GREEN);
	}

	if (blue != 0)
	{
		period_ns = PWM_LED_PERIOD;
		level = MAX_VALUE - blue;
		if(level == 0)  /* for pwm, duty_ns shoule not be zero */
			level = 1;
		duty_ns = (period_ns * level) / MAX_VALUE;
		pwm_config(PWM_LED_BLUE, duty_ns, period_ns);
		pwm_enable(PWM_LED_BLUE);
	}
	else
	{
		pwm_disable(PWM_LED_BLUE);
	}

	if(red > 0)
		gpio_write_one_pin_value(gpio_led_hd, 1, "led1");
	else
		gpio_write_one_pin_value(gpio_led_hd, 0, "led1");

	pwm_led_store(red, green, blue);

	return 0;
}

int pwm_led_test(void)
{
	int i, workmode;

	workmode = get_boot_work_mode();
	if (workmode != WORK_MODE_BOOT)
		return 0;

	printf("test red\n");
	for (i = 0; i <= MAX_VALUE; i++)
	{
		pwm_led_set(i, 0, 0);
		__msdelay(10);
	}
	for (i = 0; i <= MAX_VALUE; i++)
	{
		pwm_led_set(MAX_VALUE-i, 0, 0);
		__msdelay(10);
	}

	printf("test green\n");
	for (i = 0; i <= MAX_VALUE; i++)
	{
		pwm_led_set(0, i, 0);
		__msdelay(10);
	}
	for (i = 0; i <= MAX_VALUE; i++)
	{
		pwm_led_set(0, MAX_VALUE-i, 0);
		__msdelay(10);
	}

	printf("test blue\n");
	for (i = 0; i <= MAX_VALUE; i++)
	{
		pwm_led_set(0, 0, i);
		__msdelay(10);
	}
	for (i = 0; i <= MAX_VALUE; i++)
	{
		pwm_led_set(0, 0, MAX_VALUE-i);
		__msdelay(10);
	}
	return 0;
}

int do_pwm_led(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{

	ulong red_value, blue_value, green_value;

	if (argc < 3)
		return cmd_usage(cmdtp);

	red_value = simple_strtoul(argv[1], NULL, 10);
	blue_value = simple_strtoul(argv[2], NULL, 10);
	green_value = simple_strtoul(argv[3], NULL, 10);

	if (red_value > MAX_VALUE)
		red_value = MAX_VALUE;
	else if (red_value < 0)
		red_value = 0;

	if (blue_value > MAX_VALUE)
		blue_value = MAX_VALUE;
	else if (blue_value < 0)
		blue_value = 0;

	if (green_value > MAX_VALUE)
		green_value = MAX_VALUE;
	else if (green_value < 0)
		green_value = 0;

	pwm_led_set(red_value, green_value, blue_value);
	return 0;
}

U_BOOT_CMD(
	pwm_led,	5,	1,	do_pwm_led,
	"pwm_led  - set pwm led\n",
	"pwm_led red<0-255> blue<0-255> green<0-255>\n"
);
