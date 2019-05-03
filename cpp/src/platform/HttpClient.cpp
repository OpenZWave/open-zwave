// minihttp.cpp - All functionality required for a minimal TCP/HTTP client packed in one file.
// Released under the WTFPL (See minihttp.h)

#ifdef _MSC_VER
#  ifndef _CRT_SECURE_NO_WARNINGS
#    define _CRT_SECURE_NO_WARNINGS
#  endif
#  ifndef _CRT_SECURE_NO_DEPRECATE
#    define _CRT_SECURE_NO_DEPRECATE
#  endif
#endif

#ifdef WINRT
#  include <winsock2.h>
#  include <ws2tcpip.h>
#elif _WIN32
#  ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0501
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#if !defined(EWOULDBLOCK)
#  define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#if !defined(ETIMEDOUT)
#  define ETIMEDOUT WSAETIMEDOUT
#endif
#if !defined(ECONNRESET)
#  define ECONNRESET WSAECONNRESET
#endif
#if !defined(ENOTCONN)
#  define ENOTCONN WSAENOTCONN
#endif
#  include <io.h>
#else
#  include <sys/types.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <netdb.h>
#  define SOCKET_ERROR (-1)
#  define INVALID_SOCKET (SOCKET)(~0)
   typedef intptr_t SOCKET;
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <cctype>
#include <cerrno>
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <string>


#ifdef MINIHTTP_USE_POLARSSL
#  include "polarssl/net.h"
#  include "polarssl/ssl.h"
#  include "polarssl/entropy.h"
#  include "polarssl/ctr_drbg.h"
#endif

#include "platform/HttpClient.h"
#include "platform/Log.h"
#include "Utils.h"

#define SOCKETVALID(s) ((s) != INVALID_SOCKET)

//#define _DEBUG

#ifdef _MSC_VER
#  define STRNICMP _strnicmp
#else
#  define STRNICMP strncasecmp
#endif
#ifdef _DEBUG
#  define traceprint(...) {printf(__VA_ARGS__);}
#else
#  define traceprint(...) {}
#endif

namespace OpenZWave
{
namespace SimpleHTTPClient
{

#ifdef MINIHTTP_USE_POLARSSL
// ------------------------ SSL STUFF -------------------------
bool HasSSL
(
)
{
	return true;
}

struct SSLCtx
{
    SSLCtx
    (
    ):
    _inited(0)
    {
        entropy_init(&entropy);
        x509_crt_init(&cacert);
        memset(&ssl, 0, sizeof(ssl_context));
    }
    ~SSLCtx
    (
    )
    {
        entropy_free(&entropy);
        x509_crt_free(&cacert);
        ssl_free(&ssl);
        if(_inited & 1)
            ctr_drbg_free(&ctr_drbg);
        if(_inited & 2)
            ssl_free(&ssl);
    }
    bool init
    (
    )
    {
        const char *pers = "minihttp";
        const size_t perslen = strlen(pers);

        int err = ctr_drbg_init(&ctr_drbg, entropy_func, &entropy, (unsigned char *)pers, perslen);
        if(err)
        {
            traceprint("SSLCtx::init(): ctr_drbg_init() returned %d\n", err);
            return false;
        }
        _inited |= 1;

        err = ssl_init(&ssl);
        if(err)
        {
            traceprint("SSLCtx::init(): ssl_init() returned %d\n", err);
            return false;
        }
        _inited |= 2;

        return true;
    }
    void reset
    (
    )
    {
        ssl_session_reset(&ssl);
    }
    entropy_context entropy;
    ctr_drbg_context ctr_drbg;
    ssl_context ssl;
    x509_crt cacert;

private:
    unsigned _inited;
};


// ------------------------------------------------------------
#else // MINIHTTP_USE_POLARSSL
bool HasSSL
(
)
{
	return false;
}
#endif // MINIHTTP_USE_POLARSSL

#define DEFAULT_BUFSIZE 4096

inline int _GetError
(
)
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

inline std::string _GetErrorStr
(
int e
)
{
    std::string ret;
#ifdef WINRT
	LPTSTR s = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 512);
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, e, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), s, 512, NULL );
	char buffer[1024];
	wcstombs(buffer, s, sizeof(buffer));
	HeapFree(GetProcessHeap(), 0, s);
	ret = buffer;
#elif _WIN32
    LPTSTR s;
    ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, e, 0, (LPTSTR)&s, 0, NULL);
    if(s)
        ret = s;
    ::LocalFree(s);
#else
     const char *s = strerror(e);
     if(s)
         ret = s;
#endif
    return ret;
}

bool InitNetwork
(
)
{
#ifdef _WIN32
    WSADATA wsadata;
    if(WSAStartup(MAKEWORD(2,2), &wsadata))
    {
        traceprint("WSAStartup ERROR: %s", _GetErrorStr(_GetError()).c_str());
        return false;
    }
#endif
    return true;
}

