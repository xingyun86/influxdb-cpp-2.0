/* 
  influxdb-cpp-2.0 -- 💜 C++ client for InfluxDB 2.0.

  Copyright (c) 2021-2021 <http://www.ppsbbs.tech> <xingyun86>
  This library is released under the MIT License.

  Please see LICENSE file or visit https://github.com/xingyun86/influxdb-cpp-2.0 for details.
  support influxdb 2.0 protocol
  support gzip compress and uncompress
**********************************************************************************************
  influxdb-cpp -- 💜 C++ client for InfluxDB 1.0.

  Copyright (c) 2010-2018 <http://ez8.co> <orca.zhang@yahoo.com>
  This library is released under the MIT License.

  Please see LICENSE file or visit https://github.com/orca-zhang/influxdb-cpp for details.
 */

#ifndef INFLUXDB_CPP_2_0_HPP
#define INFLUXDB_CPP_2_0_HPP

#include <string>
#include <sstream>
#include <iostream>
#include <utils.h>

#ifndef _MSC_VER
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define closesocket close
#define socket_t int
#else
#define NOMINMAX
#include <winsock2.h>
#include <algorithm>
#define socket_t SOCKET
typedef struct iovec { void* iov_base; size_t iov_len; } iovec;
inline int writev(socket_t sock, struct iovec* iov, int cnt) {
    int r = send(sock, (const char*)iov->iov_base, (int)iov->iov_len, 0);
    return (r < 0 || cnt == 1) ? r : r + writev(sock, iov + 1, cnt - 1);
}
#endif

#ifdef ENABLE_GZIP
#include <gzip.h>
#endif

#define std_max(a,b)            (((a) > (b)) ? (a) : (b))
#define std_min(a,b)            (((a) < (b)) ? (a) : (b))

namespace influxdb_cpp_2 {
    struct server_info {
        std::string host_;
        int port_;
        std::string db_;
        std::string usr_;
        std::string pwd_;
        std::string precision_;
        server_info(const std::string& host, int port, const std::string& db = "", const std::string& usr = "", const std::string& pwd = "", const std::string& precision = "ms")
            : host_(host), port_(port), db_(db), usr_(usr), pwd_(pwd), precision_(precision) {}
    };
    struct server_info_v2 {
        std::string host_;
        int port_;
        std::string bucket_;
        std::string org_;
        std::string orgid_;
        std::string token_;
        std::string precision_;
        server_info_v2(const std::string& host, int port, const std::string& bucket = "", const std::string& org = "", const std::string& orgid = "", const std::string& token = "", const std::string& precision = "ms")
            : host_(host), port_(port), org_(org), orgid_(orgid), bucket_(bucket), token_(token), precision_(precision) {}
    };
    namespace detail {
        struct meas_caller;
        struct tag_caller;
        struct field_caller;
        struct ts_caller;
        struct inner {
            static int http_request(const char*, const char*, const std::string&, const std::string&, const server_info&, std::string*);
            static int http_request_v2(const char*, const char*, const std::string&, const std::string&, const std::string&, const server_info_v2&, std::string*);
            static inline unsigned char to_hex(unsigned char x) { return  x > 9 ? x + 55 : x + 48; }
            static void url_encode(std::string& out, const std::string& src);
        };
    }

    inline int query(std::string& resp, const std::string& query, const server_info& si) {
        std::string qs("&q=");
        detail::inner::url_encode(qs, query);
        return detail::inner::http_request("GET", "query", qs, "", si, &resp);
    }
    inline int query_v2(std::string& resp, const std::string& query_string, const server_info_v2& siv2) {
        return detail::inner::http_request_v2("POST", "query", Utils::format_string("?org=%s", siv2.org_.c_str()), "Content-Type: application/json; charset=utf-8\r\n", "{\"query\":\"" + query_string + "\",\"type\":\"flux\"}", siv2, &resp);
    }
    inline int create_db(std::string& resp, const std::string& db_name, const server_info& si) {
        std::string qs("&q=create+database+");
        detail::inner::url_encode(qs, db_name);
        return detail::inner::http_request("POST", "query", qs, "", si, &resp);
    }
    inline int create_db_v2(std::string& resp, const std::string& db_name, const server_info_v2& siv2) {
        return detail::inner::http_request_v2("POST", "buckets", "", "Content-Type: application/json; charset=utf-8\r\n", Utils::format_string("{\"name\":\"%s\",\"orgID\":\"%s\",\"retentionRules\":[{\"everySeconds\":0,\"type\":\"expire\"}]}", db_name.c_str(), siv2.orgid_.c_str()), siv2, &resp);
    }

