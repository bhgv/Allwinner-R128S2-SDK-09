/* This file contains some stuff to manipulate JPEG images
 * so they can be used in an RTP stream.
 */
#include <assert.h>

#include "wv_log.h"
#include "wv_rtp_udp.h"
#if 0
#define JPEG_SOI 0xD8
#define JPEG_EOI 0xD9
#define JPEG_SOS 0xDA
#define JPEG_DQT 0xDB

static int find_jpeg_marker(char *buffer, size_t len, uint8_t marker, char **marker_start);
static int block_length(const char *blockptr, size_t len);

int wv_jpeg_decode(char *buffer, size_t length, rtp_jpeg_data_t *rtp_jpeg_data)
{
    assert(rtp_jpeg_data != NULL);

    if (length < 4) {
        LOG_E("Invalid length: %d", length);
        return -1;
    }

    // Check magic
    if ((uint8_t)buffer[0] != 0xFF && (uint8_t)buffer[1] != JPEG_SOI) {
        LOG_E("Probably not a JPEG");
        return -1;
    }

    char *marker;
    size_t remaining;
    if (find_jpeg_marker(buffer, length, JPEG_DQT, &marker) < 0) {
        LOG_E("Failed to find marker 0x%02x", JPEG_DQT);
        return -1;
    }
    rtp_jpeg_data->quant_table_0 = marker;

    remaining = length - (marker-buffer);
    char *next = marker + block_length(marker, remaining);
    if (find_jpeg_marker(next, length - (next-buffer), JPEG_DQT, &marker) < 0) {
        LOG_E("Failed to find marker 0x%02x", JPEG_DQT);
        return -1;
    }
    rtp_jpeg_data->quant_table_1 = marker;

    remaining = length - (marker-buffer);
    next = marker + block_length(marker, remaining);
    if (find_jpeg_marker(next, length - (next-buffer), JPEG_SOS, &marker) < 0) {
        LOG_E("Failed to find marker 0x%02x", JPEG_SOS);
        return -1;
    }
    rtp_jpeg_data->jpeg_data_start = marker + block_length(marker, remaining); // Don't include the SOS header

    remaining = length - (marker-buffer);
    next = marker + block_length(marker, remaining);
    if (find_jpeg_marker(next, length - (next-buffer), JPEG_EOI, &marker) < 0) {
        //LOG_E("Failed to find marker 0x%02x", JPEG_EOI);
        return -1;
    }
    rtp_jpeg_data->jpeg_data_length = marker - rtp_jpeg_data->jpeg_data_start;

#if 0
    LOG_D("JPEG: Q1 : %p, Q2 : %p, SOS: %p, LEN: %d",
             rtsp_jpeg_data->quant_table_0 - buffer,
             rtsp_jpeg_data->quant_table_1 - buffer,
             rtsp_jpeg_data->jpeg_data_start - buffer,
             rtsp_jpeg_data->jpeg_data_length);
#endif
    return 0;
}

static int block_length(const char *blockptr, size_t len)
{
    assert(len > 4);
    assert((uint8_t)blockptr[0] == 0xff);

    return 2 + (blockptr[2] << 8 | blockptr[3]);
}

static int find_jpeg_marker(char *buffer, size_t len, uint8_t marker, char **marker_start)
{
    long position = 0;

    while (position < len - 1) {
        char *current = buffer+position;

        uint8_t framing = current[0];
        uint8_t typecode = current[1];

        if(framing != 0xff) {
            position += 1;
            continue;
        }

        if(typecode == 0x00 || typecode == 0xFF) {
            // skip
            position += 2;
            continue;
        }

        if(typecode == marker) {
            *marker_start = current;
            //LOG_E("++++++find marker 0x%x OK", marker);
            return 0;
        }

        int skip_len;
        switch(typecode) {
            case 0xd9:
                //LOG_E("-------typecode 0x%x, break...", typecode);
                break;

            case 0xe0:   // app0
            case 0xdb:   // dqt
            case 0xc4:   // dht
            case 0xc0:   // sof0
            case 0xda:   // sos
            {
                /* If we are not interested in these blocks, try to fast forward
                 * by moving the position up.
                 */
                if (len - position >= 2) {
                    // We need to be sure the length bytes are available in the buffer
                    skip_len = (current[2] << 8 | current[3]) + 2;
                } else {
                    skip_len = 2;
                }
                break;
            }
            default:
                skip_len = 2;
                //LOG_E("Unexpected jpeg typecode 0x%x\n", typecode);
                break;
        }

        position += skip_len;
    }

    //LOG_E("Failed to find jpeg marker 0x%x", marker);

    return -1;
}
#endif