void StopNetwork
(
)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

static bool _Resolve
(
const char *host,
unsigned int port,
struct sockaddr_in *addr
)
{
    char port_str[16];
    sprintf(port_str, "%u", port);

    struct addrinfo hnt, *res = 0;
    memset(&hnt, 0, sizeof(hnt));
    hnt.ai_family = AF_INET;
    hnt.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port_str, &hnt, &res))
    {
        traceprint("RESOLVE ERROR: %s", _GetErrorStr(_GetError()).c_str());
        return false;
    }
    if (res)
    {
        if (res->ai_family != AF_INET)
        {
            traceprint("RESOLVE WTF: %s", _GetErrorStr(_GetError()).c_str());
            freeaddrinfo(res);
            return false;
        }
        memcpy(addr, res->ai_addr, res->ai_addrlen);
        freeaddrinfo(res);
        return true;
    }
    return false;
}

// FIXME: this does currently not handle links like:
// http://example.com/index.html#pos

bool SplitURI
(
const std::string& uri,
std::string& protocol,
std::string& host,
std::string& file,
int& port,
bool& useSSL
)
{
    const char *p = uri.c_str();
    const char *sl = strstr(p, "//");
    unsigned int offs = 0;
    if(sl)
    {
        size_t colon = uri.find(':');
        size_t firstslash = uri.find('/');
        if(colon < firstslash)
            protocol = uri.substr(0, colon);
        if(strncmp(p, "http://", 7) == 0)
        {
            useSSL = false;
            offs = 7;
        }
        else if(strncmp(p, "https://", 8) == 0)
        {
            useSSL = true;
            offs = 8;
        }
        else
            return false;

        p = sl + 2;
    }

    sl = strchr(p, '/');
    if(!sl)
    {
        host = p;
        file = "/";
    }
    else
    {
        host = uri.substr(offs, sl - p);
        file = sl;
    }

    port = -1;
    size_t colon = host.find(':');
    if(colon != std::string::npos)
    {
        port = atoi(host.c_str() + colon);
        host.erase(port);
    }

    return true;
}

void URLEncode
(
const std::string& s,
std::string& enc
)
{
    const size_t len = s.length();
    char buf[3];
    buf[0] = '%';
    for(size_t i = 0; i < len; i++)
    {
        const unsigned char c = s[i];
        // from  https://www.ietf.org/rfc/rfc1738.txt, page 3
        // with some changes for compatibility
        if(isalnum(c) || c == '-' || c == '_' || c == '.' || c == ',')
            enc += (char)c;
        else if(c == ' ')
            enc += '+';
        else
        {
            unsigned nib = (c >> 4) & 0xf;
            buf[1] = nib < 10 ? '0' + nib : 'a' + (nib-10);
            nib = c & 0xf;
            buf[2] = nib < 10 ? '0' + nib : 'a' + (nib-10);
            enc.append(&buf[0], 3);
        }
    }
}

static bool _SetNonBlocking
(
SOCKET s,
bool nonblock
)
{
    if(!SOCKETVALID(s))
        return false;
#ifdef MINIHTTP_USE_POLARSSL
    if(nonblock)
        return net_set_nonblock(s) == 0;
    else
        return net_set_block(s) == 0;
#elif defined(_WIN32)
    ULONG tmp = !!nonblock;
    if(::ioctlsocket(s, FIONBIO, &tmp) == SOCKET_ERROR)
        return false;
#else
    int tmp = ::fcntl(s, F_GETFL);
    if(tmp < 0)
        return false;
    if(::fcntl(s, F_SETFL, nonblock ? (tmp|O_NONBLOCK) : (tmp|=~O_NONBLOCK)) < 0)
        return false;
#endif
    return true;
}

TcpSocket::TcpSocket
(
):
_inbuf(NULL),
_readptr(NULL),
_writeptr(NULL),
_inbufSize(0),
_writeSize(0),
_recvSize(0),
_lastport(0),
_nonblocking(false),
_s(INVALID_SOCKET),
_sslctx(NULL)
{
}

TcpSocket::~TcpSocket
(
)
{
    close();
    if(_inbuf)
        free(_inbuf);
}

bool TcpSocket::isOpen
(
)
{
    return SOCKETVALID(_s);
}

void TcpSocket::close
(
)
{
    if(!SOCKETVALID(_s))
        return;

    traceprint("TcpSocket::close\n");

    _OnCloseInternal();

#ifdef MINIHTTP_USE_POLARSSL
    if(_sslctx)
        ((SSLCtx*)_sslctx)->reset();
    net_close(_s);
    shutdownSSL();
#else
#  ifdef _WIN32
    ::closesocket((SOCKET)_s);
#  else
    ::close(_s);
#  endif
#endif

    _s = INVALID_SOCKET;
    _recvSize = 0;
}

