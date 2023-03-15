#!/usr/bin/env python3


import urllib.request
import re
import ssl
import sys
import socket
import argparse
import datetime
from warnings import WarningMessage

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

    def set_symbol(self, symbol: str|None):
        self._symbol = symbol

    @property
    def cert(self):
        # Cache the result so it's faster each access
        if self._cert is None:
            self.decode_cert(self._cert_data)
        return self._cert

    def decode_cert(self):
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
    def ca_list(self):
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
    def cn(self):
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
    def clang_name(self):
        """
        Make a valid C name
        """
        return re.sub('[^a-zA-Z0-9_]', '_', self.cn)

    @property
    def clang_symbol(self):
        if self._symbol is None:
            return self.clang_name
        return self._symbol

    @property
    def fingerPrint(self):
        return self.cert.fingerprint(hashes.SHA1()).hex(':')
    
    @property
    def pubKey_pem(self):
        return self.cert.public_key().public_bytes(Encoding.PEM, PublicFormat.SubjectPublicKeyInfo).decode('utf-8')

    @property
    def clang_fingerPrint(self):
        return f'const char fingerprint_{self.clang_name} [] PROGMEM = "{self.fingerPrint}";'

    @property
    def clang_pubKey(self):
        lines = []
        lines.append(f'const char pubkey_{self.clang_name} [] PROGMEM = R"PUBKEY(')
        lines.append(self.pubKey_pem + ')PUBKEY";')
        return lines.join("\n")

    @property
    def cert_pem(self):
        return self.cert.public_bytes(Encoding.PEM).decode('utf-8')

    @property
    def clang_pem(self):
        lines = []
        lines.append(f'const char cert_{self.clang_name} [] PROGMEM = R"CERT(')
        lines.append(self.cert_pem + ')CERT";')
        lines.append(f'const char cert_cn_{self.clang_name} [] PROGMEM = "{self.cn}";')

    @property
    def not_valid_before(self):
        return self.cert.not_valid_before
    
    @property
    def not_valid_after(self):
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
            append = lines.append
        else:
            append = lambda x : None

        append(f'// CN: {cert.cn} => name: {cert.clang_name}')
        append('// not valid before:', cert.not_valid_before)
        append('// not valid after: ', cert.not_valid_after)

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
                self.get_chain(crt.read(), showPub=False, clang=clang)
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
        self.ca_symbol = None

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
        lines.append(f'// by {sys.argv}')
        lines.append("")
        lines.append('#pragma once')
        lines.append("")
        return lines
    
    def cert_as_clang(self) -> str:
        lines = []
        lines.extend(self.get_clang_header())
        lines.extend(self.get_clang_cert())
        return "\n".join(lines)
    
    def get_ca_pem(self) -> str:
        self.get_chain(self.pem, symbol=self._name, clang=True)
        return self._root.cert_pem

# External mondule interface
def get_ca_pem(host: str, port: int|str = 443, name: str|None = None) -> str:
    chain = CertChain(host, port, name)
    return chain.get_ca_pem()

# CLI Interface
def main():
    parser = argparse.ArgumentParser(description='download certificate chain and public keys under a C++/Arduino compilable form')
    parser.add_argument('-s', '--server', dest="server", action='store', required=True, help='TLS server dns name')
    parser.add_argument('-p', '--port', type=int, dest="port", help='TLS server port', default=443)
    parser.add_argument('-n', '--name', type=str, dest="name", help='variable name', default=None)

    args = parser.parse_args()
    server = args.server
    try:
        split = server.split(':')
        server = split[0]
        port = int(split[1])
    except:
        pass
    try:
        port = args.port
    except:
        pass
    
    chain = CertChain(server, port, args.name)
    print(chain.cert_as_clang())
    return 0

if __name__ == '__main__':
    sys.exit(main())
