#include <string.h>
#include <limits.h>
#include <stdlib.h>

#include "wv_log.h"
#include "wv_rtsp.h"

inline int min(int a, int b) { return (a < b) ? a : b; }

static bool valid_header_name_char(char c)
{
    if (c > 127) return false;
    if (c <=31 || c == 127 ) return false; // CTLs
    if (c == '(' || c == ')' || c == '<' || c == '>' || c == '@' ||
        c == ',' || c == ';' || c == ':' || c == '\\' || c == '"' ||
        c == '/' || c == '[' || c == ']' || c == '?' || c == '=' ||
        c == '{' || c == '}' || c == ' ' || c == '\t' )
        return false; // separators

    return true;
}

static int safe_atoi(char *value)
{
    char *end;
    long lv = strtol(value, &end, 10);

    if (*end != '\0' || lv < INT_MIN || lv > INT_MAX) {
        LOG_E("Invalid numerical value: %s", value);
        return -1;
    }

    return (int)lv;
}

int rtsp_parser_init(rtsp_parser_t **parser_handle)
{
    rtsp_parser_t *parser = calloc(1, sizeof(rtsp_parser_t));
    if (!parser) {
        return PARSER_NOMEM;
    }

    memset(parser, 0 , sizeof(rtsp_parser_t));

    rtsp_req_t *request = calloc(1, sizeof(rtsp_req_t));
    if (!request) {
        free(parser);
        return PARSER_NOMEM;
    }

    memset(request, 0 , sizeof(rtsp_req_t));

    parser->request = request;
    *parser_handle = parser;

    return PARSER_OK;
}

