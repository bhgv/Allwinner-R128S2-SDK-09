#include <sys/param.h>
#include <stdlib.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "csi/hal_csi_jpeg.h"
#include "hal_cache.h"
#include "hal_mem.h"
#include "jpegenc.h"
#include "wv_log.h"
#include "wv_rtsp.h"
#include "wv_rtp_udp.h"

#define PORT 554  // IANA default for RTSP
#define MAX_CLIENTS 3

typedef struct {
    int connection_active;
    int socket;
    char client_addr_string[128];
    rtsp_parser_t *parser;
    rtp_session_t *rtp_session;
    TaskHandle_t rtp_player_task;
} rtsp_server_connection_t;

rtsp_server_connection_t connections[MAX_CLIENTS];

static int rtsp_handle_error(rtsp_server_connection_t *, int);

static void handle_options(rtsp_server_connection_t *connection, rtsp_req_t *request)
{
    char buffer[2048];
    size_t msgsize = snprintf(buffer, 2048,
                              "RTSP/1.0 200 OK\r\n"
                              "cSeq: %d\r\n"
                              "Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE\r\n"
                              "Server: XR TEST Server\r\n"
                              "\r\n",
                              request->cseq);
    size_t sent = send(connection->socket, buffer, msgsize, 0);
    LOG_I("RTSP >: %s", buffer);
    if (sent != msgsize) {
        LOG_W("Mismatch between msgsize and sent bytes: %d vs %d", msgsize, sent);
    }
}

static void handle_setup(rtsp_server_connection_t *connection, rtsp_req_t *request)
{
    int err = rtp_init(&connection->rtp_session, request->dst_rtp_port, request->dst_rtcp_port, connection->client_addr_string);
    if (err < 0) {
        LOG_W("Failed to initialize the rtp connection");
    }

    char buffer[2048];
    size_t msgsize = snprintf(buffer, 2048,
                              "RTSP/1.0 200 OK\r\n"
                              "cSeq: %d\r\n"
                              "Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
                              "Session: 11223344\r\n"
                              "\r\n",
                              request->cseq,
                              request->dst_rtp_port,
                              request->dst_rtcp_port,
                              rtp_get_src_rtp_port(connection->rtp_session),
                              rtp_get_src_rtcp_port(connection->rtp_session));
    size_t sent = send(connection->socket, buffer, msgsize, 0);
    LOG_I("RTSP >: %s", buffer);
    if (sent != msgsize) {
        LOG_W("Mismatch between msgsize and sent bytes: %d vs %d", msgsize, sent);
    }
}

static void handle_describe(rtsp_server_connection_t *connection, rtsp_req_t *request)
{
    static char sdp[2048];
    size_t sdp_size = snprintf(sdp, 2048,
                               "v=0\r\n"
                               "o=- %d 1 IN IP4 %s\r\n"
                               "s=\r\n"
                               "t=0 0\r\n"
                               "m=video 10000 RTP/AVP 26\r\n"
                               "a=rtpmap:26 JPEG/90000\r\n"
                               "a=decode_buf=300\r\n"
                               "a=framerate:25\r\n"
                               "c=IN IP4 0.0.0.0\r\n",
                                11223344,
                               "192.168.31.200");

    static char buffer[2048];
    size_t msgsize = snprintf(buffer, 2048,
                              "RTSP/1.0 200 OK\r\n"
                              "cSeq: %d\r\n"
                              "Content-Type: application/sdp\r\n"
                              "Content-Base: %s\r\n"
                              "Server: XR TEST Server\r\n"
                              "Content-Length: %d\r\n"
                              "\r\n",
                              request->cseq,
                              request->url,
                              sdp_size);

    // Send header
    size_t sent = send(connection->socket, buffer, msgsize, 0);
    LOG_I("RTSP >: %s", buffer);
    if (sent != msgsize) {
        LOG_W("Mismatch between msgsize and sent bytes: %d vs %d", msgsize, sent);
    }

    // Send body
    sent = send(connection->socket, sdp, sdp_size, 0);
    LOG_I("RTSP >: %s", sdp);
    if (sent != sdp_size) {
        LOG_W("Mismatch between sdp_size and sent bytes: %d vs %d", sdp_size, sent);
    }
}

static uint8_t* gaddr[3];
static uint32_t gsize[3];

