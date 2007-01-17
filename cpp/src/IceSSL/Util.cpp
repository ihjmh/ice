// **********************************************************************
//
// Copyright (c) 2003-2007 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <IceSSL/Util.h>
#include <Ice/LocalException.h>
#include <Ice/Network.h>

#ifdef _WIN32
#   include <direct.h>
#   include <sys/types.h>
#   include <sys/stat.h>
#   ifdef _MSC_VER
#     define S_ISDIR(mode) ((mode) & _S_IFDIR)
#     define S_ISREG(mode) ((mode) & _S_IFREG)
#   endif
#else
#   include <sys/stat.h>
#endif

#include <openssl/err.h>

#include <IceUtil/DisableWarnings.h>

using namespace std;
using namespace Ice;
using namespace IceSSL;

#ifndef OPENSSL_NO_DH

// The following arrays are predefined Diffie Hellman group parameters.
// These are known strong primes, distributed with the OpenSSL library
// in the files dh512.pem, dh1024.pem, dh2048.pem and dh4096.pem.
// They are not keys themselves, but the basis for generating DH keys
// on the fly.

static unsigned char dh512_p[] =
{
    0xF5,0x2A,0xFF,0x3C,0xE1,0xB1,0x29,0x40,0x18,0x11,0x8D,0x7C,
    0x84,0xA7,0x0A,0x72,0xD6,0x86,0xC4,0x03,0x19,0xC8,0x07,0x29,
    0x7A,0xCA,0x95,0x0C,0xD9,0x96,0x9F,0xAB,0xD0,0x0A,0x50,0x9B,
    0x02,0x46,0xD3,0x08,0x3D,0x66,0xA4,0x5D,0x41,0x9F,0x9C,0x7C,
    0xBD,0x89,0x4B,0x22,0x19,0x26,0xBA,0xAB,0xA2,0x5E,0xC3,0x55,
    0xE9,0x2A,0x05,0x5F,
};

static unsigned char dh512_g[] = { 0x02 };

static unsigned char dh1024_p[] =
{
    0xF4,0x88,0xFD,0x58,0x4E,0x49,0xDB,0xCD,0x20,0xB4,0x9D,0xE4,
    0x91,0x07,0x36,0x6B,0x33,0x6C,0x38,0x0D,0x45,0x1D,0x0F,0x7C,
    0x88,0xB3,0x1C,0x7C,0x5B,0x2D,0x8E,0xF6,0xF3,0xC9,0x23,0xC0,
    0x43,0xF0,0xA5,0x5B,0x18,0x8D,0x8E,0xBB,0x55,0x8C,0xB8,0x5D,
    0x38,0xD3,0x34,0xFD,0x7C,0x17,0x57,0x43,0xA3,0x1D,0x18,0x6C,
    0xDE,0x33,0x21,0x2C,0xB5,0x2A,0xFF,0x3C,0xE1,0xB1,0x29,0x40,
    0x18,0x11,0x8D,0x7C,0x84,0xA7,0x0A,0x72,0xD6,0x86,0xC4,0x03,
    0x19,0xC8,0x07,0x29,0x7A,0xCA,0x95,0x0C,0xD9,0x96,0x9F,0xAB,
    0xD0,0x0A,0x50,0x9B,0x02,0x46,0xD3,0x08,0x3D,0x66,0xA4,0x5D,
    0x41,0x9F,0x9C,0x7C,0xBD,0x89,0x4B,0x22,0x19,0x26,0xBA,0xAB,
    0xA2,0x5E,0xC3,0x55,0xE9,0x2F,0x78,0xC7,
};

static unsigned char dh1024_g[] = { 0x02 };

