/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_SETTINGS
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "cmd_shell.h"

#include <zephyr.h>

#include <settings/settings.h>
#include "ble/sys/util.h"
#include "ble/sys/byteorder.h"
#if (CONFIG_BT_SETTINGS)
#include "ble/settings/settings.h"
#endif

#define CONFIG_SHELL_CMD_BUFF_SIZE (128)
struct settings_list_callback_params {
	const struct shell *shell_ptr;
	const char *subtree;
};

static int settings_list_callback(const char *key,
                                  size_t len,
                                  settings_read_cb read_cb,
                                  void *cb_arg,
                                  void *param)
{
	ARG_UNUSED(len);
	ARG_UNUSED(read_cb);
	ARG_UNUSED(cb_arg);

	struct settings_list_callback_params *params = param;

	if (params->subtree != NULL) {
		shell_print(params->shell_ptr, "%s/%s", params->subtree, key);
	} else {
		shell_print(params->shell_ptr, "%s", key);
	}

	return 0;
}

static int cmd_settings_list(const struct shell *shell_ptr, size_t argc, char *argv[])
{
	int err;

	struct settings_list_callback_params params = {
		.shell_ptr = shell_ptr,
		.subtree = (argc == 2 ? argv[1] : NULL)
	};

	if (argc == 2 && !argv[1]) {
		shell_error(shell_ptr, "invalid argv");
		return -ENOEXEC;
	}

	err = settings_load_subtree_direct(params.subtree, settings_list_callback, &params);

	if (err) {
		shell_error(shell_ptr, "Failed to load settings: %d", err);
	}

	return err;
}

struct settings_read_callback_params {
	const struct shell *shell_ptr;
	bool value_found;
};

static int settings_read_callback(const char *key,
                                  size_t len,
                                  settings_read_cb read_cb,
                                  void *cb_arg,
                                  void *param)
{
	uint8_t buffer[SETTINGS_MAX_VAL_LEN];
	ssize_t num_read_bytes = MIN(len, SETTINGS_MAX_VAL_LEN);
	struct settings_read_callback_params *params = param;

	/* Process only the exact match and ignore descendants of the searched name */
	if (settings_name_next(key, NULL) != 0) {
		return 0;
	}

	params->value_found = true;
	num_read_bytes = read_cb(cb_arg, buffer, num_read_bytes);

	if (num_read_bytes < 0) {
		shell_error(params->shell_ptr, "Failed to read value: %d", (int) num_read_bytes);
		return 0;
	}

	shell_hexdump(params->shell_ptr, buffer, num_read_bytes);

	if (len > SETTINGS_MAX_VAL_LEN) {
		shell_print(params->shell_ptr, "(The output has been truncated)");
	}

	return 0;
}

static int cmd_settings_read(const struct shell *shell_ptr, size_t argc, char *argv[])
{
	int err;
	const char *name = argv[1];

	struct settings_read_callback_params params = {
		.shell_ptr = shell_ptr,
		.value_found = false
	};

	if (argc < 2) {
		shell_error(shell_ptr, "read key not specified");
		return -ENOEXEC;
	}
	if (!argv[1]) {
		shell_error(shell_ptr, "invalid argv");
		return -ENOEXEC;
	}

	err = settings_load_subtree_direct(name, settings_read_callback, &params);

	if (err) {
		shell_error(shell_ptr, "Failed to load setting: %d", err);
	} else if (!params.value_found) {
		err = -ENOENT;
		shell_error(shell_ptr, "Setting not found");
	}

	return err;
}

static int cmd_settings_write(const struct shell *shell_ptr, size_t argc, char *argv[])
{
	int err;
	uint8_t buffer[CONFIG_SHELL_CMD_BUFF_SIZE / 2];
	size_t buffer_len;

	if (argc < 3) {
		shell_error(shell_ptr, "write key/value not specified");
		return -ENOEXEC;
	}
	if (!argv[1] || !argv[2]) {
		shell_error(shell_ptr, "invalid argv");
		return -ENOEXEC;
	}

	buffer_len = hex2bin(argv[2], strlen(argv[2]), buffer, sizeof(buffer));

	if (buffer_len == 0) {
		shell_error(shell_ptr, "Failed to parse hex value");
		return -EINVAL;
	}

	err = settings_save_one(argv[1], buffer, buffer_len);

	if (err) {
		shell_error(shell_ptr, "Failed to write setting: %d", err);
	}

	return err;
}

static int cmd_settings_delete(const struct shell *shell_ptr, size_t argc, char *argv[])
{
	int err;

	if (argc < 2) {
		shell_error(shell_ptr, "delete key not specified");
		return -ENOEXEC;
	}
	if (!argv[1]) {
		shell_error(shell_ptr, "invalid argv");
		return -ENOEXEC;
	}

	err = settings_delete(argv[1]);

	if (err) {
		shell_error(shell_ptr, "Failed to delete setting: %d", err);
	}

	return err;
}

SHELL_STATIC_SUBCMD_SET_CREATE(settings_cmds,
                               SHELL_CMD_ARG(list, NULL, "[<subtree>]", cmd_settings_list, 1, 1),
                               SHELL_CMD_ARG(read, NULL, "<name>", cmd_settings_read, 2, 0),
                               SHELL_CMD_ARG(write, NULL, "<name> <hex>", cmd_settings_write, 3, 0),
                               SHELL_CMD_ARG(delete, NULL, "<name>", cmd_settings_delete, 2, 0),
                               SHELL_SUBCMD_SET_END);

static int cmd_settings(const struct shell *shell_ptr, size_t argc, char **argv)
{
	shell_error(shell_ptr, "%s unknown parameter: %s", argv[0], argv[1]);
	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(settings, &settings_cmds, "Settings shell commands", cmd_settings, 2, 0);

enum cmd_status cmd_settings_exec(char *cmd)
{
    return cmd_exec_shell(cmd, shell_settings_cmds, cmd_nitems(shell_settings_cmds) - 1);
}
#endif