void jpeg_online_cb(struct csi_jpeg_mem *jpeg_mem)
{
    hal_dcache_clean_invalidate((unsigned long)jpeg_mem->buf.addr, jpeg_mem->buf.size);

    memcpy(gaddr[jpeg_mem->index], jpeg_mem->buf.addr - JPEG_HEADER_LEN,
            jpeg_mem->buf.size + JPEG_HEADER_LEN);
    gsize[jpeg_mem->index] = jpeg_mem->buf.size + JPEG_HEADER_LEN;
}

void video_player_task(void *pvParameters)
{
    unsigned int len;
    unsigned int count = 3, i;
    unsigned int timeout_msec = 2000;
    struct csi_jpeg_fmt fmt;
    struct csi_jpeg_mem *jpeg_mem;
    unsigned int width, height;
    unsigned char eoi[2] = {0xff, 0xd9};

    if (!pvParameters) {
        LOG_E("Invalid arguments");
        vTaskDelete(NULL);
        return;
    }

    rtp_session_t *session = pvParameters;
    hal_csi_sensor_get_sizes(&width, &height);
    LOG_I("Sensor width = %d, height = %d", width, height);

    fmt.width = width;
    fmt.height = height;
    fmt.line_mode = ONLINE_MODE;
    fmt.output_mode = PIX_FMT_OUT_MAX;
    fmt.cb = (CapStatusCb)&jpeg_online_cb;
    hal_csi_jpeg_set_fmt(&fmt);

    if (hal_csi_jpeg_reqbuf(count) != 0) {
        vTaskDelete(NULL);
        return ;
    }

    for (i = 0; i < count; i++) {
        gaddr[i] = (uint8_t*)hal_malloc(50*1024 + JPEG_HEADER_LEN);
        if (gaddr[i]) {
            memset(gaddr[i], 0 , 50*1024 + JPEG_HEADER_LEN);
        } else {
            LOG_E("[%s,%d] jpeg pic malloc fail!\n", __func__, __LINE__);
            return;
        }
    }

    hal_csi_jpeg_s_stream(1);
    LOG_D("RTP[UDP] transmission start");

    while (1) {
        jpeg_mem = hal_jpeg_dqbuf(timeout_msec);
        if (jpeg_mem == NULL) {
            continue;
        }

        memcpy(gaddr[jpeg_mem->index] + gsize[jpeg_mem->index], eoi, 2);
        gsize[jpeg_mem->index] += 2;
        rtp_send_jpeg(session, gaddr[jpeg_mem->index], gsize[jpeg_mem->index]);
        hal_jpeg_qbuf();
    }
}

static void handle_play(rtsp_server_connection_t *connection, rtsp_req_t *request)
{
    BaseType_t result = xTaskCreate(video_player_task, "rtp_server", 1024, connection->rtp_session, 17, &connection->rtp_player_task);
    if (result != pdPASS) {
        LOG_E("Failed to create rtp server task: %d", result);
        send(connection->socket, "RTSP/1.0 500 Internal Server Error\r\n\r\n", 38, 0);
        return;
    }

    static char buffer[2048];
    size_t msgsize = snprintf(buffer, 2048,
                              "RTSP/1.0 200 OK\r\n"
                              "cSeq: %d\r\n"
                              "Session: %d\r\n"
                              "Server: XR TEST Server\r\n"
                              "Range: npt=0.000-\r\n"
                              "\r\n",
                              request->cseq,
                              12348765);

    size_t sent = send(connection->socket, buffer, msgsize, 0);
    LOG_I("RTSP >:\n %s", buffer);
    if (sent != msgsize) {
        LOG_W("Mismatch between msgsize and sent bytes: %d vs %d", msgsize, sent);
    }
}

static void handle_teardown(rtsp_server_connection_t *connection, rtsp_req_t *request)
{
    if (!connection->connection_active) {
        rtsp_handle_error(connection, 400);
        return;
    }

    if (connection->rtp_player_task) {
        vTaskDelete(connection->rtp_player_task);
        connection->rtp_player_task = NULL;
    }

    if (connection->rtp_session) {
        rtp_teardown(connection->rtp_session);
        connection->rtp_session = NULL;
    }

    static char buffer[2048];
    size_t msgsize = snprintf(buffer, 2048,
                              "RTSP/1.0 200 OK\r\n"
                              "cSeq: %d\r\n"
                              "Server: XR TEST Server\r\n"
                              "\r\n",
                              request->cseq);

    size_t sent = send(connection->socket, buffer, msgsize, 0);
    LOG_I("RTSP >: %s", buffer);
    if (sent != msgsize) {
        LOG_W("Mismatch between msgsize and sent bytes: %d vs %d", msgsize, sent);
    }
}