    struct builder {
        detail::tag_caller& meas(const std::string& m) {
            lines_.imbue(std::locale("C"));
            lines_.clear();
            return _m(m);
        }
    protected:
        detail::tag_caller& _m(const std::string& m) {
            _escape(m, ", ");
            return (detail::tag_caller&)*this;
        }
        detail::tag_caller& _t(const std::string& k, const std::string& v) {
            lines_ << ',';
            _escape(k, ",= ");
            lines_ << '=';
            _escape(v, ",= ");
            return (detail::tag_caller&)*this;
        }
        detail::field_caller& _f_s(char delim, const std::string& k, const std::string& v) {
            lines_ << delim;
            _escape(k, ",= ");
            lines_ << "=\"";
            _escape(v, "\"");
            lines_ << '\"';
            return (detail::field_caller&)*this;
        }
        detail::field_caller& _f_i(char delim, const std::string& k, long long v) {
            lines_ << delim;
            _escape(k, ",= ");
            lines_ << '=';
            lines_ << v << 'i';
            return (detail::field_caller&)*this;
        }
        detail::field_caller& _f_f(char delim, const std::string& k, double v, int prec) {
            lines_ << delim;
            _escape(k, ",= ");
            lines_.precision(prec);
            lines_.setf(std::ios::fixed);
            lines_ << '=' << v;
            return (detail::field_caller&)*this;
        }
        detail::field_caller& _f_b(char delim, const std::string& k, bool v) {
            lines_ << delim;
            _escape(k, ",= ");
            lines_ << '=' << (v ? 't' : 'f');
            return (detail::field_caller&)*this;
        }
        detail::ts_caller& _ts(long long ts) {
            lines_ << ' ' << ts;
            return (detail::ts_caller&)*this;
        }
        int _post_http(const server_info& si, std::string* resp) {
            return detail::inner::http_request("POST", "write", "", lines_.str(), si, resp);
        }
        int _post_http_v2(const server_info_v2& siv2, std::string* resp) {
            std::string query = Utils::format_string("?org=%s&bucket=%s&precision=%s", siv2.org_.c_str(), siv2.bucket_.c_str(), siv2.precision_.c_str());
#ifdef ENABLE_GZIP
            uLongf tlen = (uLongf)lines_.str().size();
            uLongf blen = compressBound(tlen);
            std::string cbody(blen, '\0');
            if (gzcompress((Bytef*)cbody.data(), &blen, (const Bytef*)lines_.str().data(), tlen) != Z_OK)
            {
                return detail::inner::http_request_v2("POST", "write", query, "Content-Type: text/plain; charset=utf-8\r\n", lines_.str(), siv2, resp);
            }
            else
            {
                return detail::inner::http_request_v2("POST", "write", query, "Content-Type: text/plain; charset=utf-8\r\nContent-Encoding: gzip\r\n", cbody.substr(0, blen), siv2, resp);
            }
#else
            return detail::inner::http_request_v2("POST", "write", query, "Content-Type: text/plain; charset=utf-8\r\n", lines_.str(), siv2, resp);
#endif
        }
        int _send_udp(const std::string& host, int port) {
            int ret = 0;
            socket_t sock;
            struct sockaddr_in addr;

            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            if ((addr.sin_addr.s_addr = inet_addr(host.c_str())) == INADDR_NONE) return -1;

            if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) return -2;

            lines_ << '\n';
            if (sendto(sock, &lines_.str()[0], (int)lines_.str().length(), 0, (struct sockaddr*)&addr, sizeof(addr)) < (int)lines_.str().length())
                ret = -3;

            closesocket(sock);
            return ret;
        }
        void _escape(const std::string& src, const char* escape_seq) {
            size_t pos = 0, start = 0;
            while ((pos = src.find_first_of(escape_seq, start)) != std::string::npos) {
                lines_.write(src.c_str() + start, pos - start);
                lines_ << '\\' << src[pos];
                start = ++pos;
            }
            lines_.write(src.c_str() + start, src.length() - start);
        }

