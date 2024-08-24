> 背景介绍：CAS （ Central Authentication Service ） 是 Yale 大学发起的一个企业级的、开源的项目，旨在为 Web 应用系统提供一种可靠的单点登录解决方法（属于 Web SSO ）。
CAS 开始于 2001 年， 并在 2004 年 12 月正式成为 JA-SIG 的一个项目。

> 单点登录（ Single Sign-On , 简称 SSO ）是目前比较流行的服务于企业业务整合的解决方案之一， SSO 使得在多个应用系统中，用户只需要 登录一次 就可以访问所有相互信任的应用系统(一次访问，到处运行)。

> 和现在的三方登录比如微信登录不是一个概念，微信等称为三方授权登录，区别是用户是第三方(微信)过来的。而CAS是用户属于你的，同时登录的产品也是你自己能控制的, 所以是一次登录，相当于所有应用都登录了，而微信登录还有一个授权过程。当然CAS也可以作为别人的三方登录提供商。比如应用内集成微信登录也是访问了微信的CAS，涉及三方就强调授权了。


## 本文用docker方式同时搭建CAS服务端和CAS服务管理端(cas-management)

> 管理端是用于注册CAS服务的，CAS服务是提供给需要单点登录的客户端认证用的

### 参考：

1. https://github.com/apereo/cas
2. https://apereo.github.io/cas/6.6.x/
3. https://apereo.github.io/cas/6.6.x/installation/Docker-Installation.html
4. https://hub.docker.com/r/apereo/cas/
5. https://github.com/apereo/cas-management-overlay

## 搭建CAS:

#### 运行容器

```shell
docker pull apereo/cas:6.6.15.1

docker run --rm -e SERVER_SSL_ENABLED=false -e SERVER_PORT=8080 -p 8080:8080 --name casserver apereo/cas:6.6.15.2
```

#### 运行成功后修改配置文件

```shell

docker exec -it 容器id bash

vim /etc/cas/config/cas.properties

# 配置内容
cas.service-registry.core.init-from-json=true # 启用JSON注册服务
cas.serviceRegistry.json.location=file:/etc/cas/services #JSON配置文件目录
cas.authn.oauth.grants.resourceOwner.requireServiceHeader=true
cas.authn.policy.requiredHandlerAuthenticationPolicyEnabled=false

vim /etc/cas/services/web-10000001.json

{
  "@class" : "org.apereo.cas.services.RegexRegisteredService",
  "serviceId" : "^(https|imaps|http)://.*",
  "name" : "web",
  "id" : 10000001,
  "evaluationOrder" : 10
}

docker restart cas容器ID
```

**完成这些配置后重启容器cas就配置好了，访问 http://192.168.31.39:8080/cas/login 进入单点登录界面 默认账号密码casuser/Mellon**

![alt text](image-1.png)

## 搭建管理端

> 管理端也是一个CAS的客户端也遵循单点登录

### 前置需求

1. linux系统
2. git需要安装
3. JDK11+

### 构建WAR包

```shell

git clone https://github.com/apereo/cas-management-overlay.git

cd cas-management-overlay && git checkout 6.6

vim docker-run.sh

# 脚本内容
# 如果在同一机器上面需要注意，这里开放的8080和8443和cas端口是一样的，你需要改端口，这里我改为8081

#!/bin/bash
docker stop cas-management > /dev/null 2>&1
docker rm cas-management > /dev/null 2>&1
image_tag=(`cat gradle.properties | grep "casmgmt.version" | cut -d= -f2`)
docker run -d -p 8081:8080 -p 8443:8443 --name="cas-management" apereo/cas-management:"v${image_tag}"
docker logs -f cas-management


vim etc/cas/config/management.properties

# 配置内容
cas.server.name=http://192.168.31.39:8080 #这里是cas服务地址，这里可以用https，前提是cas支持https
cas.server.prefix=${cas.server.name}/cas
mgmt.server-name=https://192.168.31.39:8443 #这里是管理端的访问地址
mgmt.admin-roles[0]=ROLE_ADMIN
mgmt.user-properties-file=file:/etc/cas/config/users.json
logging.config=file:/etc/cas/config/log4j2-management.xml

# 生成一个名为thekeystore的证书

#!/bin/bash

if [[ -z "${CAS_KEYSTORE}" ]] ; then
  keystore="$PWD"/thekeystore
  echo -e "Generating keystore for CAS Server at ${keystore}"
  dname="${dname:-CN=localhost,OU=Example,OU=Org,C=US}"
  subjectAltName="${subjectAltName:-dns:example.org,dns:localhost,ip:127.0.0.1}"
  [ -f "${keystore}" ] && rm "${keystore}"
  keytool -genkey -noprompt -alias cas -keyalg RSA \
    -keypass changeit -storepass changeit \
    -keystore "${keystore}" -dname "${dname}"
  [ -f "${keystore}" ] && echo "Created ${keystore}"
  export CAS_KEYSTORE="${keystore}"
else
  echo -e "Found existing CAS keystore at ${CAS_KEYSTORE}"
fi

# 生成好之后将证书复制到 etc/cas目录
cp thekeystore etc/cas/thekeystore
```

**管理端的配置就好了**

### 脚本加执行权限

```shell
chmod +x *.sh
```

### 构建容器环境

```shell
./docker-build.sh
```

### 运行容器

```shell
./docker-run.sh
```

**访问 https://192.168.31.39:8443/cas-management 进入管理界面**

![alt text](image.png)