#!/usr/bin/python3

import re
import sys

F = sys.argv[1]

with open(F, 'rb') as f:
    content = f.read()

content = re.sub(br'L("([^"\\]|\\.)*")', br'U(\1)', content)
content = content.replace(b'std::wstring', b'std::string')
content = content.replace(b'boost/filesystem.hpp', b'filesystem')
content = content.replace(b'boost::filesystem', b'std::filesystem')

content = content.replace(b'"%FT%T%z"', b'"%Y-%m-%dT%H:%M:%SZ"')
content = content.replace(b'localtime(', b'gmtime(')

content = content.replace(b'winsock2.h', b'WinSock2.h')

# ===== PATCH CHO AppleAPI.cpp =====
if F.endswith('AppleAPI.cpp'):
    content = content.replace(b'\r\n', b'\n')
    content = content.replace(b'\r', b'')

    # 1. Xóa Windows includes
    content = content.replace(b'#include <WinSock2.h>\n', b'')
    content = content.replace(b'#include <WinSock2.h>', b'')
    content = content.replace(b'#include <windows.h>\n', b'')
    content = content.replace(b'#include <windows.h>', b'')
    content = content.replace(b'#include <winhttp.h>\n', b'')
    content = content.replace(b'#include <winhttp.h>', b'')
    content = content.replace(b'#include <WS2tcpip.h>\n', b'')
    content = content.replace(b'#include <WS2tcpip.h>', b'')
    content = content.replace(b'#pragma comment(lib, "winhttp.lib")\n', b'')
    content = content.replace(b'#pragma comment(lib, "winhttp.lib")', b'')

    # 2. Xóa cpprestsdk compression extension
    content = content.replace(b'#include <cpprest/http_compression.h>\n', b'')
    content = content.replace(b'#include <cpprest/http_compression.h>', b'')

    # 3. odslog: OutputDebugStringA -> fprintf
    content = content.replace(
        b'OutputDebugStringA(ss.str().c_str());',
        b'fprintf(stderr, "%s", ss.str().c_str());'
    )

    # 4. Thêm zlib + cstdio
    if b'#include <zlib.h>' not in content:
        content = content.replace(
            b'#include <openssl/pem.h>',
            b'#include <zlib.h>\n#include <cstdio>\n#include <openssl/pem.h>'
        )

    # 5. decompress() dùng zlib thuần
    decompress_linux = br'''bool decompress(const uint8_t* input, size_t input_size, std::vector<uint8_t>& output)
{
    if (input == nullptr || input_size == 0) return false;
    z_stream strm = {};
    strm.next_in = const_cast<Bytef*>(input);
    strm.avail_in = (uInt)input_size;
    if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK) return false;
    output.resize(input_size * 3);
    strm.next_out = output.data();
    strm.avail_out = (uInt)output.size();
    int ret;
    do {
        if (strm.avail_out == 0) {
            output.resize(output.size() * 2);
            strm.next_out = output.data() + strm.total_out;
            strm.avail_out = (uInt)(output.size() - strm.total_out);
        }
        ret = inflate(&strm, Z_NO_FLUSH);
    } while (ret == Z_OK);
    inflateEnd(&strm);
    if (ret != Z_STREAM_END) return false;
    output.resize(strm.total_out);
    return true;
}'''
    content = re.sub(
        br'bool decompress\(const uint8_t\* input[\s\S]+?return true;\n\}',
        decompress_linux,
        content
    )

    # 6. gsaClient() - fix 503
    gsa_linux = br'''web::http::client::http_client AppleAPI::gsaClient()
{
    http_client_config config;
    config.set_validate_certificates(false);
    config.set_keep_alive(false);
    return web::http::client::http_client(U("https://gsa.apple.com"), config);
}'''
    content = re.sub(
        br'web::http::client::http_client AppleAPI::gsaClient\(\)[\s\S]+?\n\}',
        gsa_linux,
        content
    )
# ===== HET PATCH AppleAPI.cpp =====

sys.stdout.buffer.write(content)