        std::stringstream lines_;
    };

    namespace detail {
        struct tag_caller : public builder {
            detail::tag_caller& tag(const std::string& k, const std::string& v) { return _t(k, v); }
            detail::field_caller& field(const std::string& k, const std::string& v) { return _f_s(' ', k, v); }
            detail::field_caller& field(const std::string& k, bool v) { return _f_b(' ', k, v); }
            detail::field_caller& field(const std::string& k, short v) { return _f_i(' ', k, v); }
            detail::field_caller& field(const std::string& k, int v) { return _f_i(' ', k, v); }
            detail::field_caller& field(const std::string& k, long v) { return _f_i(' ', k, v); }
            detail::field_caller& field(const std::string& k, long long v) { return _f_i(' ', k, v); }
            detail::field_caller& field(const std::string& k, double v, int prec = 2) { return _f_f(' ', k, v, prec); }
        private:
            detail::tag_caller& meas(const std::string& m);
        };
        struct ts_caller : public builder {
            detail::tag_caller& meas(const std::string& m) { lines_ << '\n'; return _m(m); }
            int post_http(const server_info& si, std::string* resp = NULL) { return _post_http(si, resp); }
            int post_http_v2(const server_info_v2& siv2, std::string* resp = NULL) { return _post_http_v2(siv2, resp); }
            int send_udp(const std::string& host, int port) { return _send_udp(host, port); }
        };
        struct field_caller : public ts_caller {
            detail::field_caller& field(const std::string& k, const std::string& v) { return _f_s(',', k, v); }
            detail::field_caller& field(const std::string& k, bool v) { return _f_b(',', k, v); }
            detail::field_caller& field(const std::string& k, short v) { return _f_i(',', k, v); }
            detail::field_caller& field(const std::string& k, int v) { return _f_i(',', k, v); }
            detail::field_caller& field(const std::string& k, long v) { return _f_i(',', k, v); }
            detail::field_caller& field(const std::string& k, long long v) { return _f_i(',', k, v); }
            detail::field_caller& field(const std::string& k, double v, int prec = 2) { return _f_f(',', k, v, prec); }
            detail::ts_caller& timestamp(unsigned long long ts) { return _ts(ts); }
        };
        inline void inner::url_encode(std::string& out, const std::string& src) {
            size_t pos = 0, start = 0;
            while ((pos = src.find_first_not_of("abcdefghijklmnopqrstuvwxyqABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.~", start)) != std::string::npos) {
                out.append(src.c_str() + start, pos - start);
                if (src[pos] == ' ')
                    out += "+";
                else {
                    out += '%';
                    out += to_hex((unsigned char)src[pos] >> 4);
                    out += to_hex((unsigned char)src[pos] & 0xF);
                }
                start = ++pos;
            }
            out.append(src.c_str() + start, src.length() - start);
        }
        inline int inner::http_request(const char* method, const char* cmd,
            const std::string& query, const std::string& body, const server_info& si, std::string* resp) {
            std::string header;
            struct iovec iv[2];
            struct sockaddr_in addr;
            socket_t sock;
            int ret_code = 0, content_length = 0, len = 0;
            char ch;
            unsigned char chunked = 0;

            addr.sin_family = AF_INET;
            addr.sin_port = htons(si.port_);
            if ((addr.sin_addr.s_addr = inet_addr(si.host_.c_str())) == INADDR_NONE) return -1;

            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -2;

            if (connect(sock, (struct sockaddr*)(&addr), sizeof(addr)) < 0) {
                closesocket(sock);
                return -3;
            }

            header.resize(len = 0x100);

            for (;;) {
                iv[0].iov_len = snprintf(&header[0], len,
                    "%s /%s?db=%s&u=%s&p=%s&epoch=%s%s HTTP/1.1\r\nHost: %s\r\nContent-Length: %d\r\n\r\n",
                    method, cmd, si.db_.c_str(), si.usr_.c_str(), si.pwd_.c_str(), si.precision_.c_str(),
                    query.c_str(), si.host_.c_str(), (int)body.length());
                if ((int)iv[0].iov_len >= len)
                    header.resize(len *= 2);
                else
                    break;
            }
            iv[0].iov_base = &header[0];
            iv[1].iov_base = (void*)&body[0];
            iv[1].iov_len = body.length();

            if (writev(sock, iv, 2) < (int)(iv[0].iov_len + iv[1].iov_len)) {
                ret_code = -6;
                goto END;
            }

            iv[0].iov_len = len;

#define _NO_MORE() (len >= (int)iv[0].iov_len && \
    (iv[0].iov_len = recv(sock, &header[0], (int)header.length(), len = 0)) == size_t(-1))
#define _GET_NEXT_CHAR() (ch = _NO_MORE() ? 0 : header[len++])
#define _LOOP_NEXT(statement) for(;;) { if(!(_GET_NEXT_CHAR())) { ret_code = -7; goto END; } statement }
#define _UNTIL(c) _LOOP_NEXT( if(ch == c) break; )
#define _GET_NUMBER(n) _LOOP_NEXT( if(ch >= '0' && ch <= '9') n = n * 10 + (ch - '0'); else break; )
#define _GET_CHUNKED_LEN(n, c) _LOOP_NEXT( if(ch >= '0' && ch <= '9') n = n * 16 + (ch - '0'); \
            else if(ch >= 'A' && ch <= 'F') n = n * 16 + (ch - 'A') + 10; \
            else if(ch >= 'a' && ch <= 'f') n = n * 16 + (ch - 'a') + 10; else {if(ch != c) { ret_code = -8; goto END; } break;} )