void TcpSocket::_OnCloseInternal
(
)
{
    _OnClose();
}

bool TcpSocket::SetNonBlocking
(
bool nonblock
)
{
    _nonblocking = nonblock;
    return _SetNonBlocking(_s, nonblock);
}

void TcpSocket::SetBufsizeIn
(
unsigned int s
)
{
    if(s < 512)
        s = 512;
    if(s != _inbufSize)
        _inbuf = (char*)realloc(_inbuf, s);
    _inbufSize = s;
    _writeSize = s - 1;
    _readptr = _writeptr = _inbuf;
}

static bool _openSocket
(
SOCKET *ps,
const char *host,
unsigned port
)
{
#ifdef MINIHTTP_USE_POLARSSL
    int s;
    int err = net_connect(&s, host, port);
    if(err)
    {
        traceprint("open_ssl: net_connect(%s, %u) returned %d\n", host, port, err);
        return false;
    }
#else
    sockaddr_in addr;
    if(!_Resolve(host, port, &addr))
    {
        traceprint("RESOLV ERROR: %s\n", _GetErrorStr(_GetError()).c_str());
        return false;
    }

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);

    if(!SOCKETVALID(s))
    {
        traceprint("SOCKET ERROR: %s\n", _GetErrorStr(_GetError()).c_str());
        return false;
    }

    if (::connect(s, (sockaddr*)&addr, sizeof(sockaddr)))
    {
        traceprint("CONNECT ERROR: %s\n", _GetErrorStr(_GetError()).c_str());
        return false;
    }
#endif

    *ps = s;
    return true;
}

#ifdef MINIHTTP_USE_POLARSSL
void traceprint_ssl
(
void *ctx,
int level,
const char *str
)
{
    (void)ctx;
    printf("ssl: [%d] %s\n", level, str);
}
static bool _openSSL
(
void *ps,
SSLCtx *ctx
)
{
    ssl_set_endpoint(&ctx->ssl, SSL_IS_CLIENT);
    ssl_set_authmode(&ctx->ssl, SSL_VERIFY_OPTIONAL);
    ssl_set_ca_chain(&ctx->ssl, &ctx->cacert, NULL, NULL);

    /* SSLv3 is deprecated, set minimum to TLS 1.0 */
    ssl_set_min_version(&ctx->ssl, SSL_MAJOR_VERSION_3, SSL_MINOR_VERSION_1);

    // The following is removed from newer polarssl versions
#ifdef SSL_ARC4_DISABLED
    /* RC4 is deprecated, disable it */
    ssl_set_arc4_support(&ctx->ssl, SSL_ARC4_DISABLED );
#endif

    ssl_set_rng(&ctx->ssl, ctr_drbg_random, &ctx->ctr_drbg);
    ssl_set_dbg(&ctx->ssl, traceprint_ssl, NULL);
    //ssl_set_ciphersuites( &ctx->ssl, ssl_default_ciphersuites); // FIXME
    ssl_set_bio(&ctx->ssl, net_recv, ps, net_send, ps);

    traceprint("SSL handshake now...\n");
    int err;
    while( (err = ssl_handshake(&ctx->ssl)) )
    {
        if(err != POLARSSL_ERR_NET_WANT_READ && err != POLARSSL_ERR_NET_WANT_WRITE)
        {
            traceprint("open_ssl: ssl_handshake returned -0x%x\n\n", -err);
            return false;
        }
    }
    traceprint("SSL handshake done\n");
    return true;
}
#endif

bool TcpSocket::open
(
const char *host,
unsigned int port
)
{
    if(isOpen())
    {
        if( (host && host != _host) || (port && port != _lastport) )
            close();
            // ... and continue connecting to new host/port
        else
            return true; // still connected, to same host and port.
    }

    if(host)
        _host = host;
    else
        host = _host.c_str();

    if(port)
        _lastport = port;
    else
    {
        port = _lastport;
        if(!port)
            return false;
    }

    traceprint("TcpSocket::open(): host = [%s], port = %d\n", host, port);

    assert(!SOCKETVALID(_s));

    _recvSize = 0;

    {
        SOCKET s;
        if(!_openSocket(&s, host, port))
            return false;
        _s = s;

#ifdef SO_NOSIGPIPE
        // Don't fire SIGPIPE when trying to write to a closed socket
        {
            int set = 1;
            setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
        }
#endif

    }

    _SetNonBlocking(_s, _nonblocking); // restore setting if it was set in invalid state. static call because _s is intentionally still invalid here.

#ifdef MINIHTTP_USE_POLARSSL
    if(_sslctx)
    {
        traceprint("TcpSocket::open(): SSL requested...\n");
        if(!_openSSL(&_s, (SSLCtx*)_sslctx))
        {
            close();
            return false;
        }
    }
#endif

    _OnOpen();

    return true;
}

