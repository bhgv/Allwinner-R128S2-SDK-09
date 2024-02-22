#include <lwip/sockets.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <hal_time.h>

#include "wv_log.h"
#include "wv_rtp_udp.h"

#define MAX_PAYLOAD_SIZE 1400 // This is based on MTU 1500 minus udp headers
#define RTP_PAYLOAD_JPEG 26   //RTP协议版本
#define RTP_VERSION 2 // RTP协议版本

#define TYPE_BASELINE_DCT_SEQUENTIAL 0
#define TYPE_0_SPECIFIC_PROGRESSIVE 0

static int socket_bind_udp(int port)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        LOG_E("Unable to create socket: errno %d", errno);
        return -1;
    }

    struct sockaddr_in serv_addr = {
            .sin_family  = PF_INET,
            .sin_addr    = {
                    .s_addr = INADDR_ANY
                    },
            .sin_port    = htons(port)
    };

    int option = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
        LOG_E("Unable to set socket SO_REUSEADDR: errno %d", errno);
        return -1;
    }

    int err = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (err != 0) {
        LOG_E("Socket unable to bind: errno %d", errno);
        goto CLEAN_UP;
    }

    return sockfd;

CLEAN_UP:
    close(sockfd);
    return -1;
}

static int serialize_header(rtp_header_t header, uint8_t *buffer, size_t length)
{
    assert(buffer != NULL);
    assert(length >= 12);

    buffer[0] = 0x80; // Version 2, no padding, no extensions, no csrc
    buffer[1] = header.payload_type;
    if (header.marker) {
        buffer[1] |= 1 << 7;
    }

    uint16_t *sequence_number = (uint16_t *) &buffer[2];
    *sequence_number = PP_HTONS(header.sequence_number);

    uint32_t *timestamp = (uint32_t *) &buffer[4];
    *timestamp = PP_HTONL(header.timestamp);

    uint32_t *ssrc = (uint32_t *) &buffer[8];
    *ssrc = PP_HTONL(header.ssrc);

    return 12;
}

#if 0
static int serialize_jpeg_header(rtp_jpeg_header_t header, uint8_t *buffer, size_t length)
{
    assert(buffer != NULL);
    assert(length >= 8);

    // actually 24 bits
    uint32_t *fragment_offset = (uint32_t *) &buffer[0];
    *fragment_offset = PP_HTONL(header.fragment_offset);

    buffer[0] = header.type;

    buffer[4] = header.type_specific;
    buffer[5] = header.q;
    buffer[6] = header.width / 8;
    buffer[7] = header.height / 8;

    return 8;
}

static int serialize_quant_tables(rtp_quant_t quant, uint8_t *buffer, size_t length)
{
    assert(buffer != NULL);

    if (length < quant.length + 4) {
        LOG_E("No enough space in buffer for quant tables");
        return -1;
    }

    buffer[0] = quant.mbz;
    buffer[1] = quant.precision;

    uint16_t *size = (uint16_t *) &buffer[2];
    *size = PP_HTONS(quant.length);

    memcpy(buffer+4, quant.table0, 64);
    memcpy(buffer+68, quant.table1, 64);

    return quant.length + 4;
}