int rtsp_parse_request(rtsp_parser_t *parser_handle, const char *buffer, const size_t len)
{
    int position = 0;
    int header_marker = 0;
    int header_value_marker = 0;

    if (!parser_handle) {
        LOG_D("Error; Invalid handle");
        return PARSER_INVALID_ARGS;
    }

    rtsp_req_t *request = parser_handle->request;

    if (parser_handle->parse_complete) {
        LOG_D("Error; Can't add data to completed request");
        return PARSER_INVALID_STATE;
    }

    for (int i = 0; i < len; i++) {
        char current = buffer[i];
        parser_handle->intermediate[parser_handle->intermediate_len + 1] = 0x0; // workaround
//        LOG_D("Parsing '%c' at %d in state %d", current, position, parser_handle->state);
//        LOG_D("Intermediate: %s (%d bytes)", parser_handle->intermediate, parser_handle->intermediate_len);

        if (parser_handle->intermediate_len >= 1023) {
            LOG_D("Parse failed, no space in intermediate buffer");
            return PARSER_NOMEM;
        }

        // Handle CR/LF
        if (current == '\r' && i < len - 1) {
            current = buffer[i++];
            position++;
        }

        switch(parser_handle->state) {
            case RTSP_PARSER_PARSE_METHOD:
                if (current == ' ') {
                    if (strncmp(parser_handle->intermediate, "OPTIONS", min(parser_handle->intermediate_len, 7)) == 0) {
                        request->request_type = OPTIONS;
                    } else if (strncmp(parser_handle->intermediate, "SETUP", min(parser_handle->intermediate_len, 5)) == 0) {
                        request->request_type = SETUP;
                    } else if (strncmp(parser_handle->intermediate, "DESCRIBE", min(parser_handle->intermediate_len, 8)) == 0) {
                        request->request_type = DESCRIBE;
                    } else if (strncmp(parser_handle->intermediate, "PLAY", min(parser_handle->intermediate_len, 4)) == 0) {
                        request->request_type = PLAY;
                    } else if (strncmp(parser_handle->intermediate, "TEARDOWN", min(parser_handle->intermediate_len, 8)) == 0) {
                        request->request_type = TEARDOWN;
                    } else {
                        request->request_type = UNSUPPORTED;
                    }

                    parser_handle->state = RTSP_PARSER_PARSE_URL;
                    parser_handle->intermediate_len = 0;
                    position++;

                    LOG_D("Method: %d", request->request_type);
                    continue;
                } else if ((current < 'A' || current > 'Z') && current != '_') {
                    LOG_D("Invalid character in method: %c", current);
                    parser_handle->error = 400;
                    return i;
                } else {
                    parser_handle->intermediate[parser_handle->intermediate_len] = current;
                    position++;
                    parser_handle->intermediate_len++;
                }
                break;
            case RTSP_PARSER_PARSE_URL:
                if (current == ' ') {
                    strncpy(request->url, parser_handle->intermediate, min(parser_handle->intermediate_len, URL_MAX_LENGTH));
                    request->url[min(parser_handle->intermediate_len, URL_MAX_LENGTH)] = 0x0;
                    LOG_D("Parsed url: %s", request->url);

                    parser_handle->state = RTSP_PARSER_PARSE_PROTOCOL;
                    parser_handle->intermediate_len = 0;
                    position++;

                    continue;
                } else if (current == '\r' || current == '\n') {
                    LOG_D("Invalid character in url: %c", current);
                    parser_handle->error = 400;
                    return i;
                } else {
                    parser_handle->intermediate[parser_handle->intermediate_len] = current;
                    position++;
                    parser_handle->intermediate_len++;
                }
                break;
            case RTSP_PARSER_PARSE_PROTOCOL:
                if (current == '\r' || current == '\n') {
                    if (strncmp(parser_handle->intermediate, "RTSP/1.0", min(parser_handle->intermediate_len,9)) != 0) {
                        LOG_W("Only supporting RTSP/1.0 but got: %s", parser_handle->intermediate);
                        parser_handle->error = 400;
                    }

                    parser_handle->state = RTSP_PARSER_OPTIONAL_HEADER;
                    parser_handle->intermediate_len = 0;
                    position++;
                } else {
                    parser_handle->intermediate[parser_handle->intermediate_len] = current;
                    position++;
                    parser_handle->intermediate_len++;
                }
                break;
            case RTSP_PARSER_OPTIONAL_HEADER:
                if (current == '\r' || current == '\n') {
                    LOG_D("Setting parse_complete");
                    parser_handle->parse_complete = true;
                    return len;
                }
            case RTSP_PARSER_PARSE_HEADER:
                if (current == ':') {
                    parser_handle->intermediate[parser_handle->intermediate_len] = 0x0;
                    position++;
                    parser_handle->intermediate_len++;
                    header_marker = 0;
                    header_value_marker = parser_handle->intermediate_len;
                    parser_handle->state = RTSP_PARSER_PARSE_HEADER_WS;
                } else if (!valid_header_name_char(current)) {
                    LOG_W("Invalid characters in header name: %c", current);
                    parser_handle->error = 400;
                    return i;
                } else {
                    parser_handle->intermediate[parser_handle->intermediate_len] = current;
                    position++;
                    parser_handle->intermediate_len++;
                }
                break;
            case RTSP_PARSER_PARSE_HEADER_WS:
                if (current == ' ') {
                    parser_handle->state = RTSP_PARSER_PARSE_HEADER_VALUE;
                    continue;
                }
                LOG_W("Unexpected character in header: %c", current);
                parser_handle->error = 400;
                return i;
            case RTSP_PARSER_PARSE_HEADER_VALUE:
                if (current == '\r' || current == '\n') {
                    char *header = parser_handle->intermediate + header_marker;
                    char *value = parser_handle->intermediate + header_value_marker;

                    LOG_D("Header> %s: %s", header, value);
                    if (strcasecmp(header, "cseq") == 0) {
                        char *end;
                        long lv = strtol(value, &end, 10);
                        if (*end != '\0' || lv < INT_MIN || lv > INT_MAX) {
                            LOG_W("Invalid numerical value for header %s: %s", header, value);
                            parser_handle->error = 400;
                            return i;
                        }
                        request->cseq = (int)lv;
                    } else if (strcasecmp(header, "transport") == 0) {
                        char *saveptr;

                        char *token = strtok_r(value, ";", &saveptr);
                        if (token == NULL || strcmp(token, "RTP/AVP") != 0) {
                            LOG_W( "Unsupported stream transport: %s", token);
                            parser_handle->error = 461;
                            return i;
                        }

                        token = strtok_r(NULL, ";", &saveptr);
                        if (token == NULL || strcmp(token, "unicast") != 0) {
                            LOG_W("Unsupported direction transport: %s", token);
                            parser_handle->error = 400;
                            return i;
                        }

                        token = strtok_r(NULL, ";", &saveptr);
                        if (token == NULL || strncmp(token, "client_port=", min(strlen(token), 7)) != 0) {
                            LOG_E("Expected client_port, got : %s", token);
                            parser_handle->error = 400;
                            return i;
                        }

                        char *porta = strtok_r(token+12, "-", &saveptr);
                        request->dst_rtp_port = safe_atoi(porta);

                        char *portb = strtok_r(NULL, "-", &saveptr);
                        request->dst_rtcp_port = safe_atoi(portb);

                        if (request->dst_rtp_port < 0 || request->dst_rtcp_port < 0) {
                            LOG_W("Invalid client_port values: %s", token);
                            parser_handle->error = 400;
                            return i;
                        }

                    }
                    parser_handle->state = RTSP_PARSER_OPTIONAL_HEADER;
                    parser_handle->intermediate_len = 0;
                    position++;
                } else {
                    parser_handle->intermediate[parser_handle->intermediate_len] = current;
                    position++;
                    parser_handle->intermediate_len++;
                }
                break;

            default:
                continue;
        };
    }
    return len;
};

int rtsp_parser_free(rtsp_parser_t *parser_handle)
{
    parser_handle->request = NULL; // Freeing request is left to the caller
    free(parser_handle);

    return 0;
}

int rtsp_parser_get_error(rtsp_parser_t *parser_handle)
{
    return parser_handle->error;
}

int rtsp_parser_is_complete(rtsp_parser_t *parser_handle)
{
    return parser_handle->parse_complete;
}

rtsp_req_t *rtsp_parser_get_request(rtsp_parser_t *parser_handle)
{
    return parser_handle->request;
}