#ifdef MINIHTTP_USE_POLARSSL
void TcpSocket::shutdownSSL
(
)
{
    delete ((SSLCtx*)_sslctx);
    _sslctx = NULL;
}

bool TcpSocket::initSSL
(
const char *certs
)
{
    SSLCtx *ctx = (SSLCtx*)_sslctx;
    if(ctx)
        ctx->reset();
    else
    {
        ctx = new SSLCtx();
        _sslctx = ctx;
        if(!ctx->init())
        {
            shutdownSSL();
            return false;
        }
    }

    if(certs)
    {
        int err = x509_crt_parse(&ctx->cacert, (const unsigned char*)certs, strlen(certs));
        if(err)
        {
            shutdownSSL();
            traceprint("x509_crt_parse() returned %d\n", err);
            return false;
        }
    }

    return true;
}

SSLResult TcpSocket::verifySSL
(
)
{
    if(!_sslctx)
        return SSLR_NO_SSL;

    SSLCtx *ctx = (SSLCtx*)_sslctx;
    unsigned r = SSLR_OK;
    int res = ssl_get_verify_result(&ctx->ssl);
    if(res)
    {
        if(res & BADCERT_EXPIRED)
            r |= SSLR_CERT_EXPIRED;

        if(res & BADCERT_REVOKED)
            r |= SSLR_CERT_REVOKED;

        if(res & BADCERT_CN_MISMATCH)
            r |= SSLR_CERT_CN_MISMATCH;

        if(res & BADCERT_NOT_TRUSTED)
            r |= SSLR_CERT_NOT_TRUSTED;

        if(res & BADCERT_MISSING)
            r |= SSLR_CERT_MISSING;

        if(res & BADCERT_SKIP_VERIFY)
            r |= SSLR_CERT_SKIP_VERIFY;

        if(res & BADCERT_FUTURE)
            r |= SSLR_CERT_FUTURE;

        // More than just this?
        if(res & (BADCERT_SKIP_VERIFY | SSLR_CERT_NOT_TRUSTED))
            r |= SSLR_FAIL;
    }

    return (SSLResult)r;
}
#else // MINIHTTP_USE_POLARSSL
void TcpSocket::shutdownSSL
(
)
{
}

bool TcpSocket::initSSL
(
const char *certs
)
{
    traceprint("initSSL: Compiled without SSL support!");
    return false;
}

SSLResult TcpSocket::verifySSL
(
)
{
	return SSLR_NO_SSL;
}
#endif

bool TcpSocket::SendBytes
(
const void *str,
unsigned int len
)
{
    if(!len)
        return true;
    if(!SOCKETVALID(_s))
        return false;
    //traceprint("SEND: '%s'\n", str);

    unsigned written = 0;
    while(true) // FIXME: buffer bytes to an internal queue instead?
    {
        int ret = _writeBytes((const unsigned char*)str + written, len - written);
        if(ret > 0)
        {
            assert((unsigned)ret <= len);
            written += (unsigned)ret;
            if(written >= len)
                break;
        }
        else if(ret < 0)
        {
#ifdef _DEBUG
        	int err = ret == -1 ? _GetError() : ret;
            traceprint("SendBytes: error %d: %s\n", err, _GetErrorStr(err).c_str());
#endif
            close();
            return false;
        }
        // and if ret == 0, keep trying.
    }

    assert(written == len);
    return true;
}

int TcpSocket::_writeBytes
(
const unsigned char *buf,
size_t len
)
{
    int ret = 0;

#ifdef MINIHTTP_USE_POLARSSL
    int err;
    if(_sslctx)
        err = ssl_write(&((SSLCtx*)_sslctx)->ssl, buf, len);
    else
        err = net_send(&_s, buf, len);

    switch(err)
    {
        case POLARSSL_ERR_NET_WANT_WRITE:
            ret = 0; // FIXME: Nothing written, try later?
        default:
            ret = err;
    }
#else
    int flags = 0;
    #ifdef MSG_NOSIGNAL
       flags |= MSG_NOSIGNAL;
    #endif
    return ::send(_s, (const char*)buf, len, flags);
#endif

    return ret;
}

void TcpSocket::_ShiftBuffer
(
)
{
    size_t by = _readptr - _inbuf;
    memmove(_inbuf, _readptr, by);
    _readptr = _inbuf;
    _writeptr = _inbuf + by;
    _writeSize = _inbufSize - by - 1;
}

void TcpSocket::_OnData
(
)
{
    _OnRecv(_readptr, _recvSize);
}

