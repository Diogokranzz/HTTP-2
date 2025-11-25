@echo off
set OPENSSL_BIN=vendor\vcpkg\downloads\tools\perl\5.42.0.1\c\bin\openssl.exe
set OPENSSL_CONF=vendor\vcpkg\buildtrees\openssl\x64-windows-rel\apps\openssl.cnf

echo Generating Private Key...
"%OPENSSL_BIN%" genrsa -out server.key 2048

echo Generating Certificate Signing Request (CSR)...
"%OPENSSL_BIN%" req -new -key server.key -out server.csr -subj "/C=BR/ST=SP/L=Sao Paulo/O=DK Server/CN=localhost"

echo Generating Self-Signed Certificate...
"%OPENSSL_BIN%" x509 -req -days 365 -in server.csr -signkey server.key -out server.crt

echo Done!
echo Key: server.key
echo Cert: server.crt
