#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/input.h>

#define SCREEN_MAX_X     1080
#define SCREEN_MAX_Y     1920
#define PRESS_MAX        0xFF

struct input_dev *ts_input_dev;

int __init zebra_ts_init(void)
{
	int ret;

	ts_input_dev = input_allocate_device();
	if (ts_input_dev == NULL){
		printk("input_allocate_device fail\n");
		return -ENOMEM;
	}

	ts_input_dev->name = "zebra_ts";
	set_bit(ABS_MT_TOUCH_MAJOR, ts_input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, ts_input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, ts_input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, ts_input_dev->absbit);
	set_bit(BTN_TOUCH, ts_input_dev->keybit);
	set_bit(EV_ABS, ts_input_dev->evbit);
	set_bit(EV_KEY, ts_input_dev->evbit);
	set_bit(INPUT_PROP_DIRECT, ts_input_dev->propbit);

	input_set_abs_params(ts_input_dev, ABS_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(ts_input_dev, ABS_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(ts_input_dev, ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(ts_input_dev, ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(ts_input_dev, ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
	input_set_abs_params(ts_input_dev, ABS_MT_WIDTH_MAJOR, 0, 20, 0, 0);
	input_set_abs_params(ts_input_dev, ABS_MT_TRACKING_ID, 0, 4, 0, 0);

	ret = input_register_device(ts_input_dev);
	if (ret){
		printk("input_register_device fail\n");
		return ret;
	}

	return 0;
}

void __exit zebra_ts_exit(void)
{
	input_unregister_device(ts_input_dev);
	input_free_device(ts_input_dev);
}

module_init(zebra_ts_init);
module_exit(zebra_ts_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("FlyZebra <flycnzebra@gmail.com>");
MODULE_DESCRIPTION("Touchscreen driver");
MODULE_ALIAS("platform:zebra-ts");