static unsigned char dh2048_p[] =
{
    0xF6,0x42,0x57,0xB7,0x08,0x7F,0x08,0x17,0x72,0xA2,0xBA,0xD6,
    0xA9,0x42,0xF3,0x05,0xE8,0xF9,0x53,0x11,0x39,0x4F,0xB6,0xF1,
    0x6E,0xB9,0x4B,0x38,0x20,0xDA,0x01,0xA7,0x56,0xA3,0x14,0xE9,
    0x8F,0x40,0x55,0xF3,0xD0,0x07,0xC6,0xCB,0x43,0xA9,0x94,0xAD,
    0xF7,0x4C,0x64,0x86,0x49,0xF8,0x0C,0x83,0xBD,0x65,0xE9,0x17,
    0xD4,0xA1,0xD3,0x50,0xF8,0xF5,0x59,0x5F,0xDC,0x76,0x52,0x4F,
    0x3D,0x3D,0x8D,0xDB,0xCE,0x99,0xE1,0x57,0x92,0x59,0xCD,0xFD,
    0xB8,0xAE,0x74,0x4F,0xC5,0xFC,0x76,0xBC,0x83,0xC5,0x47,0x30,
    0x61,0xCE,0x7C,0xC9,0x66,0xFF,0x15,0xF9,0xBB,0xFD,0x91,0x5E,
    0xC7,0x01,0xAA,0xD3,0x5B,0x9E,0x8D,0xA0,0xA5,0x72,0x3A,0xD4,
    0x1A,0xF0,0xBF,0x46,0x00,0x58,0x2B,0xE5,0xF4,0x88,0xFD,0x58,
    0x4E,0x49,0xDB,0xCD,0x20,0xB4,0x9D,0xE4,0x91,0x07,0x36,0x6B,
    0x33,0x6C,0x38,0x0D,0x45,0x1D,0x0F,0x7C,0x88,0xB3,0x1C,0x7C,
    0x5B,0x2D,0x8E,0xF6,0xF3,0xC9,0x23,0xC0,0x43,0xF0,0xA5,0x5B,
    0x18,0x8D,0x8E,0xBB,0x55,0x8C,0xB8,0x5D,0x38,0xD3,0x34,0xFD,
    0x7C,0x17,0x57,0x43,0xA3,0x1D,0x18,0x6C,0xDE,0x33,0x21,0x2C,
    0xB5,0x2A,0xFF,0x3C,0xE1,0xB1,0x29,0x40,0x18,0x11,0x8D,0x7C,
    0x84,0xA7,0x0A,0x72,0xD6,0x86,0xC4,0x03,0x19,0xC8,0x07,0x29,
    0x7A,0xCA,0x95,0x0C,0xD9,0x96,0x9F,0xAB,0xD0,0x0A,0x50,0x9B,
    0x02,0x46,0xD3,0x08,0x3D,0x66,0xA4,0x5D,0x41,0x9F,0x9C,0x7C,
    0xBD,0x89,0x4B,0x22,0x19,0x26,0xBA,0xAB,0xA2,0x5E,0xC3,0x55,
    0xE9,0x32,0x0B,0x3B,
};

static unsigned char dh2048_g[] = { 0x02 };