#define _(c) if((_GET_NEXT_CHAR()) != c) break;
#define __(c) if((_GET_NEXT_CHAR()) != c) { ret_code = -9; goto END; }

            if (resp) resp->clear();

            _UNTIL(' ')_GET_NUMBER(ret_code)
                for (;;) {
                    _UNTIL('\n')
                        switch (_GET_NEXT_CHAR()) {
                        case 'C':_('o')_('n')_('t')_('e')_('n')_('t')_('-')
                            _('L')_('e')_('n')_('g')_('t')_('h')_(':')_(' ')
                            _GET_NUMBER(content_length)
                            break;
                        case 'T':_('r')_('a')_('n')_('s')_('f')_('e')_('r')_('-')
                            _('E')_('n')_('c')_('o')_('d')_('i')_('n')_('g')_(':')
                            _(' ')_('c')_('h')_('u')_('n')_('k')_('e')_('d')
                            chunked = 1;
                            break;
                        case '\r':__('\n')
                            switch (chunked) {
                                do {
                                    __('\r')__('\n')
                            case 1:
                                _GET_CHUNKED_LEN(content_length, '\r')__('\n')
                                    if (!content_length) {
                                        __('\r')__('\n')
                                            goto END;
                                    }
                            case 0:
                                while (content_length > 0 && !_NO_MORE()) {
                                    content_length -= (int)(iv[1].iov_len = std_min(content_length, (int)iv[0].iov_len - len));
                                    if (resp) resp->append(&header[len], iv[1].iov_len);
                                    len += (int)iv[1].iov_len;
                                }
                                } while (chunked);
                            }
                                 goto END;
                        }
                    if (!ch) {
                        ret_code = -10;
                        goto END;
                    }
                }
            ret_code = -11;
        END:
            closesocket(sock);
            return ret_code / 100 == 2 ? 0 : ret_code;
