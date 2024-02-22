/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _BT_VENDOR_H_
#define _BT_VENDOR_H_


#ifdef __cplusplus
extern "C" {
#endif

#define STREAM_TO_U8(u8, p)             {u8 = (uint8_t)(*(p)); (p) += 1;}
#define STREAM_TO_U16(u16, p)           {u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); (p) += 2;}
#define STREAM_SKIP_U8(p)               { (p) += 1; }

#define HCI_OPCODE(ogf, ocf)            (((ogf) << 10) + (ocf))

#define HCI_OGF_VENDOR_SPEC             0x3F

#define HCI_OCF_VS_COEX_STATE           0x60

#define HCI_OPCODE_VS_COEX_STATE        HCI_OPCODE(HCI_OGF_VENDOR_SPEC, HCI_OCF_VS_COEX_STATE)

#define HCI_EVENT_VS                    0xFF


/**
 * @brief Abstraction which represents the VS transport to the controller.
 *
 * This struct is used to represent the VS transport to the controller.
 */
struct vs_driver {
	const char *name;
	int (*open)(void);
	int (*close)(void);
	int (*send)(uint8_t type, uint8_t *data, uint16_t len);
};

/**
 * @brief Receive VS data from controller.
 *
 * @param data containing data to be received from the controller.
 * @param len the length of data to be received.
 *
 * @return 0 on success or negative error number on failure.
 */
int vs_recv(const uint8_t *data, uint16_t len);

/**
 * @brief Send VS data to controller.
 *
 * @param data containing data to be sent to the controller.
 * @param len the length of data to be sent.
 *
 * @return 0 on success or negative error number on failure.
 */
int vs_send(uint8_t *data, uint16_t len);

/**
 * @brief Register VS driver interfaces.
 *
 * @param drv point to driver interfaces.
 *
 * @return 0 on success or negative error number on failure.
 */
int vs_driver_register(const struct vs_driver *drv);

/**
 * @brief Unregister VS driver interfaces.
 *
 * @return 0 on success or negative error number on failure.
 */
int vs_driver_unregister(void);

#ifdef __cplusplus
}
#endif

#endif /* _BT_VENDOR_H_ */
