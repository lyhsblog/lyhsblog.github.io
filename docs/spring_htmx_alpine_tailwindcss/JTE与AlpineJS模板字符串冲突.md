# JTE 与 Alpine.js 模板字符串冲突及解决办法

在 **Spring Boot + JTE** 与 **Alpine.js** 同页混用时，JTE 的 **`${...}`** 与浏览器里 **反引号模板字符串**中的 **`${...}`** 会争用同一套写法；若把 Alpine 定义和复杂字符串直接散写在模板里，容易出现**解析顺序错误**或**输出不符合预期**。

下面先给出推荐写法：**用 `Alpine.data` 定义组件、用 `@raw` 包住 Alpine 脚本、用函数参数把 JTE 变量送进 Alpine**；再说明冲突原因与其它补救手段。

---

## 推荐：`Alpine.data` + `@raw` + JS 参数传递

思路分三步：

1. **`Alpine.data`**：在页面里注册可复用的 Alpine 组件工厂，由 Alpine 在浏览器侧解析，与 JTE 的插值职责分离。
2. **`@raw` … `@endraw`**：包住 **Alpine 组件定义所在的 `<script>`**，让 JTE **不要**对这段内容做额外解析/转义，避免与模板引擎规则打架。
3. **`x-data` 调用工厂并传参**：在 HTML 属性里用 **普通 JavaScript 对象字面量**把服务端数据以 **函数实参**形式传入，例如 `userComponent({ name: '...', age: ... })`。字符串用 JTE 输出到 **引号内**（如 `'${user.name}'`），数字可直接 `${user.age}`。

示例（`.jte` 中）：

```html
<div x-data="userComponent({ name: '${user.name}', age: ${user.age} })">
</div>

@raw
<script>
document.addEventListener('alpine:init', () => {
    Alpine.data('userComponent', (props) => ({
        ...props
    }))
})
</script>
@endraw
```

说明：

- **`userComponent`**：在 `alpine:init` 里通过 `Alpine.data` 注册，`(props) => ({ ...props })` 把传入的对象展开成 Alpine 组件的 **data**。
- **`@raw` / `@endraw`**：保证 `<script>` 内的 `Alpine.data`、`=>`、`...` 等 **按原样输出**，不被 JTE 误处理。
- **JTE → Alpine**：`name` 走 **字符串插值**（注意外层已是 JS 单引号，由 JTE 填入 `user.name`）；`age` 走 **数值插值** `${user.age}`，输出为合法数字。这样 **不需要**在 Alpine 侧再和 JTE 抢反引号模板字符串里的 `${}`。

若 `user.name` 可能含引号或来自不可信输入，需在服务端做 **HTML/JS 上下文转义**，避免破坏属性或产生 XSS；生产环境请按项目规范处理。

---

## 冲突原因简述（为何还要关心 `${}`）

| 场景 | 含义 |
|------|------|
| **JTE** | `${expression}` 在服务端渲染时求值并输出 |
| **Alpine / 浏览器 JS** | 反引号 `` `...` `` 模板字符串中的 `${expr}` 由浏览器解析 |

若在内联脚本或属性里混用 **反引号模板字符串**，JTE 仍会先处理其中的 `${...}`，与「留给浏览器」的意图不一致。采用上一节的 **`Alpine.data` + `@raw` + 对象参数** 后，**数据走函数参数与 JTE 插值**，通常可**避免**在内联 Alpine 里写反引号模板字符串，从结构上消除冲突。

---

## 其它补救手段（按需）

### 1. 在 JTE 中转义 `$`

若必须在模板中输出字面量 `${` 给浏览器，可将 `$` 写成 `\$`（以当前 JTE 版本文档为准），使 JTE 不当作自身插值。

### 2. 少用浏览器模板字符串

能用 **`'Hello ' + name`** 或 Alpine 表达式时，避免在 `.jte` 里写 `` `...${}...` ``。

### 3. 独立 `.js` 静态资源

把 Alpine 注册逻辑放在 **不经 JTE 解析的 `.js` 文件**中；页面仍可用 `x-data="userComponent({ ... })"` 由 JTE 填参数。

### 4. `x-html`（谨慎）

仅在对内容可信且做好防护时使用，一般优先 **参数传递** 与 **`@raw` 脚本**。

---

## 实践建议

1. **优先**：`Alpine.data` 定义行为，`@raw` 包住脚本，**JTE 变量通过 `x-data` 里函数调用的实参**传入。
2. **分清**：属性里哪一段是 **JTE 插值**（`${user.xxx}`），哪一段是 **固定输出给浏览器的 JS**。
3. **HTMX**：若属性里需要字面量 `$`，同样注意 JTE 是否抢先解析；组件化后问题多集中在 **模板与内联脚本**边界。

按上述方式可在 **Spring + JTE + HTMX + Alpine.js + Tailwind CSS** 中稳定混用服务端渲染与 Alpine 状态。
