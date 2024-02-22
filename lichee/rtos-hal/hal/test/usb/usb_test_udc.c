#include "usb_test.h"

int usb_test_cmd_udc(int argc, const char **argv)
{
	int c;
	if ((argc != 3) && (argc != 4))
		return -1;

	while ((c = getopt(argc, (char *const *)argv, "ir")) != -1) {
		switch (c) {
		case 'i':
#ifdef CONFIG_HAL_TEST_HCI
			// rmmod host driver before insmod otg
			if ((usb_test_is_otg(0) == 0) &&
					(hal_usb_get_otg_role() == USB_ROLE_HOST)) /*hci mode*/
				hal_usb_hcd_deinit(0);
#endif
			if (hal_usb_get_otg_role() != USB_ROLE_DEVICE) {
				usb_stop_otg_run();
				printf("[usb0] insmod device driver!\n");
				hal_gadget_init();
				hal_usb_set_otg_role(USB_ROLE_DEVICE);
			} else {
				printf("[usb0] is already device drvier!\n");
			}
			break;
		case 'r':
			if (hal_usb_get_otg_role() == USB_ROLE_DEVICE) {
				usb_stop_otg_run();
				printf("[usb0] rmmod device driver!\n");
				hal_gadget_exit();
				hal_usb_set_otg_role(USB_ROLE_NULL);
			} else {
				printf("[usb0] is not device, can't rmmod device driver!\n");
			}
			break;
		default:
			printf("err: insmod/rmmod error!\n");
			usb_test_show_help();
			break;
		}
	}

	return 0;
}