uint32_t test_timestamp = 0;
int rtp_send_jpeg(rtp_session_t *rtp_session, uint8_t *frame, size_t frame_length)
{
    if (!rtp_session) {
        return -1;
    }

    if (!rtp_session->initialized) {
        return -1;
    }

    rtp_jpeg_data_t jpeg_data;
    unsigned int  ts_inscreate = (unsigned int)(90000/25);

    if (wv_jpeg_decode((char *)frame, frame_length, &jpeg_data) != 0) {
        LOG_E("Failed to parse jpeg data");
        return -1;
    }

    rtp_header_t rtp_header = {
            .payload_type = RTP_PAYLOAD_JPEG,
            .ssrc = 12348765,         // TODO make this random and unique
            .timestamp = test_timestamp, //Increase timestamp per rtp frame
            .sequence_number = 0,
            .marker = 0
    };

    rtp_jpeg_header_t rtp_jpeg_header = {
            .height = 240,             // TODO get this from camera or image
            .width = 320,              // TODO get this from camera or image
            .q = 12,                   // TODO get this from camera or image
            .type = TYPE_BASELINE_DCT_SEQUENTIAL,
            .type_specific = TYPE_0_SPECIFIC_PROGRESSIVE,
            .fragment_offset = 0,
    };

    test_timestamp += ts_inscreate;

    while (rtp_jpeg_header.fragment_offset < jpeg_data.jpeg_data_length) {
        static uint8_t payload[MAX_PAYLOAD_SIZE];
        uint8_t *offset = payload;
        size_t payload_remaining = MAX_PAYLOAD_SIZE;

        rtp_header.sequence_number = rtp_session->sequence_number++; // Increase sequence per packet

#if 0
        int include_quant = jpeg_data.quant_table_0 && jpeg_data.quant_table_1 && rtp_jpeg_header.fragment_offset == 0;
        if (include_quant) {
            rtp_jpeg_header.q |= 1 << 7;
        } else {
            rtp_jpeg_header.q &= 0b0111111;
        }
#endif
        //int last_packet = (jpeg_data.jpeg_data_length - rtp_jpeg_header.fragment_offset + 20) < payload_remaining;
        int last_packet = (jpeg_data.jpeg_data_length - rtp_jpeg_header.fragment_offset + 12) < payload_remaining;
        rtp_header.marker = last_packet;

#if 0
        LOG_D("Serializing RTP/AVP packet, offset=%d, quant=%d, last=%d, timestamp=%ud, sequence=%ud",
                 rtp_jpeg_header.fragment_offset, include_quant, last_packet,
                 rtp_session->timestamp, rtp_session->sequence_number);
#endif
        int n = serialize_header(rtp_header, offset, payload_remaining);
        if (n < 0) {
            return -1;
        };
        payload_remaining -= n;
        offset += n;
#if 0
        n = serialize_jpeg_header(rtp_jpeg_header, offset, payload_remaining);
        if (n < 0) {
            return -1;
        };
        payload_remaining -= n;
        offset += n;

        if (include_quant) {
            rtp_quant_t quant = RTP_QUANT_DEFAULT();
            memcpy(quant.table0, jpeg_data.quant_table_0 + 5, 64);
            memcpy(quant.table1, jpeg_data.quant_table_1 + 5, 64);

            n = serialize_quant_tables(quant, offset, payload_remaining);
            if (n < 0) {
                return -1;
            };
            payload_remaining -= n;
            offset += n;
        }
#endif
        if (last_packet) {
            size_t remaining_bytes =  jpeg_data.jpeg_data_length - rtp_jpeg_header.fragment_offset;
            memcpy(offset, jpeg_data.jpeg_data_start + rtp_jpeg_header.fragment_offset, remaining_bytes);
            payload_remaining -= remaining_bytes;
            rtp_jpeg_header.fragment_offset += remaining_bytes;
        } else {
            memcpy(offset, jpeg_data.jpeg_data_start + rtp_jpeg_header.fragment_offset, payload_remaining);
            rtp_jpeg_header.fragment_offset += payload_remaining;
            payload_remaining = 0;
        }

        const struct sockaddr_in client = {
                .sin_family = AF_INET,
                .sin_addr.s_addr = inet_addr(rtp_session->dst_addr),
                .sin_port = htons(rtp_session->dst_rtp_port)
        };

        int retries = 5;  // ENOMEM might occur if the buffer in LWIP is full
        size_t size = MAX_PAYLOAD_SIZE - payload_remaining;
        do {
            ssize_t sent = sendto(rtp_session->rtp_socket, payload, size, 0,
                                  (const struct sockaddr *) &client, sizeof(client));

            if (sent < 0 && errno != ENOMEM) {
                LOG_E("Failed to sent RTP package: %d", errno);
                return -1;
            }

            if (sent == size) {
                break;
            }

            // We might need to take some time to clear the transmit buffers
            //hal_udelay(250);
            retries--;
        } while (retries > 0);
    }
    return 0;
}
#endif