int TcpSocket::_readBytes
(
unsigned char *buf,
size_t maxlen
)
{
#ifdef MINIHTTP_USE_POLARSSL
    if(_sslctx)
        return ssl_read(&((SSLCtx*)_sslctx)->ssl, buf, maxlen);
    else
        return net_recv(&_s, buf, maxlen);
#else
    return recv(_s, (char*)buf, maxlen, 0); // last char is used as string terminator
#endif
}

bool TcpSocket::update
(
)
{
   if(!_OnUpdate())
       return false;

   if(!isOpen())
       return false;

    if(!_inbuf)
        SetBufsizeIn(DEFAULT_BUFSIZE);

    int bytes = _readBytes((unsigned char*)_writeptr, _writeSize);
    //traceprint("TcpSocket::update: _readBytes() result %d\n", bytes);
    if(bytes > 0) // we received something
    {
        _inbuf[bytes] = 0;
        _recvSize = bytes;

        // reset pointers for next read
        _writeSize = _inbufSize - 1;
        _readptr = _writeptr = _inbuf;

        _OnData();
    }
    else if(bytes == 0) // remote has closed the connection
    {
        close();
    }
    else // whoops, error?
    {
        // Possible that the error is returned directly (in that case, < -1, or -1 is returned and the error has to be retrieved seperately.
        // But in the latter case, error numbers may be positive (at least on windows...)
        int err = bytes == -1 ? _GetError() : bytes;
        switch(err)
        {
        case EWOULDBLOCK:
#if defined(EAGAIN) && (EWOULDBLOCK != EAGAIN)
        case EAGAIN: // linux man pages say this can also happen instead of EWOULDBLOCK
#endif
            return false;

#ifdef MINIHTTP_USE_POLARSSL
        case POLARSSL_ERR_NET_WANT_READ:
            break; // Try again later
#endif

        default:
            traceprint("SOCKET UPDATE ERROR: (%d): %s\n", err, _GetErrorStr(err).c_str());
        case ECONNRESET:
        case ENOTCONN:
        case ETIMEDOUT:
#ifdef _WIN32
        case WSAECONNABORTED:
        case WSAESHUTDOWN:
#endif
            close();
            break;
        }
    }
    return true;
}


// ==========================
// ===== HTTP SPECIFIC ======
// ==========================
#ifdef MINIHTTP_SUPPORT_HTTP


POST& POST::add
(
const char *key,
const char *value
)
{
    if(!empty())
        data += '&';
    URLEncode(key, data);
    data += '=';
    URLEncode(value, data);
    return *this;
}


HttpSocket::HttpSocket
(
):
TcpSocket(),
_user_agent("OpenZWave"),
_accept_encoding(),
_tmpHdr(),
_keep_alive(0),
_remaining(0),
_contentLen(0),
_status(0),
_inProgress(false),
_chunkedTransfer(false),
_mustClose(true),
_followRedir(true),
_alwaysHandle(false),
_filename(),
_pFile(NULL)
{
}

HttpSocket::~HttpSocket
(
)
{
}

void HttpSocket::_OnOpen
(
)
{
    TcpSocket::_OnOpen();
    _chunkedTransfer = false;
    _mustClose = true;
}

void HttpSocket::_OnCloseInternal
(
)
{
    if(!IsRedirecting() || _alwaysHandle)
        _OnClose();
}

bool HttpSocket::_OnUpdate
(
)
{
    if(!TcpSocket::_OnUpdate())
        return false;

    if(_inProgress && !_chunkedTransfer && !_remaining && _status)
        _FinishRequest();

    //traceprint("HttpSocket::_OnUpdate, Q = %d\n", (unsigned)_requestQ.size());

    // initiate transfer if queue is not empty, but the socket somehow forgot to proceed
    if(_requestQ.size() && !_remaining && !_chunkedTransfer && !_inProgress)
        _DequeueMore();

    return true;
}

bool HttpSocket::Download
(
const std::string& url,
const char *extraRequest,
void *user,
const POST *post
)
{
	if (_filename.length() == 0) {
		traceprint("No Filename Set\n");
		return false;
	}
    Request req;
    req.user = user;
    if(post)
        req.post = *post;
    SplitURI(url, req.protocol, req.host, req.resource, req.port, req.useSSL);
    if(IsRedirecting() && req.host.empty()) // if we're following a redirection to the same host, the server is likely to omit its hostname
        req.host = _curRequest.host;
    if(req.port < 0)
        req.port = req.useSSL ? 443 : 80;
    if(extraRequest)
        req.extraGetHeaders = extraRequest;
    return SendRequest(req, false);
}


