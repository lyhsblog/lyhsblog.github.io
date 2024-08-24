"use strict";(self.webpackChunkliyhblog=self.webpackChunkliyhblog||[]).push([[5472],{4842:(n,e,s)=>{s.r(e),s.d(e,{assets:()=>c,contentTitle:()=>r,default:()=>h,frontMatter:()=>t,metadata:()=>d,toc:()=>o});var i=s(4848),l=s(8453);const t={id:"install-sublinux",title:"\u5b89\u88c5SubLinux"},r=void 0,d={id:"windows/install-sublinux",title:"\u5b89\u88c5SubLinux",description:"\u7279\u6027\u914d\u7f6e",source:"@site/docs/windows/windows10\u5b89\u88c5SubLinux.md",sourceDirName:"windows",slug:"/windows/install-sublinux",permalink:"/en/docs/windows/install-sublinux",draft:!1,unlisted:!1,editUrl:"https://github.com/facebook/docusaurus/tree/main/packages/create-docusaurus/templates/shared/docs/windows/windows10\u5b89\u88c5SubLinux.md",tags:[],version:"current",frontMatter:{id:"install-sublinux",title:"\u5b89\u88c5SubLinux"},sidebar:"tutorialSidebar",previous:{title:"\u914d\u7f6e",permalink:"/en/docs/uni\u6280\u672f\u6808/\u914d\u7f6e"},next:{title:"\u8f6f\u94fe\u63a5\u64cd\u4f5c",permalink:"/en/docs/windows/\u8f6f\u94fe\u63a5\u64cd\u4f5c"}},c={},o=[{value:"\u7279\u6027\u914d\u7f6e",id:"\u7279\u6027\u914d\u7f6e",level:2},{value:"\u4f7f\u7528\u547d\u4ee4\u884c\u5b89\u88c5WSL",id:"\u4f7f\u7528\u547d\u4ee4\u884c\u5b89\u88c5wsl",level:2},{value:"\u5b89\u88c5docker",id:"\u5b89\u88c5docker",level:2},{value:"\u5b89\u88c5\u6210\u529f\u540e\u5c31\u53ef\u4ee5\u4eab\u53d7\u5bb9\u5668\u5e26\u6765\u7684\u4fbf\u5229\u4e86",id:"\u5b89\u88c5\u6210\u529f\u540e\u5c31\u53ef\u4ee5\u4eab\u53d7\u5bb9\u5668\u5e26\u6765\u7684\u4fbf\u5229\u4e86",level:3}];function a(n){const e={code:"code",h2:"h2",h3:"h3",img:"img",li:"li",ol:"ol",p:"p",pre:"pre",...(0,l.R)(),...n.components};return(0,i.jsxs)(i.Fragment,{children:[(0,i.jsx)(e.h2,{id:"\u7279\u6027\u914d\u7f6e",children:"\u7279\u6027\u914d\u7f6e"}),"\n",(0,i.jsxs)(e.ol,{children:["\n",(0,i.jsx)(e.li,{children:"\u6253\u5f00\u8bbe\u7f6e"}),"\n",(0,i.jsx)(e.li,{children:"\u627e\u5230\u5e94\u7528\u548c\u7279\u6027"}),"\n",(0,i.jsx)(e.li,{children:"\u5f00\u542fwindows\u529f\u80fd\u4e0e\u7279\u6027"}),"\n",(0,i.jsx)(e.li,{children:"\u9009\u4e2d\u865a\u62df\u673a\u5e73\u53f0\u548clinux\u5b50\u7cfb\u7edf"}),"\n",(0,i.jsx)(e.li,{children:"\u9700\u8981\u5728bios\u91cc\u9762\u6253\u5f00VT-X CPU\u865a\u62df\u5316\u6280\u672f"}),"\n"]}),"\n",(0,i.jsx)(e.p,{children:(0,i.jsx)(e.img,{alt:"\u5f00\u542f\u7279\u6027\u529f\u80fd",src:s(5056).A+"",width:"1920",height:"1030"})}),"\n",(0,i.jsx)(e.h2,{id:"\u4f7f\u7528\u547d\u4ee4\u884c\u5b89\u88c5wsl",children:"\u4f7f\u7528\u547d\u4ee4\u884c\u5b89\u88c5WSL"}),"\n",(0,i.jsxs)(e.ol,{children:["\n",(0,i.jsx)(e.li,{children:"wsl\u672c\u8eab\u96c6\u6210\u5728\u4e86windows\u4e0a\u9762"}),"\n",(0,i.jsx)(e.li,{children:"wsl\u6709\u7248\u672c\u5206wsl1\u548cwsl2"}),"\n",(0,i.jsx)(e.li,{children:"\u6709\u7684windows\u7248\u672c\u4e0a\u9762\u9ed8\u8ba4\u542f\u52a8\u7684\u662fwsl1"}),"\n"]}),"\n",(0,i.jsx)(e.pre,{children:(0,i.jsx)(e.code,{className:"language-shell",children:"#win\u952e+r \u8f93\u5165\u5e76\u8fd0\u884ccmd\r\n#\u68c0\u67e5wsl\u7248\u672c\r\nwsl --version\r\n#\u5347\u7ea7wsl\r\nwsl --update\r\n#\u67e5\u8be2\u652f\u6301\u7684linux\u53d1\u884c\u7248\u672c\r\nwsl --list --online\r\n#\u5b89\u88c5\u4e00\u4e2a\u5b50\u7cfb\u7edf\r\nwsl --install Debian\r\n#\u67e5\u8be2\u5df2\u5b89\u88c5\u7684\u5b50\u7cfb\u7edf\r\nwsl --list\r\n#\u8fd0\u884c\u5b50\u7cfb\u7edf\r\nwsl #\u4f1a\u8fdb\u5165\u9ed8\u8ba4\u7684\u5b50\u7cfb\u7edf\n"})}),"\n",(0,i.jsx)(e.p,{children:(0,i.jsx)(e.img,{alt:"\u547d\u4ee4",src:s(7891).A+"",width:"1920",height:"1039"})}),"\n",(0,i.jsx)(e.h2,{id:"\u5b89\u88c5docker",children:"\u5b89\u88c5docker"}),"\n",(0,i.jsxs)(e.ol,{children:["\n",(0,i.jsx)(e.li,{children:"\u4e0b\u8f7dDocker Desktop Installer.exe"}),"\n",(0,i.jsx)(e.li,{children:"\u5b89\u88c5\u5e76\u8fd0\u884c"}),"\n",(0,i.jsx)(e.li,{children:"\u8bbe\u7f6ewindows\u5b50\u7cfb\u7edf\u4f5c\u4e3adocker\u5e94\u7528\u7684\u540e\u7aef"}),"\n"]}),"\n",(0,i.jsx)(e.p,{children:(0,i.jsx)(e.img,{alt:"\u8bbe\u7f6e\u540e\u7aef",src:s(6290).A+"",width:"1590",height:"900"})}),"\n",(0,i.jsxs)(e.ol,{start:"4",children:["\n",(0,i.jsx)(e.li,{children:"\u914d\u7f6e\u54ea\u4e9b\u5b50\u7cfb\u7edf\u5185\u90e8\u53ef\u4ee5\u8bbf\u95eedocker\u5e94\u7528"}),"\n"]}),"\n",(0,i.jsx)(e.p,{children:(0,i.jsx)(e.img,{alt:"\u5141\u8bb8\u8bbf\u95ee",src:s(8671).A+"",width:"1590",height:"900"})}),"\n",(0,i.jsxs)(e.ol,{start:"5",children:["\n",(0,i.jsx)(e.li,{children:"\u914d\u7f6e\u5b8c\u6210\u540e\u91cd\u542f\u4e00\u4e0bdocker\u5e94\u7528"}),"\n"]}),"\n",(0,i.jsx)(e.p,{children:(0,i.jsx)(e.img,{alt:"\u91cd\u542f",src:s(1777).A+"",width:"1590",height:"900"})}),"\n",(0,i.jsx)(e.h3,{id:"\u5b89\u88c5\u6210\u529f\u540e\u5c31\u53ef\u4ee5\u4eab\u53d7\u5bb9\u5668\u5e26\u6765\u7684\u4fbf\u5229\u4e86",children:"\u5b89\u88c5\u6210\u529f\u540e\u5c31\u53ef\u4ee5\u4eab\u53d7\u5bb9\u5668\u5e26\u6765\u7684\u4fbf\u5229\u4e86"}),"\n",(0,i.jsx)(e.p,{children:(0,i.jsx)(e.img,{alt:"\u6210\u529f\u56fe",src:s(3732).A+"",width:"1590",height:"900"})})]})}function h(n={}){const{wrapper:e}={...(0,l.R)(),...n.components};return e?(0,i.jsx)(e,{...n,children:(0,i.jsx)(a,{...n})}):a(n)}},5056:(n,e,s)=>{s.d(e,{A:()=>i});const i=s.p+"assets/images/1-3e5f5d9e2a2004e43ff67b26bd6f45ca.jpg"},8671:(n,e,s)=>{s.d(e,{A:()=>i});const i=s.p+"assets/images/2-43bd4192b42cdbcc46ba331b63f7fca0.jpg"},6290:(n,e,s)=>{s.d(e,{A:()=>i});const i=s.p+"assets/images/3-b52d7a375f14aa50679385a3859c2796.jpg"},1777:(n,e,s)=>{s.d(e,{A:()=>i});const i=s.p+"assets/images/4-9a5e3f53ce7d6853a31a7b2325c2e385.jpg"},3732:(n,e,s)=>{s.d(e,{A:()=>i});const i=s.p+"assets/images/5-6ee31f4acf27823720b4c1f3eb87cd5a.jpg"},7891:(n,e,s)=>{s.d(e,{A:()=>i});const i=s.p+"assets/images/6-48f617e26fc53c4e9aeb2048d20880b2.jpg"},8453:(n,e,s)=>{s.d(e,{R:()=>r,x:()=>d});var i=s(6540);const l={},t=i.createContext(l);function r(n){const e=i.useContext(t);return i.useMemo((function(){return"function"==typeof n?n(e):{...e,...n}}),[e,n])}function d(n){let e;return e=n.disableParentContext?"function"==typeof n.components?n.components(l):n.components||l:r(n.components),i.createElement(t.Provider,{value:e},n.children)}}}]);