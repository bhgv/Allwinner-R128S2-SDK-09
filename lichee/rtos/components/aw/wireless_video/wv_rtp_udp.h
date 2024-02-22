#ifndef WV_RTP_UDP_H
#define WV_RTP_UDP_H

// RFC3550 RTP: A Transport Protocol for Real-Time Applications
// 5.1 RTP Fixed Header Fields (p12)
/*
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V=2|P|X|   CC  |M|     PT      |      sequence number          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                synchronization source (SSRC) identifier       |
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
|                 contributing source (CSRC) identifiers        |
|                               ....                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

//https://www.rfc-editor.org/rfc/rfc2435
// RTP JPEG header
/*
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Type-specific |              Fragment Offset                  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      Type     |       Q       |     Width     |     Height    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

// RQuantization Table heade r
/*
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      MBZ      |   Precision   |             Length            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Quantization Table Data                    |
   |                              ...                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

typedef struct {
    int initialized;
    uint16_t rtp_socket;
    uint16_t rtcp_socket;
    uint16_t src_rtp_port;
    uint16_t src_rtcp_port;
    char dst_addr[128];
    uint16_t dst_rtp_port;
    uint16_t dst_rtcp_port;
    uint32_t timestamp;
    uint32_t sequence_number;
} rtp_session_t;

typedef struct {
    uint8_t payload_type;
    uint8_t marker;
    uint16_t sequence_number;
    uint32_t timestamp;
    uint32_t ssrc;
} rtp_header_t;

#if 0
typedef struct {
    uint8_t type_specific;
    uint16_t fragment_offset;
    uint8_t type;
    uint8_t q;
    uint16_t width;
    uint16_t height;
} rtp_jpeg_header_t;

typedef struct {
    char *jpeg_data_start;
    size_t jpeg_data_length;
    char *quant_table_0;
    char *quant_table_1;
} rtp_jpeg_data_t;

int wv_jpeg_decode(char *buffer, size_t length, rtp_jpeg_data_t *rtp_jpeg_data);

typedef struct {
    uint8_t mbz;
    uint8_t precision;
    uint16_t length;
    char table0[64];
    char table1[64];
} rtp_quant_t;

#define RTP_QUANT_DEFAULT() { \
    .mbz = 0,                 \
    .precision = 0,           \
    .length = 128             \
}
#endif
int rtp_init(rtp_session_t **rtp_session, int dst_rtp_port, int dst_rtcp_port, char *dst_addr_string);
int rtp_teardown(rtp_session_t *rtp_session);
int rtp_send_jpeg(rtp_session_t *rtp_session, uint8_t *frame, uint32_t len);
int rtp_get_src_rtp_port(rtp_session_t *rtp_session);
int rtp_get_src_rtcp_port(rtp_session_t *rtp_session);

#endif //WV_RTP_UDP_H