bool HttpSocket::_Redirect
(
std::string loc,
bool forceGET
)
{
    traceprint("Following HTTP redirect to: %s\n", loc.c_str());
    if(loc.empty())
        return false;

    Request req;
    req.user = _curRequest.user;
    req.useSSL = _curRequest.useSSL;
    if(!forceGET)
        req.post = _curRequest.post;
    SplitURI(loc, req.protocol, req.host, req.resource, req.port, req.useSSL);
    if(req.protocol.empty()) // assume local resource
    {
        req.host = _curRequest.host;
        req.resource = loc;
    }
    if(req.host.empty())
        req.host = _curRequest.host;
    if(req.port < 0)
        req.port = _curRequest.port;
    req.extraGetHeaders = _curRequest.extraGetHeaders;
    return SendRequest(req, false);
}

bool HttpSocket::SendRequest
(
const std::string what,
const char *extraRequest,
void *user
)
{
    Request req(what, _host, _lastport, user);
    if(extraRequest)
        req.extraGetHeaders = extraRequest;
    return SendRequest(req, false);
}

bool HttpSocket::QueueRequest
(
const std::string what,
const char *extraRequest,
void *user
)
{
    Request req(what, _host, _lastport, user);
    if(extraRequest)
        req.extraGetHeaders = extraRequest;
    return SendRequest(req, true);
}

bool HttpSocket::SendRequest
(
Request& req,
bool enqueue
)
{
    if(req.host.empty() || !req.port)
        return false;

    const bool post = !req.post.empty();

    std::stringstream r;
    const char *crlf = "\r\n";
    r << (post ? "POST " : "GET ") << req.resource << " HTTP/1.1" << crlf;
    r << "Host: " << req.host << crlf;
    if(_keep_alive)
    {
        r << "Connection: Keep-Alive" << crlf;
        r << "Keep-Alive: " << _keep_alive << crlf;
    }
    else
        r << "Connection: close" << crlf;

    if(_user_agent.length())
        r << "User-Agent: " << _user_agent << crlf;

    if(_accept_encoding.length())
        r << "Accept-Encoding: " << _accept_encoding << crlf;

    if(post)
    {
        r << "Content-Length: " << req.post.length() << crlf;
        r << "Content-Type: application/x-www-form-urlencoded" << crlf;
    }

    if(req.extraGetHeaders.length())
    {
        r << req.extraGetHeaders;
        if(req.extraGetHeaders.compare(req.extraGetHeaders.length() - 2, std::string::npos, crlf))
            r << crlf;
    }

    r << crlf; // header terminator

    // FIXME: appending this to the 'header' field is probably not a good idea
    if(post)
        r << req.post.str();

    req.header = r.str();

    return _EnqueueOrSend(req, enqueue);
}

bool HttpSocket::_EnqueueOrSend
(
const Request& req,
bool forceQueue
)
{
    traceprint("HttpSocket::_EnqueueOrSend, forceQueue = %d\n", forceQueue);
    if(_inProgress || forceQueue) // do not send while receiving other data
    {
        traceprint("HTTP: Transfer pending; putting into queue. Now %u waiting.\n", (unsigned int)_requestQ.size());
        _requestQ.push(req);
        return true;
    }
    // ok, we can send directly
    traceprint("HTTP: Open request for immediate send.\n");
    if(!_OpenRequest(req))
        return false;
    bool sent = SendBytes(req.header.c_str(), req.header.length());
    _inProgress = sent;
    return sent;
}

// called whenever a request is finished completely and the socket checks for more things to send
void HttpSocket::_DequeueMore
(
)
{
    traceprint("HttpSocket::_DequeueMore, Q = %u\n", (unsigned)_requestQ.size());
    _FinishRequest(); // In case this was not done yet.

    // _inProgress is known to be false here
    if(_requestQ.size()) // still have other requests queued?
        if(_EnqueueOrSend(_requestQ.front(), false)) // could we send?
            _requestQ.pop(); // if so, we are done with this request

    // otherwise, we are done for now. socket is kept alive for future sends. Nothing to do.
}

bool HttpSocket::_OpenRequest
(
const Request& req
)
{
    if(_inProgress)
    {
        traceprint("HttpSocket::_OpenRequest(): _inProgress == true, should not be called.");
        return false;
    }
    if(req.useSSL && !hasSSL())
    {
        traceprint("HttpSocket::_OpenRequest(): Is an SSL connection, but SSL was not inited, doing that now\n");
        if(!initSSL(NULL)) // FIXME: supply cert list?
        {
            traceprint("FAILED to init SSL");
            return false;
        }
    }
    if(!open(req.host.c_str(), req.port))
        return false;
    _inProgress = true;
    _curRequest = req;
    _status = 0;
    return true;
}