int rtp_init(rtp_session_t **rtp_session, int dst_rtp_port, int dst_rtcp_port, char *dst_addr_string)
{
    rtp_session_t *session = calloc(1, sizeof(rtp_session_t));
    if (!session) {
        return -1;
    }

    memset(session, 0, sizeof(rtp_session_t));
    session->dst_rtp_port = dst_rtp_port;
    session->dst_rtcp_port = dst_rtcp_port;

    // Find two consecutive ports in the range 9001 - 65534
    for (int port = 9001; port < 0xFFFE; port += 2) {
        int rtp_sock = socket_bind_udp(port);
        if (rtp_sock < 0) {
            continue;
        }

        LOG_I("RTP Socket bound, port %d", port);

        int rtcp_sock = socket_bind_udp(port + 1);
        if (rtcp_sock < 0) {
            close(rtp_sock);
            continue;
        }

        LOG_I("RTCP Socket bound, port %d", port);
        session->rtp_socket = rtp_sock;
        session->src_rtp_port = port;

        session->rtcp_socket = rtcp_sock;
        session->src_rtcp_port = port + 1;
        break;
    }

    if (session->rtp_socket <= 0) {
        LOG_E("Unable to prepare UDP sockets for RTP/RTCP");
        free(session);
        session = NULL;
        return -1;
    }

    memcpy(session->dst_addr, dst_addr_string, sizeof(session->dst_addr));

    session->timestamp = rand() >> 16;
    session->sequence_number = 0;
    session->initialized = true;

    *rtp_session = session;

    return 0;
}

int rtp_teardown(rtp_session_t *rtp_session)
{
    if (!rtp_session) {
        return -1;
    }

    if (rtp_session->initialized) {
        shutdown(rtp_session->rtp_socket, 0);
        close (rtp_session->rtp_socket);

        shutdown(rtp_session->rtcp_socket, 0);
        close(rtp_session->rtcp_socket);
    }

    free(rtp_session);

    return 0;
}

int rtp_send_jpeg(rtp_session_t *rtp_session, uint8_t *frame, uint32_t len)
{
    uint32_t fragment_offset = 0;
    static uint32_t timestamp = 0;

    if (!rtp_session) {
        return -1;
    }

    if (!rtp_session->initialized) {
        return -1;
    }

    rtp_header_t rtp_header = {
        .payload_type = RTP_PAYLOAD_JPEG,
        .ssrc = 11223344,         // TODO make this random and unique
        .timestamp = 0,   //Increase timestamp per rtp frame
        .sequence_number = 0,
        .marker = 0
    };

    uint32_t  ts_inscreate = (unsigned int)(90000/25);
    timestamp += ts_inscreate;

    while (fragment_offset < len) {
        static uint8_t payload[MAX_PAYLOAD_SIZE];
        uint8_t *offset = payload;
        uint16_t payload_remaining = MAX_PAYLOAD_SIZE;

        // Increase sequence per packet
        rtp_header.sequence_number = rtp_session->sequence_number++;
        rtp_header.timestamp = timestamp;

        int last_packet = (len - fragment_offset + 12) < payload_remaining;
        rtp_header.marker = last_packet;

        int n = serialize_header(rtp_header, offset, payload_remaining);
        if (n < 0) {
            return -1;
        };
        payload_remaining -= n;
        offset += n;

        if (last_packet) {
            uint32_t remaining_bytes =  len - fragment_offset;
            memcpy(offset, frame + fragment_offset, remaining_bytes);
            payload_remaining -= remaining_bytes;
            fragment_offset += remaining_bytes;
        } else {
            memcpy(offset, frame + fragment_offset, payload_remaining);
            fragment_offset += payload_remaining;
            payload_remaining = 0;
        }

        const struct sockaddr_in client = {
                .sin_family = AF_INET,
                .sin_addr.s_addr = inet_addr(rtp_session->dst_addr),
                .sin_port = htons(rtp_session->dst_rtp_port)
        };

        int retries = 5;  // ENOMEM might occur if the buffer in LWIP is full
        uint16_t size = MAX_PAYLOAD_SIZE - payload_remaining;

        do {
            uint16_t sent = sendto(rtp_session->rtp_socket, payload, size, 0,
                                  (const struct sockaddr *) &client, sizeof(client));

            if (sent < 0 && errno != ENOMEM) {
                LOG_E("Failed to sent RTP package: %d", errno);
                return -1;
            }

            if (sent == size) {
                break;
            }

            //take some time to clear the transmit buffers
            hal_usleep(100);
            retries--;
        } while (retries > 0);
    }
    return 0;
}

int rtp_get_src_rtp_port(rtp_session_t *rtp_session)
{
    if (!rtp_session) {
        return -1;
    }

    if (!rtp_session->initialized) {
        return -1;
    }

    return rtp_session->src_rtp_port;
}

int rtp_get_src_rtcp_port(rtp_session_t *rtp_session)
{
    if (!rtp_session) {
        return -1;
    }

    if (!rtp_session->initialized) {
        return -1;
    }

    return rtp_session->src_rtcp_port;
}
