//https://blog.csdn.net/weixin_42534940/article/details/90745452
//https://blog.csdn.net/weixin_42534940/article/details/90745452#3.%20%E8%AF%81%E4%B9%A6%E7%BD%91%E7%AB%99%E7%94%9F%E6%88%90%E6%96%B0%E8%AF%81%E4%B9%A6

mkdir certificate
cd certificate
mkdir -p CA/private
#第一步 生成私钥文件
sudo openssl genrsa -out /etc/pki/CA/private/cakey.pem 2048
#第二步 生成自签证书 -key 私钥文件 -out 证书存放位置 -new 生成新证书签署请求 -days n 证书有效天数 -x509 生成自签证书
//CN Guangdong Shenzhen TXQP Test test.Api.cn andy_ro@qq.com
sudo openssl req -new -x509 -key /etc/pki/CA/private/cakey.pem -out /etc/pki/CA/cacert.pem -days 365
#第三步 生成私钥
sudo openssl genrsa -out /etc/pki/CA/private/certificate.key 2048
#第四步 生成请求签署文件
//CN Guangdong Shenzhen TXQP Test www.testApi.com andy_ro@qq.com
sudo openssl req -new -key /etc/pki/CA/private/certificate.key -out CA/private/certificate.csr
#第五步 签署证书 -in 证书请求签署文件 -out 签发后的证书文件 -days n 证书有效天数
sudo touch CA/private/index.txt
echo 01 | sudo tee CA/private/serial
sudo cp -f CA/private/cakey.pem /etc/pki/CA/private/
sudo cp -f CA/private/cacert.pem /etc/pki/CA/
sudo cp -f CA/private/index.txt /etc/pki/CA/
sudo cp -f CA/private/serial /etc/pki/CA/

sudo cp -f /etc/pki/CA/private/cakey.pem CA/private/
sudo cp -f /etc/pki/CA/cacert.pem CA/private/
sudo openssl ca -in CA/private/certificate.csr -out CA/private/certificate.crt -days 365

sudo cp -f /etc/pki/tls/certs/ca-bundle.crt CA/private/
cat CA/private/certificate.crt CA/private/ca-bundle.crt > CA/private/certificate-ca-bundle.crt

sudo vim /etc/hostname
192.168.2.93 www.testApi.com
sudo vim /etc/hosts
192.168.2.93 www.testApi.com



#192.168.2.93:8080 uses an invalid security certificate.
#The certificate is not trusted because it is self-signed.
#The certificate is not valid for the name 192.168.2.93.
#Error code: SEC_ERROR_UNKNOWN_ISSUER/ERROR_SELF_SIGNED_CERT/SEC_ERROR_UNTRUSTED_ISSUER




#curl -v https://www.baidu.com/
* About to connect() to www.baidu.com port 443 (#0)
*   Trying 45.113.192.101...
* Connected to www.baidu.com (45.113.192.101) port 443 (#0)
* Initializing NSS with certpath: sql:/etc/pki/nssdb
*   CAfile: /etc/pki/tls/certs/ca-bundle.crt
  CApath: none
* Server certificate:
* 	subject: CN=baidu.com,O="Beijing Baidu Netcom Science Technology Co., Ltd",OU=service operation department,L=beijing,ST=beijing,
* 	start date: May 09 01:22:02 2019 GMT
* 	expire date: Jun 25 05:31:02 2020 GMT
* 	common name: baidu.com
* 	issuer: CN=GlobalSign Organization Validation CA - SHA256 - G2,O=GlobalSign nv-sa,C=BE


#curl -v https://192.168.2.93:8080/
* About to connect() to 192.168.2.93 port 8080 (#0)
*   Trying 192.168.2.93...
* Connected to 192.168.2.93 (192.168.2.93) port 8080 (#0)
* Initializing NSS with certpath: sql:/etc/pki/nssdb
*   CAfile: /etc/pki/tls/certs/ca-bundle.crt
  CApath: none
* Server certificate:
* 	subject: E=andy_ro@qq.com,CN=test.Api.cn,OU=Test,O=TXQP,L=Shenzhen,ST=Guangdong,C=CN
* 	start date: Feb 27 12:09:40 2020 GMT
* 	expire date: Feb 26 12:09:40 2021 GMT
* 	common name: test.Api.cn
* 	issuer: E=andy_ro@qq.com,CN=test.Api.cn,OU=Test,O=TXQP,L=Shenzhen,ST=Guangdong,C=CN
* NSS error -8172 (SEC_ERROR_UNTRUSTED_ISSUER)
* Peer's certificate issuer has been marked as not trusted by the user.
* Closing connection 0
curl: (60) Peer's certificate issuer has been marked as not trusted by the user.
More details here: http://curl.haxx.se/docs/sslcerts.html

curl performs SSL certificate verification by default, using a "bundle"
 of Certificate Authority (CA) public keys (CA certs). If the default
 bundle file isn't adequate, you can specify an alternate file
 using the --cacert option.
If this HTTPS server uses a certificate signed by a CA represented in
 the bundle, the certificate verification probably failed due to a
 problem with the certificate (it might be expired, or the name might
 not match the domain name in the URL).
If you'd like to turn off curl's verification of the certificate, use
 the -k (or --insecure) option.


