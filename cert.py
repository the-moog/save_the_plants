#!/usr/bin/env python3


import urllib.request
import urllib.parse
import re
import ssl
import sys
import socket
import argparse
import datetime
from warnings import WarningMessage
from pathlib import Path
import io
import pickle

from cryptography import x509
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.serialization import pkcs7
from cryptography.hazmat.primitives.serialization import Encoding
from cryptography.hazmat.primitives.serialization import PublicFormat

def get_dt() -> str:
    return datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")


class Certificate:

    def __init__(self, cert_data: str, clang_symbol: str|None = None):
        self._cert_data = cert_data
        self._symbol = clang_symbol
        self._cert = None

    def set_symbol(self, symbol: str|None) -> None:
        self._symbol = symbol

    @property
    def cert(self) -> str:
        # Cache the result so it's faster each access
        if self._cert is None:
            self.decode_cert()
        return self._cert

    def decode_cert(self) -> None:
        """
        Try and decode cert_data into either an x509 or pkcs7 certificate
        """
        self._cert = None
        try:
            self._cert = x509.load_der_x509_certificate(self._cert_data)
        except:
            try:
                self._cert = x509.load_pem_x509_certificate(self._cert_data)
            except:
                try:
                    self._cert = pkcs7.load_der_pkcs7_certificates(self._cert_data)
                except:
                    self._cert = pkcs7.load_pem_pkcs7_certificates(self._cert_data)
                if len(self._cert) > 1:
                    WarningMessage(f'// Warning: TODO: pkcs7 has {len(self._cert)} entries')

    @property
    def ca_list(self) -> list[str]:
        """
        Get a list of CAs that have signed this certificate
        """   
        cas = []
        if self.cert is not None:
            for ext in self.cert.extensions:
                if ext.oid == x509.ObjectIdentifier("1.3.6.1.5.5.7.1.1"):
                    for desc in ext.value:
                        if desc.access_method == x509.oid.AuthorityInformationAccessOID.CA_ISSUERS:
                            cas.append(desc.access_location.value)
        return cas  
    
    @property
    def cn(self) -> str:
        """
        Extract the common name (CN) from distinguished name (DN)
        """
        cn = ''
        for dn in self.cert.subject.rfc4514_string().split(','):
            keyval = dn.split('=')
            if keyval[0] == 'CN':
                cn += keyval[1]
        return cn
    
    @property
    def clang_name(self) -> str:
        """
        Make a valid C name
        """
        return re.sub('[^a-zA-Z0-9_]', '_', self.cn)

    @property
    def clang_symbol(self) -> str:
        if self._symbol is None:
            return self.clang_name
        return self._symbol

    @property
    def fingerPrint(self) -> str:
        return self.cert.fingerprint(hashes.SHA1()).hex(':')
    
    @property
    def pubKey_pem(self) -> str:
        return self.cert.public_key().public_bytes(Encoding.PEM, PublicFormat.SubjectPublicKeyInfo).decode('utf-8')

    @property
    def clang_fingerPrint(self) -> str:
        return f'const char fingerprint_{self.clang_name} [] PROGMEM = "{self.fingerPrint}";'

    @property
    def clang_pubKey(self) -> str:
        lines = []
        lines.append(f'const char pubkey_{self.clang_name} [] PROGMEM = R"PUBKEY(')
        lines.append(f'{self.pubKey_pem})PUBKEY";')
        return "\n".join(lines)

    @property
    def cert_pem(self) -> str:
        return self.cert.public_bytes(Encoding.PEM).decode('utf-8')

    @property
    def clang_pem(self) -> str:
        lines = []
        lines.append(f'const char cert_{self.clang_name} [] PROGMEM = R"CERT(')
        lines.append(f'{self.cert_pem})CERT";')
        lines.append(f'const char cert_cn_{self.clang_name} [] PROGMEM = "{self.cn}";')
        return "\n".join(lines)

    @property
    def not_valid_before(self) -> str:
        return self.cert.not_valid_before
    
    @property
    def not_valid_after(self) -> str:
        return self.cert.not_valid_after


