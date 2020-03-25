//https://blog.csdn.net/weixin_42534940/article/details/90745452
//https://blog.csdn.net/weixin_42534940/article/details/90745452#3.%20%E8%AF%81%E4%B9%A6%E7%BD%91%E7%AB%99%E7%94%9F%E6%88%90%E6%96%B0%E8%AF%81%E4%B9%A6

mkdir certificate
cd certificate
mkdir -p CA/private
#第一步 生成私钥文件
sudo openssl genrsa -out /etc/pki/CA/private/cakey.pem 2048
#第二步 生成自签证书 -key 私钥文件 -out 证书存放位置 -new 生成新证书签署请求 -days n 证书有效天数 -x509 生成自签证书
//CN Guangdong Shenzhen MUDUO Test test.Api.cn andy_ro@qq.com
sudo openssl req -new -x509 -key /etc/pki/CA/private/cakey.pem -out /etc/pki/CA/cacert.pem -days 365
#第三步 生成私钥
sudo openssl genrsa -out /etc/pki/CA/private/certificate.key 2048
#第四步 生成请求签署文件
//CN Guangdong Shenzhen MUDUO Test www.testApi.com andy_ro@qq.com
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
* 	subject: E=andy_ro@qq.com,CN=test.Api.cn,OU=Test,O=MUDUO,L=Shenzhen,ST=Guangdong,C=CN
* 	start date: Feb 27 12:09:40 2020 GMT
* 	expire date: Feb 26 12:09:40 2021 GMT
* 	common name: test.Api.cn
* 	issuer: E=andy_ro@qq.com,CN=test.Api.cn,OU=Test,O=MUDUO,L=Shenzhen,ST=Guangdong,C=CN
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

//nginx cdn加速和反向代理
//https://blog.csdn.net/zdp072/article/details/51069331

其实涉及到前端H5网站访问才需要做CDN流量分发减轻单个H5服务器访问的负载压力
其他的网关ProxyServer 和 ApiServer 完全没有必要，
设计出来本来就是带HA负载均衡支持和高并发特征的。
加了nginx 之后层层代理转发，这个效率根本就上不来。（不知道是配置节点问题还是其他原因）
这个问题解决了我相信现在线上的各种问题都可以解决!

ProxyServer 只要加上wss支持及协议数据加密，内部也做了负载均衡，HA都可以不用。
ApiServer 已经新增了SSL认证功能，已经支持HTTPS，只需要加上Api网关就可负载均衡，
单个节点都已经完全满足现有业务支撑。

因为H5前端要加载很多页面数据没法做到流量分发及负载均衡功能，
所以需要加上CDN转发分流减轻H5服务器压力，
ProxyServer 和 ApiServer 本来就应该挂载公网环境！
现在线上那套环境需要整改

游戏服务器本身通信就是基于TCP数据流的
哪家游戏公司的游戏服务器用nginx 这种web页面分流的

只有H5/IIS或者web页面请求才会用nginx 或者tomcat 这样的做代理分发的。
本来DNS 域名绑定多个proxy IP ,proxyServer 做内部负载均衡就搞定的事情。
不然为什么自己内部还要搞ProxyServer,ProxyServer 本来就是挂到公网跑的
游戏服务器都是挂公网
就是用nginx或者HA 反向代理，也只是返回ProxyServer 或者ApiServer的真实IP给apk或者H5前端，
然后客户端和ProxyServer或者ApiServer直连通信，游戏服务器TCP数据流走Nginx 就是扯淡

proxyServer 关于wss 后面再做支持，目前不支持其实也没关系
只要在LoginServer 安全认证，做好安全防御就可以了。
目前LoginServer 其实和ApiServer是一样的，
ApiServer已经做好了Https支持, LoginServer改一下就可以了。

即使被别人恶意攻击，只要在LoginServer 安全认证，不被劫持或者破解。人家是那不到你的proxyServer domain/ip port 的。

HA 和 nginx 对于游戏服务器来说完全可以忽略，
只是IIS 或者前端H5 这块加上CDN 就可以，
防护再登陆那块做好SSL认证。apk 或者 H5 被破解也无所谓。
所以部署也要按照游戏服务器架构来做，而不是按照web网站部署那套来搞。
H5页面涉及到很多图片动画资源加载，单节点又没法做到分流和负载，所以加cdn 或nginx 没有问题
游戏服务器就不行。

* CDN情况一：客户端与CDN之间1w个连接，那么对等的，CDN作为[C]端与ProxyServer作为[S]端之间也是1w个连接，没问题
* -------------------------------------------------------------------------------------------------------------
[C]端           [S]端  [C]端                     [S]端
C1                              C1'          
C2                              C2'             
...       =>        CDN         ...       =>    ProxyServer
C9999                           C9999'
C10000                          C10000'
                              (通道对等)

* CDN情况二：客户端与CDN之间1w个连接，CDN作为[C]端与ProxyServer作为[S]端之间1个连接，存在瓶颈
* -------------------------------------------------------------------------------------------------------------
[C]端           [S]端  [C]端                     [S]端
C1                                         
C2                                            
...      =>         CDN         C1'       =>    ProxyServer
C9999                         (单通道瓶颈)
C10000                           


* CDN情况三：客户端与CDN之间1w个连接，CDN作为[S]端与ProxyServer作为[C]端之间1个连接，存在瓶颈
* -------------------------------------------------------------------------------------------------------------
[C]端           [S]端  [S]端                      [C]端
C1                                         
C2                                            
...      =>         CDN      <=     C1'         ProxyServer
C9999                           (单通道瓶颈)
C10000                           





