void HttpSocket::_FinishRequest
(
)
{
    traceprint("HttpSocket::_FinishRequest\n");
    if(_inProgress)
    {
        traceprint("... in progress. redirecting = %d\n", IsRedirecting());
        if(!IsRedirecting() || _alwaysHandle)
            _OnRequestDone(); // notify about finished request
        _inProgress = false;
        _hdrs.clear();
        if(_mustClose)
            close();
    }
}

void HttpSocket::_ProcessChunk
(
)
{
    if(!_chunkedTransfer)
        return;

    unsigned int chunksize = -1;

    while(true)
    {
        // less data required until chunk end than received, means the new chunk starts somewhere in the middle
        // of the received data block. finish this chunk first.
        if(_remaining)
        {
            if(_remaining <= _recvSize) // it contains the rest of the chunk, including CRLF
            {
                _OnRecvInternal(_readptr, _remaining - 2); // implicitly skip CRLF
                _readptr += _remaining;
                _recvSize -= _remaining;
                _remaining = 0; // done with this one.
                if(!chunksize) // and if chunksize was 0, we are done with all chunks.
                    break;
            }
            else // buffer did not yet arrive completely
            {
                _OnRecvInternal(_readptr, _recvSize);
                _remaining -= _recvSize;
                _recvSize = 0; // done with the whole buffer, but not with the chunk
                return; // nothing else to do here
            }
        }

        // each chunk identifier ends with CRLF.
        // if we don't find that, we hit the corner case that the chunk identifier was not fully received.
        // in that case, adjust the buffer and wait for the rest of the data to be appended
        char *term = strstr(_readptr, "\r\n");
        if(!term)
        {
            if(_recvSize) // if there is still something queued, move it to the left of the buffer and append on next read
                _ShiftBuffer();
            return;
        }
        term += 2; // skip CRLF

        // when we are here, the (next) chunk header was completely received.
        chunksize = strtoul(_readptr, NULL, 16);
        _remaining = chunksize + 2; // the http protocol specifies that each chunk has a trailing CRLF
        _recvSize -= (term - _readptr);
        _readptr = term;
    }

    if(!chunksize) // this was the last chunk, no further data expected unless requested
    {
        _chunkedTransfer = false;
        _DequeueMore();
        if(_recvSize)
            traceprint("_ProcessChunk: There are %u bytes left in the buffer, huh?\n", _recvSize);
        if(_mustClose)
            close();
    }
}

void HttpSocket::_ParseHeaderFields
(
const char *s,
size_t size
)
{
    // Key: Value data\r\n

    const char * const maxs = s + size;
    while(s < maxs)
    {
        while(isspace(*s))
        {
            ++s;
            if(s >= maxs)
                return;
        }
        const char * const colon = strchr(s, ':');
        if(!colon)
            return;
        const char *valEnd = strchr(colon, '\n'); // last char of val data
        if(!valEnd)
            return;
        while(valEnd[-1] == '\n' || valEnd[-1] == '\r') // skip backwards if necessary
            --valEnd;
        const char *val = colon + 1; // value starts after ':' ...
        while(isspace(*val) && val < valEnd) // skip spaces after the colon
            ++val;
        std::string key(s, colon - s);
        key = ToLower(key);
        std::string valstr(val, valEnd - val);
        _hdrs[key] = valstr;
        traceprint("HDR: %s: %s\n", key.c_str(), valstr.c_str());
        s = valEnd;
    }
}

const char *HttpSocket::Hdr
(
const char *h
) const
{
    std::map<std::string, std::string>::const_iterator it = _hdrs.find(h);
    return it == _hdrs.end() ? NULL : it->second.c_str();
}

static int safeatoi
(
const char *s
)
{
    return s ? atoi(s) : 0;
}

bool HttpSocket::_HandleStatus
(
)
{
    _remaining = _contentLen = safeatoi(Hdr("content-length"));

    const char *encoding = Hdr("transfer-encoding");
    _chunkedTransfer = encoding && !STRNICMP(encoding, "chunked", 7);

    const char *conn = Hdr("connection"); // if its not keep-alive, server will close it, so we can too
    _mustClose = !conn || STRNICMP(conn, "keep-alive", 10);

    const bool success = IsSuccess();

    if(!(_chunkedTransfer || _contentLen) && success)
        traceprint("_ParseHeader: Not chunked transfer and content-length==0, this will go fail");

    traceprint("Got HTTP Status %d\n", _status);

    if(success)
        return true;

    bool forceGET = false;
    switch(_status)
    {
        case 303:
            forceGET = true; // As per spec, continue with a GET request
        case 301:
        case 302:
        case 307:
        case 308:
            if(_followRedir)
                if(const char *loc = Hdr("location"))
                    _Redirect(loc, forceGET);
            return false;

        default:
            return false;
    }
}

