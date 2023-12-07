# 微信内网页打开APP

> 在微信内推广移动应用时非常有用

## 前置条件

1. 微信开放平台需要注册移动应用
2. 微信开发平台需要关联认证微信服务号
3. 微信服务号需要关联移动应用的APPID
4. 服务号JS接口安全域名需要配置为h5网页的域名
5. 服务号需要在微信公众平台配置业务域名、JS接口安全域名
6. 移动应用需要接入微信的OPENSDK
7. 公众号安全中心需要配置IP白名单
8. 微信开放平台服务号关联里面要配置js接口安全域名

## 依赖库

1. weixin-js-sdk
2. 微信开发平台标签wx-open-launch-app
3. OPENSDK

## 对接流程

### 网页内对接weixin-js-sdk

#### 引入wx-open-launch-app

1. 在需要的网页内引入标签wx-open-launch-app
2. 填写appid
3. extinfo是微信开发标签传递给app的数据
4. vue3要用component主键包裹div并且设置:is="'script'" type="text/wxtag-template"
5. @error是无法拉起时的回调方法

``` vue
<wx-open-launch-app
          id="wx-launch-btn"
          class="launch-btn"
          style="width:100vw; position:fixed; bottom:70px; z-index: 99999999"
          :appid="'wxa143e530a03978fd'"
          :extinfo="ext"
          @error="onWakeError"
      >
        <component :is="'script'" type="text/wxtag-template">
          <div style="width: 80%;
            text-align: center;
            font-size: 14px;
            display: block;
            color: rgb(255, 255, 255);
            background: #1989fa;
            margin: auto;
            line-height: 45px;
            border-radius: 8px;">打开APP</div>
        </component>
      </wx-open-launch-app>
```


#### 实例化weixin-js-sdk

```javascript
wx.config({
  debug: true, // 开启调试模式,调用的所有api的返回值会在客户端alert出来，若要查看传入的参数，可以在pc端打开，参数信息会通过log打出，仅在pc端时才会打印。
  appId: '', // 必填，公众号的唯一标识
  timestamp: , // 必填，生成签名的时间戳
  nonceStr: '', // 必填，生成签名的随机串
  signature: '',// 必填，签名
  jsApiList: [] // 必填，需要使用的JS接口列表
});
```

_里面的json对象需要从服务端签名_

服务端签名时需要配置IP白名单

微信jssdk签名可以，由第三方服务器代签
只需要在微信公众平台白名单IP里加入服务端ip