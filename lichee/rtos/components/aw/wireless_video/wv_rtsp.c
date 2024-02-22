#include <FreeRTOS.h>
#include <task.h>
#include <stdbool.h>
#include <stdlib.h>

#include "wv_log.h"
#include "wv_rtsp.h"

static void rtsp_server_task(void *pvParameters)
{
    rtsp_server_main();
    vTaskDelete(NULL);
}

int rtsp_server_start(rtsp_server_t **server_handle)
{
    if (!server_handle) {
        return -1;
    }

    rtsp_server_t *server = calloc(1, sizeof(rtsp_server_t));
    if (!server) {
        return -1;
    }

    BaseType_t result = xTaskCreate(rtsp_server_task, "rtsp_tcp_server",
                        SERVER_STACKSIZE, NULL, SERVER_PRIORITY, &server->server_taskhandle);
    if (result != pdPASS) {
        LOG_E("Failed to create rtsp server task: %d", result);
        free(server);
        return -1;
    }

    server->running = true;
    *server_handle = server;

    return 0;
}

int rtsp_server_stop(rtsp_server_t *server_handle)
{
    if (!server_handle) {
        return -1;
    }

    rtsp_server_t *server = (rtsp_server_t *)server_handle;

    if (server->running) {
        vTaskDelete(server->server_taskhandle);
        server->running = false;
    }

    free(server);

    return 0;
}