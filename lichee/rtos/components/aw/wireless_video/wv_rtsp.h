#ifndef WV_RTSP_H
#define WV_RTSP_H

#include <task.h>
#include <stdbool.h>

#define URL_MAX_LENGTH 1024
#define SERVER_STACKSIZE 1024
#define SERVER_PRIORITY  17

typedef struct {
    bool running;
    TaskHandle_t server_taskhandle;
} rtsp_server_t;

typedef enum {
    OPTIONS,
    DESCRIBE,
    SETUP,
    PLAY,
    TEARDOWN,
    UNSUPPORTED
} rtsp_request_type_t;

typedef struct {
    rtsp_request_type_t request_type;
    char url[URL_MAX_LENGTH + 1];
    int protocol_version;
    int cseq;
    int dst_rtp_port;
    int dst_rtcp_port;
} rtsp_req_t;

#define PARSER_OK 0
#define PARSER_FAIL -1
#define PARSER_NOMEM -2
#define PARSER_INVALID_STATE -3
#define PARSER_INVALID_ARGS -4

#define PARSE_INIT() { \
     .parse_complete = false, \
     .state = RTSP_PARSER_PARSE_METHOD, \
     .intermediate_len = 0   \
     }

#define RTSP_PARSER_PARSE_METHOD 0
#define RTSP_PARSER_PARSE_URL 1
#define RTSP_PARSER_PARSE_PROTOCOL 2
#define RTSP_PARSER_OPTIONAL_HEADER 9
#define RTSP_PARSER_PARSE_HEADER 10
#define RTSP_PARSER_PARSE_HEADER_VALUE 11
#define RTSP_PARSER_PARSE_HEADER_WS 12

typedef struct {
    int state;
    int parse_complete;
    int error;
    char intermediate[1024];
    size_t intermediate_len;
    rtsp_req_t *request;
} rtsp_parser_t;

int rtsp_parser_init(rtsp_parser_t **parser_handle);
int rtsp_parse_request(rtsp_parser_t *parser_handle, const char *buffer, size_t len);
int rtsp_parser_is_complete(rtsp_parser_t *parser_handle);
int rtsp_parser_get_error(rtsp_parser_t *parser_handle);
rtsp_req_t *rtsp_parser_get_request(rtsp_parser_t *parser_handle);
int rtsp_parser_free(rtsp_parser_t *parser_handle);

int rtsp_server_main();
int rtsp_server_start(rtsp_server_t **server_handle);
int rtsp_server_stop(rtsp_server_t *server_handle);

#endif //WV_RTSP_H