static unsigned char dh4096_p[] =
{
    0xFA,0x14,0x72,0x52,0xC1,0x4D,0xE1,0x5A,0x49,0xD4,0xEF,0x09,
    0x2D,0xC0,0xA8,0xFD,0x55,0xAB,0xD7,0xD9,0x37,0x04,0x28,0x09,
    0xE2,0xE9,0x3E,0x77,0xE2,0xA1,0x7A,0x18,0xDD,0x46,0xA3,0x43,
    0x37,0x23,0x90,0x97,0xF3,0x0E,0xC9,0x03,0x50,0x7D,0x65,0xCF,
    0x78,0x62,0xA6,0x3A,0x62,0x22,0x83,0xA1,0x2F,0xFE,0x79,0xBA,
    0x35,0xFF,0x59,0xD8,0x1D,0x61,0xDD,0x1E,0x21,0x13,0x17,0xFE,
    0xCD,0x38,0x87,0x9E,0xF5,0x4F,0x79,0x10,0x61,0x8D,0xD4,0x22,
    0xF3,0x5A,0xED,0x5D,0xEA,0x21,0xE9,0x33,0x6B,0x48,0x12,0x0A,
    0x20,0x77,0xD4,0x25,0x60,0x61,0xDE,0xF6,0xB4,0x4F,0x1C,0x63,
    0x40,0x8B,0x3A,0x21,0x93,0x8B,0x79,0x53,0x51,0x2C,0xCA,0xB3,
    0x7B,0x29,0x56,0xA8,0xC7,0xF8,0xF4,0x7B,0x08,0x5E,0xA6,0xDC,
    0xA2,0x45,0x12,0x56,0xDD,0x41,0x92,0xF2,0xDD,0x5B,0x8F,0x23,
    0xF0,0xF3,0xEF,0xE4,0x3B,0x0A,0x44,0xDD,0xED,0x96,0x84,0xF1,
    0xA8,0x32,0x46,0xA3,0xDB,0x4A,0xBE,0x3D,0x45,0xBA,0x4E,0xF8,
    0x03,0xE5,0xDD,0x6B,0x59,0x0D,0x84,0x1E,0xCA,0x16,0x5A,0x8C,
    0xC8,0xDF,0x7C,0x54,0x44,0xC4,0x27,0xA7,0x3B,0x2A,0x97,0xCE,
    0xA3,0x7D,0x26,0x9C,0xAD,0xF4,0xC2,0xAC,0x37,0x4B,0xC3,0xAD,
    0x68,0x84,0x7F,0x99,0xA6,0x17,0xEF,0x6B,0x46,0x3A,0x7A,0x36,
    0x7A,0x11,0x43,0x92,0xAD,0xE9,0x9C,0xFB,0x44,0x6C,0x3D,0x82,
    0x49,0xCC,0x5C,0x6A,0x52,0x42,0xF8,0x42,0xFB,0x44,0xF9,0x39,
    0x73,0xFB,0x60,0x79,0x3B,0xC2,0x9E,0x0B,0xDC,0xD4,0xA6,0x67,
    0xF7,0x66,0x3F,0xFC,0x42,0x3B,0x1B,0xDB,0x4F,0x66,0xDC,0xA5,
    0x8F,0x66,0xF9,0xEA,0xC1,0xED,0x31,0xFB,0x48,0xA1,0x82,0x7D,
    0xF8,0xE0,0xCC,0xB1,0xC7,0x03,0xE4,0xF8,0xB3,0xFE,0xB7,0xA3,
    0x13,0x73,0xA6,0x7B,0xC1,0x0E,0x39,0xC7,0x94,0x48,0x26,0x00,
    0x85,0x79,0xFC,0x6F,0x7A,0xAF,0xC5,0x52,0x35,0x75,0xD7,0x75,
    0xA4,0x40,0xFA,0x14,0x74,0x61,0x16,0xF2,0xEB,0x67,0x11,0x6F,
    0x04,0x43,0x3D,0x11,0x14,0x4C,0xA7,0x94,0x2A,0x39,0xA1,0xC9,
    0x90,0xCF,0x83,0xC6,0xFF,0x02,0x8F,0xA3,0x2A,0xAC,0x26,0xDF,
    0x0B,0x8B,0xBE,0x64,0x4A,0xF1,0xA1,0xDC,0xEE,0xBA,0xC8,0x03,
    0x82,0xF6,0x62,0x2C,0x5D,0xB6,0xBB,0x13,0x19,0x6E,0x86,0xC5,
    0x5B,0x2B,0x5E,0x3A,0xF3,0xB3,0x28,0x6B,0x70,0x71,0x3A,0x8E,
    0xFF,0x5C,0x15,0xE6,0x02,0xA4,0xCE,0xED,0x59,0x56,0xCC,0x15,
    0x51,0x07,0x79,0x1A,0x0F,0x25,0x26,0x27,0x30,0xA9,0x15,0xB2,
    0xC8,0xD4,0x5C,0xCC,0x30,0xE8,0x1B,0xD8,0xD5,0x0F,0x19,0xA8,
    0x80,0xA4,0xC7,0x01,0xAA,0x8B,0xBA,0x53,0xBB,0x47,0xC2,0x1F,
    0x6B,0x54,0xB0,0x17,0x60,0xED,0x79,0x21,0x95,0xB6,0x05,0x84,
    0x37,0xC8,0x03,0xA4,0xDD,0xD1,0x06,0x69,0x8F,0x4C,0x39,0xE0,
    0xC8,0x5D,0x83,0x1D,0xBE,0x6A,0x9A,0x99,0xF3,0x9F,0x0B,0x45,
    0x29,0xD4,0xCB,0x29,0x66,0xEE,0x1E,0x7E,0x3D,0xD7,0x13,0x4E,
    0xDB,0x90,0x90,0x58,0xCB,0x5E,0x9B,0xCD,0x2E,0x2B,0x0F,0xA9,
    0x4E,0x78,0xAC,0x05,0x11,0x7F,0xE3,0x9E,0x27,0xD4,0x99,0xE1,
    0xB9,0xBD,0x78,0xE1,0x84,0x41,0xA0,0xDF,
};