static int rtsp_server_connection_close(rtsp_server_connection_t *connection)
{
    LOG_I("Closing connection with %s", connection->client_addr_string);
    if (!connection->connection_active) {
        return 0;
    }

    if (connection->parser) {
        rtsp_req_t *request = rtsp_parser_get_request(connection->parser);
        rtsp_parser_free(connection->parser);
        if (request) {
            free(request);
        }
    }

    if (connection->rtp_player_task) {
        hal_csi_jpeg_s_stream(0);
        hal_csi_jpeg_freebuf();
        vTaskDelete(connection->rtp_player_task);
    }

    if (connection->rtp_session) {
        rtp_teardown(connection->rtp_session);
    }

    connection->connection_active = false;
    shutdown(connection->socket, 0);
    close(connection->socket);

    memset(connection, 0, sizeof(rtsp_server_connection_t));

    return 0;
}

static int rtsp_server_read_block(rtsp_server_connection_t *connection)
{
    char buffer[1024];

    if (!connection->connection_active) {
        return -1;
    }

    int sock = connection->socket;

    int n = read(sock, buffer, 1023);
    if (n < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            LOG_E("Receive timeout");
            return -1;
        } else {
            LOG_E("Read error: %d", errno);
            return -1;
        }
    }

    if (n == 0) {
        return n;
    }

    buffer[n] = 0x0;

    LOG_I("RTSP read [%d bytes]:", n);
    printf("********************************************************************\n");
    printf("%s\n", buffer);
    printf("********************************************************************\n");
    if (rtsp_parse_request(connection->parser, buffer, n) < 0) {
        LOG_E("Error parsing request");
        return -1;
    }

    return n;
}

static int rtsp_handle_request(rtsp_server_connection_t *connection, rtsp_req_t *request)
{
    if (!connection->connection_active) {
        return -1;
    }

    switch (request->request_type) {
        case OPTIONS:
            handle_options(connection, request);
            break;
        case SETUP:
            handle_setup(connection, request);
            break;
        case DESCRIBE:
            handle_describe(connection, request);
            break;
        case PLAY:
            handle_play(connection, request);
            break;
        case TEARDOWN:
            handle_teardown(connection, request);
            break;
        default:
            return rtsp_handle_error(connection, 405);
    }

    return 0;
}

static int rtsp_handle_error(rtsp_server_connection_t *connection, int error)
{
    if (!connection->connection_active) {
    return -1;
    }

    int sock = connection->socket;

    if (error == 461) {
        LOG_I("RTSP >: %s", "RTSP/1.0 461 Unsupported Transport\r\n\r\n");
        send(sock, "RTSP/1.0 461 Unsupported Transport\r\n\r\n", 38, 0);
        return 0;
    }

    if (error == 405) {
        static char buffer[2048];
        size_t msgsize = snprintf(buffer, 2048,
                                  "RTSP/1.0 405 Method Not Allowed\r\n"
                                  "Server: XR TEST Server\r\n"
                                  "Allow: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE\r\n"
                                  "\r\n");
        size_t sent = send(connection->socket, buffer, msgsize, 0);
        LOG_I("RTSP >: %s", buffer);
        if (sent != msgsize) {
            LOG_W("Mismatch between msgsize and sent bytes: %d vs %d", msgsize, sent);
        }
        return 0;
    }

    LOG_I("RTSP >: %s", "RTSP/1.0 400 Bad Request\r\n\r\n");
    send(sock, "RTSP/1.0 400 Bad Request\r\n\r\n", 28, 0);

    return 0;
}

static int rtsp_handle_read(rtsp_server_connection_t *connection)
{
    if (!connection) {
        LOG_E("Read on socket %d, but no registered connection", connection->socket);
        return -1;
    }

    if (!connection->connection_active) {
        LOG_E("Read on socket inactive socket %d", connection->socket);
        return -1;
    }

    ssize_t n = rtsp_server_read_block(connection);
    if (n < 0) {
        LOG_E("Read failed");
        rtsp_server_connection_close(connection);
        return -1;
    }

    if (n == 0) {
        rtsp_server_connection_close(connection);
        return -1;
    }

    int error = rtsp_parser_get_error(connection->parser);
    if (error) {
        rtsp_handle_error(connection, error);
        LOG_D("Closing connection after bad request error");
        rtsp_server_connection_close(connection);
        return -1;
    }

    if (rtsp_parser_is_complete(connection->parser)) {
        rtsp_req_t *request = rtsp_parser_get_request(connection->parser);
        rtsp_parser_free(connection->parser);
        rtsp_parser_init(&connection->parser);

        if (rtsp_handle_request(connection, request) < 0) {
            LOG_W("Failed to handle request %d", request->request_type);
            return -1;
        }

        free(request);
    }

    return 0;
}

