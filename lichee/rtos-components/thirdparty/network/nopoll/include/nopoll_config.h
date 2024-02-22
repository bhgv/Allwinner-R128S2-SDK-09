/*
 * noPoll Library nopoll_config.h
 * Platform dependant definitions for Win32 platform.
 *
 * This file is maintained manually for those people that do not
 * compile nopoll using autoconf. It should look really similar to
 * nopoll_config.h file created on a i386 linux platform but changing
 * NOPOLL_OS_UNIX to NOPOLL_OS_WIN32 (at least for now).
 *
 *  For commercial support on build WebSocket enabled solutions contact us:
 *
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         Av. Juan Carlos I, Nº13, 2ºC
 *         Alcalá de Henares 28806 Madrid
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/nopoll
 *
 */

#ifndef __NOPOLL_CONFIG_H__
#define __NOPOLL_CONFIG_H__

/**
 * \addtogroup nopoll_decl_module
 * @{
 */

/**
 * @brief Allows to convert integer value (including constant values)
 * into a pointer representation.
 *
 * Use the oposite function to restore the value from a pointer to a
 * integer: \ref PTR_TO_INT.
 *
 * @param integer The integer value to cast to pointer.
 *
 * @return A \ref noPollPtr reference.
 */
#ifndef INT_TO_PTR
#define INT_TO_PTR(integer) ((noPollPtr) (unsigned long)(integer))
#endif

/**
 * @brief Allows to convert a pointer reference (\ref noPollPtr),
 * which stores an integer that was stored using \ref INT_TO_PTR.
 *
 * Use the oposite function to restore the pointer value stored in the
 * integer value.
 *
 * @param ptr The pointer to cast to a integer value.
 *
 * @return A int value.
 */
#ifndef PTR_TO_INT
#define PTR_TO_INT(ptr) ((int) (ptr))
#endif

/**
 * @brief Allows to get current platform configuration. This is used
 * by Nopoll library but could be used by applications built on top of
 * Nopoll to change its configuration based on the platform information.
 *
 * Note when this flag is enabled (set to 1), it means we are
 * compiling in a FreeRTOS platform.
 */
#define NOPOLL_OS_FREERTOS (1)

/**
 * @brief Indicates where we have support for LwIP.
 */
#define NOPOLL_LWIP (1)

/**
 * @brief Indicates where we do not have support for IPV6.
 */
#define NOPOLL_NO_IPV6 (1)

/**
 * @brief Indicates where we have support for mbedTLS.
 */
#define NOPOLL_MBEDTLS (1)

/**
 * @brief disable psram for SDK adaptation
 */
#define CONFIG_NOPOLL_HEAP_MODE 0

/* @} */

#endif