bool HttpSocket::IsRedirecting
(
)
const
{
    switch(_status)
    {
        case 301:
        case 302:
        case 303:
        case 307:
        case 308:
            return true;
    }
    return false;
}

bool HttpSocket::IsSuccess
(
) const
{
    const unsigned s = _status;
    return s >= 200 && s <= 205;
}



void HttpSocket::_ParseHeader
(
)
{
    _tmpHdr += _inbuf;
    const char *hptr = _tmpHdr.c_str();

    if((_recvSize >= 5 || _tmpHdr.size() >= 5) && memcmp("HTTP/", hptr, 5))
    {
        traceprint("_ParseHeader: not HTTP stream\n");
        return;
    }

    const char *hdrend = strstr(hptr, "\r\n\r\n");
    if(!hdrend)
    {
        traceprint("_ParseHeader: could not find end-of-header marker, or incomplete buf; delaying.\n");
        return;
    }

    //traceprint(hptr);

    hptr = strchr(hptr + 5, ' '); // skip "HTTP/", already known
    if(!hptr)
        return; // WTF?
    ++hptr; // number behind first space is the status code
    _status = atoi(hptr);

    // Default values
    _chunkedTransfer = false;
    _contentLen = 0; // yet unknown

    hptr = strstr(hptr, "\r\n");
    _ParseHeaderFields(hptr + 2, hdrend - hptr);

    // FIXME: return value indicates success.
    // Bail out on non-success, or at least make it so that _OnRecv() is not called.
    // (Unless an override bool is given that even non-successful answers get their data delivered!)
    _HandleStatus();

    // get ready
    _readptr = strstr(_inbuf, "\r\n\r\n") + 4; // skip double newline. must have been found in hptr earlier.
    _recvSize -= (_readptr - _inbuf); // skip the header part
    _tmpHdr.clear();
}

// generic http header parsing
void HttpSocket::_OnData
(
)
{
    if(!(_chunkedTransfer || (_remaining && _recvSize)))
        _ParseHeader();

    if(_chunkedTransfer)
    {
        _ProcessChunk(); // first, try to finish one or more chunks
    }
    else if(_remaining && _recvSize) // something remaining? if so, we got a header earlier, but not all data
    {
        _remaining -= _recvSize;
        _OnRecvInternal(_readptr, _recvSize);

        if(int(_remaining) < 0)
        {
            traceprint("_OnRecv: _remaining wrap-around, huh??\n");
            _remaining = 0;
        }
        if(!_remaining) // received last block?
        {
            if(_mustClose)
                close();
            else
                _DequeueMore();
        }

        // nothing else to do here.
    }

    // otherwise, the server sent just the header, with the data following in the next packet
}

void HttpSocket::_OnRecv(void *buf, unsigned int size)
    {
        if(!size)
            return;

        if (!_pFile) {
        	_pFile = fopen( _filename.c_str(), "w" );
        }
        if (!_pFile) {
        	Log::Write(LogLevel_Error, "Failed to open file %s: %s", _filename.c_str(), strerror(errno));
        	return;
        }

        fwrite(buf, size, 1, _pFile );
    }


void HttpSocket::_OnClose
(
)
{
    if(!ExpectMoreData())
        _FinishRequest();
    if (_pFile) {
    	fclose(_pFile);
    	_pFile = NULL;
    }
}

void HttpSocket::_OnRecvInternal
(
void *buf,
unsigned int size
)
{
    if(IsSuccess() || _alwaysHandle)
        _OnRecv(buf, size);
}

#endif

// ===========================
// ===== SOCKET SET ==========
// ===========================
#ifdef MINIHTTP_SUPPORT_SOCKET_SET

SocketSet::~SocketSet
(
)
{
    deleteAll();
}

void SocketSet::deleteAll
(
)
{
    for(Store::iterator it = _store.begin(); it != _store.end(); ++it)
        delete it->first;
    _store.clear();
}

bool SocketSet::update
(
)
{
    bool interesting = false;
    Store::iterator it = _store.begin();
    for( ; it != _store.end(); )
    {
        TcpSocket *sock =  it->first;
        SocketSetData& sdata = it->second;
        interesting = sock->update() || interesting;
        if(sdata.deleteWhenDone && !sock->isOpen() && !sock->HasPendingTask())
        {
            traceprint("Delete socket\n");
            delete sock;
            _store.erase(it++);
        }
        else
           ++it;
    }
    return interesting;
}

void SocketSet::remove
(
TcpSocket *s
)
{
    _store.erase(s);
}

void SocketSet::add
(
TcpSocket *s,
bool deleteWhenDone
)
{
    s->SetNonBlocking(true);
    SocketSetData sdata;
    sdata.deleteWhenDone = deleteWhenDone;
    _store[s] = sdata;
}

#endif

} // namespace HttpClient
} // namespace OpenZWave