static int rtsp_create_listening_socket(int port)
{
    int listen_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        LOG_E("Unable to create socket: errno %d", errno);
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    int option = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
        LOG_E("Unable to set socket SO_REUSEADDR: errno %d", errno);
        goto CLEAN_UP;
    }

    int err = bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err != 0) {
        LOG_E("Socket unable to bind: errno %d", errno);
        goto CLEAN_UP;
    }

    err = listen(listen_sock, MAX_CLIENTS);
    if (err != 0) {
        LOG_E("Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    LOG_I("listening socket on port %d, Waiting connect...", port);
    return listen_sock;

CLEAN_UP:
    close(listen_sock);
    return -1;
}

static int rtsp_server_accept(int listen_sock)
{
    int keepAlive = 1;
    int keepIdle = 5;
    int keepInterval = 5;
    int keepCount = 3;
    struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
    socklen_t addr_len = sizeof(source_addr);

    int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
    if (sock < 0) {
        LOG_E("Unable to accept connection: errno %d", errno);
        return -1;
    }

    rtsp_server_connection_t *connection = NULL;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!connections[i].connection_active) {
            //claim this connection
            connections[i].connection_active = true;
            connection = &connections[i];
            break;
        }
    }

    if (!connection) {
        LOG_W("No free connections");
        goto fail;
    }

    // Set tcp keepalive option
    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive))) {
        LOG_E("Error setsockopt(SO_KEEPALIVE) failed");
        goto fail;
    }
    if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(keepIdle))) {
         LOG_E("Error setsockopt(TCP_KEEPIDLE) failed");
         goto fail;
    }
    if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval))) {
        LOG_E("Error setsockopt(TCP_KEEPINTVL) failed");
        goto fail;
    }
    if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount))) {
        LOG_E("Error setsockopt(TCP_KEEPCNT) failed");
        goto fail;
    }

    struct timeval to;
    to.tv_sec = 3;
    to.tv_usec = 0;

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to)) < 0) {
        LOG_W("Set recv timeout failed");
        goto fail;
    }

    // Convert ip address to string
    if (source_addr.ss_family == PF_INET) {
        inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, connection->client_addr_string, sizeof(connection->client_addr_string) - 1);
    }

    LOG_I("TCP Socket accepted ip address: %s", connection->client_addr_string);

    connection->socket = sock;

    if (rtsp_parser_init(&connection->parser) < 0) {
        LOG_E("Failed to create parser state for connection");
        goto fail;
    }

    return 0;

fail:
    shutdown(sock, 0);
    close(sock);
    return -1;
}

int rtsp_server_main()
{
    int listen_sock = rtsp_create_listening_socket(PORT);
    if (listen_sock < 0) {
        return -1;
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        connections[i].connection_active = false;
        connections[i].socket = -1;
    }

    while (1) {
        int sock_max = listen_sock;
        fd_set read_set;

        FD_ZERO(&read_set);
        FD_SET(listen_sock, &read_set);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (connections[i].connection_active) {
                FD_SET(connections[i].socket, &read_set);
                if (connections[i].socket > sock_max) {
                    sock_max = connections[i].socket;
                }
            }
        }

        LOG_D("Entering Socket select");
        int n = select(sock_max + 1, &read_set, NULL, NULL, NULL);
        if (n < 0) {
            if (errno == EINTR) {
                LOG_W("select interrupted");
                continue;
            }

            LOG_E("Failure in select, errno %d", errno);
            break;
        }

        if (FD_ISSET(listen_sock, &read_set)) {
            LOG_D("Read on listen socket");
            int err = rtsp_server_accept(listen_sock);
            if (err != 0) {
                LOG_W("Failed to accept connection");
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (FD_ISSET(connections[i].socket, &read_set)) {
                LOG_D("Read on connection %d", i);
                rtsp_handle_read(&connections[i]);
            }
        }
   }

    LOG_I("Shutting down listening socket");
    close(listen_sock);

    return 0;
}