static unsigned char dh4096_g[] = { 0x02 };

//
// Convert a predefined parameter set into a DH value.
//
static DH*
convertDH(unsigned char* p, int plen, unsigned char* g, int glen)
{
    assert(p != 0);
    assert(g != 0);

    DH* dh = DH_new();

    if(dh != 0)
    {
        dh->p = BN_bin2bn(p, plen, 0);
        dh->g = BN_bin2bn(g, glen, 0);

        if((dh->p == 0) || (dh->g == 0))
        {
            DH_free(dh);
            dh = 0;
        }
    }

    return dh;
}

void IceInternal::incRef(IceSSL::DHParams* p) { p->__incRef(); }
void IceInternal::decRef(IceSSL::DHParams* p) { p->__decRef(); }

IceSSL::DHParams::DHParams() :
    _dh512(0), _dh1024(0), _dh2048(0), _dh4096(0)
{
}

IceSSL::DHParams::~DHParams()
{
    ParamList::iterator p;
    for(p = _params.begin(); p != _params.end(); ++p)
    {
	DH_free(p->second);
    }
    DH_free(_dh512);
    DH_free(_dh1024);
    DH_free(_dh2048);
    DH_free(_dh4096);
}

bool
IceSSL::DHParams::add(int keyLength, const string& file)
{
    BIO* bio = BIO_new(BIO_s_file());
    if(BIO_read_filename(bio, file.c_str()) <= 0)
    {
	BIO_free(bio);
	return false;
    }
    DH* dh = PEM_read_bio_DHparams(bio, 0, 0, 0);
    BIO_free(bio);
    if(!dh)
    {
	return false;
    }
    ParamList::iterator p = _params.begin();
    while(p != _params.end() && keyLength > p->first)
    {
	++p;
    }
    _params.insert(p, KeyParamPair(keyLength, dh));
    return true;
}