class CertChain:

    def __init__(self, hostname: str, port: int|str = 443, name: str|None = None):
        """
        Get the certificate chain for https://<host>:<port> and (optionally)
        use <name> to describe it
        """
        self._host = hostname
        self._port = int(port)
        self._name = name
        self._chain = dict()
        self._root = None

    def get_chain(self, data: str, showPub: bool = False, symbol: str|None = None, clang: bool = False) -> list[str]:
        cert = Certificate(data, clang_symbol=symbol)
        self._chain[hash(data)] = cert
        self._root = cert

        lines = []
        if clang:
            append = lambda x: lines.append(x)
        else:
            append = lambda x : None

        append(f'// CN: {cert.cn} => name: {cert.clang_name}')
        append(f'// not valid before: {cert.not_valid_before}')
        append(f'// not valid after: {cert.not_valid_after}')

        if showPub:
            append(cert.clang_fingerPrint)
            append(cert.clang_pubKey)
        else:
            append(cert.clang_pem)

        if showPub:
            append(f'const char * const {cert.clang_symbol}_fingerprint = fingerprint_{cert.clang_name};')
            append(f'const char * const {cert.clang_symbol}_pubkey = pubkey_{cert.clang_name};')

        for ca in cert.ca_list:
            with urllib.request.urlopen(ca) as crt:
                append("")
                append('// ' + ca)
                # Set root to the most recent cert as the last one is the root for this chain
                self._root = cert
                for line in self.get_chain(crt.read(), showPub=False, clang=clang):
                    append(line)
            append("")
        
        return lines
    
    @property
    def ca_symbol(self) -> str:
        if self._root is None:
            return "INVALID_ROOT"
        return self._root.clang_symbol
    
    @staticmethod
    def fetch_pem(host: str, port: int|str) -> str:
        """
        Get a PEM cert for a given host
        """
        context = ssl.create_default_context()
        context.check_hostname = False
        context.verify_mode = ssl.CERT_NONE
        with socket.create_connection((host, int(port))) as sock:
            with context.wrap_socket(sock, server_hostname=host) as ssock:
                pem = ssock.getpeercert(binary_form=True)
        return pem

    @property
    def pem(self):
        """
        Return the PEM format certificate for the host we are interested in
        """
        return self.fetch_pem(self._host, self._port)

    def get_clang_cert(self, include_url: bool = True) -> list[str]:
        """
        Get the PEM certificate and encode it into valid C syntax
        """
        lines = []

        # Chain header
        lines.append('////////////////////////////////////////////////////////////')
        lines.append(f'// certificate chain for {self._host}:{self._port}')
        lines.append("")
        if include_url:
            lines.append(f'const char * {self._name}_fqdn = "{self._host}";')
            lines.append(f'const uint16_t {self._name}_port = {self._port};')
            lines.append("")

        # Chain data (recursive)
        lines.extend(self.get_chain(self.pem, symbol=self._name, clang=True))

        # Chain footer
        lines.append(f'const char * const Root_CA = cert_{self.ca_symbol};')
        lines.append(f'const char * const Root_CA_cn = cert_cn_{self.ca_symbol};')
        lines.append(f'// end of certificate chain for {self._host}:{self._port}')
        lines.append('////////////////////////////////////////////////////////////')
        lines.append("")
        return lines
    
    @staticmethod
    def get_clang_header() -> list[str]:
        lines = []
        lines.append("")
        lines.append('// this file is autogenerated - any modification will be overwritten')
        lines.append('// unused symbols will not be linked in the final binary')
        lines.append(f'// generated on {get_dt()}')
        lines.append(f'// by {" ".join(sys.argv)} on {sys.platform}')
        lines.append("")
        lines.append('#pragma once')
        lines.append("")
        return lines
    
    def cert_as_clang(self) -> str:
        lines = []
        lines.extend(self.get_clang_header())
        lines.extend(self.get_clang_cert())
        return "\n".join(lines)
    
    def cert_as_chain(self) -> object:
        return self._chain
    
    def get_ca_pem(self) -> str:
        self.get_chain(self.pem, symbol=self._name, clang=True)
        return self._root.cert_pem
    
    def write_chain(self, output, force=False, filetype='c'):
        opened = False
        if isinstance(output, Path):
            if output.exists():
                if not force:
                    raise IOError("File exists, not writing. Use -f to force")
                else:
                    Path.unlink()
            opened = True
            stream = Path.open("wb")
        elif isinstance(output, io.IOBase):
            stream = output 
        else:
            raise IOError("I don't know how to handle {output}")
        self._write_chain_stream(stream, filetype=filetype)
        stream.flush()
        if not stream.isatty() and opened:
            stream.close()

    def _write_chain_stream(self, stream, filetype=None):
        if not isinstance(filetype,str) or filetype[0].lower() not in "cpa":
            raise ValueError("Valid filetypes are [p]ickle or [c]lang or c[a]pem")
        filetype = filetype.lower()
        if filetype == "c":
            stream.write(self.cert_as_clang())
        if filetype == "p":
            dd = {"pickle_version": 1, "data": self.cert_as_chain(), "dt":datetime.datetime.utcnow(), "type":type(self.cert_as_chain())}
            pp = pickle.dumps(dd)
            stream.write(pp)
        if filetype == "a":
            stream.write(self.get_ca_pem())


# External mondule interface
def get_ca_pem(host: str, port: int|str = 443, name: str|None = None) -> str:
    chain = CertChain(host, port, name)
    return chain.get_ca_pem()

DEFAULT_PORT = 443
# CLI Interface
def main():
    parser = argparse.ArgumentParser(description='download certificate chain and public keys under a C++/Arduino compilable form')
    grp = parser.add_mutually_exclusive_group(required=True)
    grp.add_argument('-s', '--server', dest="server", type=str, action='store',  help='TLS server dns name',default=None)
    grp.add_argument('-u', '--url', dest="url", type=str, action='store',  help="URL", default=None)
    parser.add_argument('-p', '--port', type=int, dest="port", help='TLS server port', default=None)
    parser.add_argument('-n', '--name', type=str, dest="name", help='variable name', default=None)
    parser.add_argument('-o', dest="file", default=None, help="output to <file> (default stdout)")
    parser.add_argument('-f', "--force", dest='force',help="Force overwrite of file", action="store_true", default=False)

    args = parser.parse_args()
    url = args.server if args.server is not None else args.url if args.url is not None else None
    if args.url is not None or ":" in args.server:
        res = urllib.parse.urlsplit(url, scheme="https")
        res._replace(scheme="https")
    else:
        url = args.server
    server = res.hostname
    try:
        port = res.port
    except ValueError:
        if args.port is None:
            WarningMessage(f"Invalid port in url {url}, using default: {DEFAULT_PORT}")
            port = DEFAULT_PORT
        else:
            port = args.port

    ofp = args.file
    if ofp is None:
        ofp = sys.stdout
    elif isinstance(ofp, str):
        ofp = Path(ofp)

    chain = CertChain(server, port, args.name)
    chain.write_chain(ofp, force=args.force, filetype="c" )
    return 0

if __name__ == '__main__':
    sys.exit(main())