#undef _NO_MORE
#undef _GET_NEXT_CHAR
#undef _LOOP_NEXT
#undef _UNTIL
#undef _GET_NUMBER
#undef _GET_CHUNKED_LEN
#undef _
#undef __
        }
        inline int inner::http_request_v2(const char* method, const char* cmd,
            const std::string& query, const std::string& headers, const std::string& body, const server_info_v2& siv2, std::string* resp) {
            std::string header;
            struct iovec iv[2];
            struct sockaddr_in addr;
            socket_t sock;
            int ret_code = 0, content_length = 0, len = 0;
            char ch;
            unsigned char chunked = 0;

            addr.sin_family = AF_INET;
            addr.sin_port = htons(siv2.port_);
            if ((addr.sin_addr.s_addr = inet_addr(siv2.host_.c_str())) == INADDR_NONE) return -1;

            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -2;

            if (connect(sock, (struct sockaddr*)(&addr), sizeof(addr)) < 0) {
                closesocket(sock);
                return -3;
            }

            header.resize(len = 0x100);

            for (;;) {
                iv[0].iov_len = snprintf(&header[0], len,
                    "%s /api/v2/%s%s HTTP/1.1\r\nHost: %s:%d\r\n%sAuthorization: Token %s\r\nContent-Length: %d\r\n\r\n",
                    method, cmd, query.c_str(), siv2.host_.c_str(), siv2.port_, headers.c_str(), siv2.token_.c_str(), (int)body.size());
                if ((int)iv[0].iov_len >= len)
                {
                    header.resize(len = (len * 2));
                }
                else
                {
                    break;
                }
            }
            iv[0].iov_base = &header[0];
            iv[1].iov_base = (void*)&body[0];
            iv[1].iov_len = body.size();

            if (writev(sock, iv, 2) < (int)(iv[0].iov_len + iv[1].iov_len)) {
                ret_code = -6;
                goto END;
            }

            iv[0].iov_len = len;

#define _NO_MORE() (len >= (int)iv[0].iov_len && \
    (iv[0].iov_len = recv(sock, &header[0], (int)header.length(), len = 0)) == size_t(-1))
#define _GET_NEXT_CHAR() (ch = _NO_MORE() ? 0 : header[len++])
#define _LOOP_NEXT(statement) for(;;) { if(!(_GET_NEXT_CHAR())) { ret_code = -7; goto END; } statement }
#define _UNTIL(c) _LOOP_NEXT( if(ch == c) break; )
#define _GET_NUMBER(n) _LOOP_NEXT( if(ch >= '0' && ch <= '9') n = n * 10 + (ch - '0'); else break; )
#define _GET_CHUNKED_LEN(n, c) _LOOP_NEXT( if(ch >= '0' && ch <= '9') n = n * 16 + (ch - '0'); \
            else if(ch >= 'A' && ch <= 'F') n = n * 16 + (ch - 'A') + 10; \
            else if(ch >= 'a' && ch <= 'f') n = n * 16 + (ch - 'a') + 10; else {if(ch != c) { ret_code = -8; goto END; } break;} )
#define _(c) if((_GET_NEXT_CHAR()) != c) break;
#define __(c) if((_GET_NEXT_CHAR()) != c) { ret_code = -9; goto END; }

            if (resp) resp->clear();

            _UNTIL(' ')_GET_NUMBER(ret_code)
                for (;;) {
                    _UNTIL('\n')
                        switch (_GET_NEXT_CHAR()) {
                        case 'C':_('o')_('n')_('t')_('e')_('n')_('t')_('-')
                            _('L')_('e')_('n')_('g')_('t')_('h')_(':')_(' ')
                            _GET_NUMBER(content_length)
                            break;
                        case 'T':_('r')_('a')_('n')_('s')_('f')_('e')_('r')_('-')
                            _('E')_('n')_('c')_('o')_('d')_('i')_('n')_('g')_(':')
                            _(' ')_('c')_('h')_('u')_('n')_('k')_('e')_('d')
                            chunked = 1;
                            break;
                        case '\r':__('\n')
                            switch (chunked) {
                                do {
                                    __('\r')__('\n')
                            case 1:
                                _GET_CHUNKED_LEN(content_length, '\r')__('\n')
                                    if (!content_length) {
                                        __('\r')__('\n')
                                            goto END;
                                    }
                            case 0:
                                while (content_length > 0 && !_NO_MORE()) {
                                    content_length -= (int)(iv[1].iov_len = std_min(content_length, (int)iv[0].iov_len - len));
                                    if (resp) resp->append(&header[len], iv[1].iov_len);
                                    len += (int)iv[1].iov_len;
                                }
                                } while (chunked);
                            }
                                 goto END;
                        }
                    if (!ch) {
                        ret_code = -10;
                        goto END;
                    }
                }
            ret_code = -11;
        END:
            closesocket(sock);
            return ret_code / 100 == 2 ? 0 : ret_code;
#undef _NO_MORE
#undef _GET_NEXT_CHAR
#undef _LOOP_NEXT
#undef _UNTIL
#undef _GET_NUMBER
#undef _GET_CHUNKED_LEN
#undef _
#undef __
        }
    }
}

#endif // INFLUXDB_CPP_2_0_HPP