DH*
IceSSL::DHParams::get(int keyLength)
{
    //
    // First check the set of parameters specified by the user.
    // Return the first set whose key length is at least keyLength.
    //
    ParamList::iterator p;
    for(p = _params.begin(); p != _params.end(); ++p)
    {
	if(p->first >= keyLength)
	{
	    return p->second;
	}
    }

    //
    // No match found. Use one of the predefined parameter sets instead.
    //
    IceUtil::Mutex::Lock sync(*this);

    if(keyLength >= 4096)
    {
	if(!_dh4096)
	{
	    _dh4096 = convertDH(dh4096_p, (int) sizeof(dh4096_p), dh4096_g, (int) sizeof(dh4096_g));
	}
	return _dh4096;
    }
    else if(keyLength >= 2048)
    {
	if(!_dh2048)
	{
	    _dh2048 = convertDH(dh2048_p, (int) sizeof(dh2048_p), dh2048_g, (int) sizeof(dh2048_g));
	}
	return _dh2048;
    }
    else if(keyLength >= 1024)
    {
	if(!_dh1024)
	{
	    _dh1024 = convertDH(dh1024_p, (int) sizeof(dh1024_p), dh1024_g, (int) sizeof(dh1024_g));
	}
	return _dh1024;
    }
    else
    {
	if(!_dh512)
	{
	    _dh512 = convertDH(dh512_p, (int) sizeof(dh512_p), dh512_g, (int) sizeof(dh512_g));
	}
	return _dh512;
    }
}

#endif

static bool
selectReadWrite(SOCKET fd, bool read, int timeout)
{
#ifdef _WIN32
    fd_set rFdSet, wFdSet;
    FD_ZERO(&rFdSet);
    FD_ZERO(&wFdSet);
    if(read)
    {
	FD_SET(fd, &rFdSet);
    }
    else
    {
	FD_SET(fd, &wFdSet);
    }
#else
    struct pollfd pollfd[1];
    pollfd[0].fd = fd;
    pollfd[0].events = read ? POLLIN : POLLOUT;
#endif

repeatSelect:
    int ret;
#ifdef _WIN32
    if(timeout >= 0)
    {
	struct timeval tv;
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout - tv.tv_sec * 1000) * 1000;
	ret = ::select(static_cast<int>(fd) + 1, &rFdSet, &wFdSet, 0, &tv);
    }
    else
    {
	ret = ::select(static_cast<int>(fd) + 1, &rFdSet, &wFdSet, 0, 0);
    }
#else
    ret = ::poll(pollfd, 1, timeout); 
#endif

    if(ret == 0)
    {
	return false; // Timeout.
    }
    else if(ret == SOCKET_ERROR)
    {
	if(IceInternal::interrupted())
	{
	    goto repeatSelect;
	}
	
	SocketException ex(__FILE__, __LINE__);
	ex.error = IceInternal::getSocketErrno();
	throw ex;
    }

    return true;
}

bool
IceSSL::selectRead(SOCKET fd, int timeout)
{
    return selectReadWrite(fd, true, timeout);
}

bool
IceSSL::selectWrite(SOCKET fd, int timeout)
{
    return selectReadWrite(fd, false, timeout);
}

bool
IceSSL::splitString(const string& str, const string& delim, bool handleQuotes, vector<string>& result)
{
    string::size_type pos = str.find_first_not_of(delim + " \t");
    if(pos == string::npos)
    {
	return true;
    }

    string::value_type quoteChar = 0;
    while(pos != string::npos)
    {
	if(handleQuotes && (str[pos] == '"' || str[pos] == '\''))
	{
	    quoteChar = str[pos];
	    ++pos;
	}

	string val;
	while(pos < str.size())
	{
	    if((!handleQuotes || !quoteChar) && delim.find(str[pos]) != string::npos)
	    {
		break;
	    }
	    if(handleQuotes)
	    {
		if(str[pos] == '\\')
		{
		    if(pos + 1 < str.size() && str[pos + 1] == quoteChar)
		    {
			++pos;
		    }
		}
		else if(str[pos] == quoteChar)
		{
		    quoteChar = 0;
		    ++pos;
		    continue;
		}
	    }
	    val.push_back(str[pos]);
	    ++pos;
	}

	if(!val.empty())
	{
	    result.push_back(val);
	}

	pos = str.find_first_not_of(delim, pos);
    }

    if(quoteChar) // Mismatched quote.
    {
	return false;
    }

    return true;
}

bool
IceSSL::checkPath(string& path, const string& defaultDir, bool dir)
{
    //
    // Check if file exists. If not, try prepending the default
    // directory and check again. If the path exists, the string
    // argument is modified and true is returned. Otherwise
    // false is returned.
    //
#ifdef _WIN32
    struct _stat st;
    int err = ::_stat(path.c_str(), &st);
#else
    struct stat st;
    int err = ::stat(path.c_str(), &st);
#endif
    if(err == 0)
    {
	return dir ? S_ISDIR(st.st_mode) != 0 : S_ISREG(st.st_mode) != 0;
    }

    if(!defaultDir.empty())
    {
#ifdef _WIN32
	string s = defaultDir + "\\" + path;
	err = ::_stat(s.c_str(), &st);
#else
	string s = defaultDir + "/" + path;
	err = ::stat(s.c_str(), &st);
#endif
	if(err == 0 && ((!dir && S_ISREG(st.st_mode)) || (dir && S_ISDIR(st.st_mode))))
	{
	    path = s;
	    return true;
	}
    }

    return false;
}

ConnectionInfo
IceSSL::populateConnectionInfo(SSL* ssl, SOCKET fd, const string& adapterName, bool incoming)
{
    ConnectionInfo info;
    info.adapterName = adapterName;
    info.incoming = incoming;

    assert(ssl != 0);

    //
    // On the client side, SSL_get_peer_cert_chain returns the entire chain of certs.
    // On the server side, the peer certificate must be obtained separately.
    //
    // Since we have no clear idea whether the connection is server or client side,
    // the peer certificate is obtained separately and compared against the first
    // certificate in the chain. If they are not the same, it is added to the chain.
    //
    X509* cert = SSL_get_peer_certificate(ssl);
    STACK_OF(X509)* chain = SSL_get_peer_cert_chain(ssl);
    if(cert != 0 && (chain == 0 || sk_X509_num(chain) == 0 || cert != sk_X509_value(chain, 0)))
    {
	info.certs.push_back(new Certificate(cert));
    }
    else
    {
	X509_free(cert);
    }

    if(chain != 0)
    {
	for(int i = 0; i < sk_X509_num(chain); ++i)
	{
	    X509* cert = sk_X509_value(chain, i);
	    //
	    // Duplicate the certificate since the stack comes straight from the SSL connection.
	    //
	    info.certs.push_back(new Certificate(X509_dup(cert)));
	}
    }

    info.cipher = SSL_get_cipher_name(ssl); // Nothing needs to be free'd.

    IceInternal::fdToLocalAddress(fd, info.localAddr);

    if(!IceInternal::fdToRemoteAddress(fd, info.remoteAddr))
    {
	SocketException ex(__FILE__, __LINE__);
	ex.error = IceInternal::getSocketErrno();
	throw ex;	
    }

    return info;
}

string
IceSSL::getSslErrors(bool verbose)
{
    ostringstream ostr;

    const char* file;
    const char* data;
    int line;
    int flags;
    unsigned long err;
    int count = 0;
    while((err = ERR_get_error_line_data(&file, &line, &data, &flags)) != 0)
    {
	if(count > 0)
	{
	    ostr << endl;
	}

	if(verbose)
	{
	    if(count > 0)
	    {
		ostr << endl;
	    }

	    char buf[200];
	    ERR_error_string_n(err, buf, sizeof(buf));

	    ostr << "error # = " << err << endl;
	    ostr << "message = " << buf << endl;
	    ostr << "location = " << file << ", " << line;
	    if(flags & ERR_TXT_STRING)
	    {
		ostr << endl;
		ostr << "data = " << data;
	    }
	}
	else
	{
	    const char* reason = ERR_reason_error_string(err);
	    ostr << (reason == NULL ? "unknown reason" : reason);
	    if(flags & ERR_TXT_STRING)
	    {
		ostr << ": " << data;
	    }
	}

	++count;
    }

    ERR_clear_error();

    return ostr.str();
}
