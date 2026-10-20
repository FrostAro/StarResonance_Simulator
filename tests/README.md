# 测试框架 API 参考

> 位置：`tests/TestHarness.h`（框架）、`tests/test_main.cpp`（运行器）
> 零依赖、单头文件、C++17、MSVC 与 MinGW 均可编译。

---

## 目录

1. [30 秒上手](#1-30-秒上手)
2. [公开 API](#2-公开-api)
   - [`TEST(标识符, "显示名")`](#21-test标识符-显示名)
   - [`CHECK_TRUE(expr)`](#22-check_trueexpr)
   - [`CHECK_FALSE(expr)`](#23-check_falseexpr)
   - [`CHECK_EQ(actual, expected)`](#24-check_eqactual-expected)
   - [`CHECK_NEAR(actual, expected, tol)`](#25-check_nearactual-expected-tol)
3. [内部 API（一般不用直接调）](#3-内部-api一般不用直接调)
4. [怎么跑](#4-怎么跑)
5. [加一个新测试](#5-加一个新测试)
6. [约定与限制](#6-约定与限制)
7. [常见错误](#7-常见错误)

---

## 1. 30 秒上手

```cpp
#include "TestHarness.h"

TEST(proficient_roundtrip, "精通换算往返")
{
    //       ↑ ASCII 标识符      ↑ 中文显示名
    // 断言写在这里
    CHECK_NEAR(实际值, 期望值, 1e-6);
}
```

> ⚠️ **两个参数不能少**：第一个必须是 ASCII 标识符（MinGW 的 GCC 8.1 不支持中文标识符），
> 第二个是字符串（中文随便写）。详见 [2.1 节](#21-test标识符-显示名)。

写完**不用改 `main`、不用改 CMake**，下次构建 + `ctest` 就会自动跑到。

---

## 2. 公开 API

### 2.1 `TEST(标识符, "显示名")`

**功能**：定义一个测试用例，并**自动登记**（`main()` 之前完成）。

```cpp
TEST(proficient_roundtrip, "精通换算往返")
{
    // 用例体
}
```

| 参数 | 类型 | 可否用中文 | 说明 |
|---|---|---|---|
| `标识符` | **ASCII 标识符** | ❌ **不行** | 参与生成函数名和静态变量名 |
| `显示名` | **字符串字面量** | ✅ 可以 | 只用于报告输出 |

#### ⚠️ 为什么必须写两个参数

这是被 **MinGW GCC 8.1** 逼出来的设计：

```cpp
TEST(精通换算往返)     // ❌ GCC 8.1 报错
```

```
error: stray '\344' in program
```

**原因**：GCC 从 **10.0** 才开始支持 UTF-8 标识符；你的 MinGW 是 **8.1.0**。

但中文放在**字符串**里完全没问题（GCC 8.1 只是把字节原样带走）：

```cpp
TEST(proficient_roundtrip, "精通换算往返")     // ✅ 两个工具链都能编
```

**于是**：标识符用 ASCII 保证双工具链能编，显示名用中文保证报告能读。

**展开后等价于**：

```cpp
static void proficient_roundtrip();                                       // ① 前置声明
static test::Registrar test_registrar_proficient_roundtrip(               // ② 静态对象 → 自动登记
    "精通换算往返", proficient_roundtrip);
static void proficient_roundtrip() { /* 用例体 */ }                        // ③ 函数定义
```

注意第二个参数**不需要 `#` 字符串化**——它本来就是字符串字面量。

#### 规则

| 项 | 说明 |
|---|---|
| 标识符必须合法 C++ 标识符**且全 ASCII** | ✅ `proficient_roundtrip`　❌ `精通换算往返` |
| 标识符可以带下划线和数字 | ✅ `check_attr_2` |
| 标识符不能以数字开头 | ❌ `2nd_check` |
| 同一 `.cpp` 内标识符不能重名 | 会重复定义 |
| 不同 `.cpp` 可以重名 | 宏内用 `static`（内部链接） |
| 显示名可以是任何字符串 | ✅ `"精通换算往返"`　✅ `"Engineer 的 50% 加成"` |
| 一个文件可以写几十个 `TEST` | ✅ |

**注意**：`TEST(...)` 后面**不要加分号**，直接跟 `{`。

```cpp
TEST(a, "甲") { }      // ✅
TEST(a, "甲"); { }     // ❌ 会在 `{` 处报语法错误
```

---

### 2.2 `CHECK_TRUE(expr)`

**功能**：断言 `expr` 为真。

```cpp
CHECK_TRUE(p != nullptr);
CHECK_TRUE(count > 0);
CHECK_TRUE(1 + 1 == 2);
```

**失败时输出**：

```
       [FAIL] tests\xxx.cpp:42
              期望为真: p != nullptr
```

`#expr` 把源码文本原样打印出来，所以你能看到**是哪条断言**失败的。

---

### 2.3 `CHECK_FALSE(expr)`

**功能**：断言 `expr` 为假。

```cpp
CHECK_FALSE(p == nullptr);
CHECK_FALSE(vector.empty());
```

输出格式同 `CHECK_TRUE`，只是文案变成 `期望为假: ...`。

---

### 2.4 `CHECK_EQ(actual, expected)`

**功能**：断言两者**精确相等**（用 `operator==`）。

```cpp
CHECK_EQ(2 + 2, 4);                          // int
CHECK_EQ(person->getCriticalCount(), 10792); // int
CHECK_EQ(Beam::name, std::string("Beam"));   // std::string
CHECK_EQ(flag, true);                        // bool
```

**失败时输出**：

```
       [FAIL] tests\xxx.cpp:43
              1 + 1 == 3 失败: 实际=2 期望=3
```

**适用类型**：

| 类型 | 支持 |
|---|---|
| `int` / `long` / `long long` / `unsigned` | ✅ |
| `double` / `float` | ⚠️ 见下方警告 |
| `bool` | ✅ 打印 `true`/`false` |
| `std::string` | ✅ |
| `const char*` | ✅（`nullptr` 打印为 `(null)`） |
| 其他自定义类型 | ⚠️ 能比较，但失败时值打印为 `?` |

> ⚠️ **浮点数不要用 `CHECK_EQ`**
> `0.1 + 0.2 == 0.3` 在双精度下是 **false**。浮点一律用 `CHECK_NEAR`。

**两个参数都会先求值一次再比较**，所以带副作用的表达式是安全的：

```cpp
CHECK_EQ(count++, 5);    // count++ 只执行一次
```

---

### 2.5 `CHECK_NEAR(actual, expected, tol)`

**功能**：断言 `|actual - expected| <= tol`（浮点近似相等）。

```cpp
CHECK_NEAR(0.1 + 0.2, 0.3, 1e-9);            // 通过
CHECK_NEAR(getProficient(), 0.84, 1e-6);     // 通过
CHECK_NEAR(reverse(0.478), 0.478, 1e-6);     // 通过
```

| 参数 | 类型 | 说明 |
|---|---|---|
| `actual` | 任意可转 `double` 的值 | 实际值 |
| `expected` | 任意可转 `double` 的值 | 期望值 |
| `tol` | 任意可转 `double` 的值 | **绝对容差**（不是相对百分比） |

**失败时输出**：

```
       [FAIL] tests\xxx.cpp:44
              0.5 ≈ 0.6 失败: 实际=0.500000 期望=0.600000 容差=0.010000
```

**容差怎么定**（重要）：

| 场景 | 建议容差 | 理由 |
|---|---|---|
| 纯数学往返 | `1e-9` | 双精度有效位约 15~17 位 |
| **经过 `int` 截断的换算** | **`1e-6`** | 截断误差量级 ≈ `(1−y)²/200000 ≈ 7e-7`，`1e-9` **永远做不到** |
| 总 DPS（大数值） | `±0.01%` 相对 | 用 `totalDps * 0.0001`，别写死绝对值 |
| 技能伤害占比 | `±0.5pp` | 百分点，写成 `0.005`（因为占比是 0~1 的小数） |

> **容差不是拍脑袋定的，要按"物理下限"推。** 定太严 → 跨编译器永远红；定太松 → 抓不到真实漂移。

**三个参数都只求值一次**，副作用安全。

---

## 3. 内部 API（一般不用直接调）

这些在 `namespace test` 里，`CHECK_*` 宏内部会用到。你写测试时**基本不会直接调**，但知道它们有助于理解。

### `test::registry()`

```cpp
std::vector<TestCase>& registry();
```

返回**全局唯一的测试登记表**。所有 `TEST` 都会往里塞一条。

`test_main.cpp` 的 `main` 就是遍历它。

### `test::failureCount()`

```cpp
int& failureCount();
```

返回**全局累计失败计数**的引用。返回引用是为了能直接 `++`。

**用途**：

- `reportFailure` 里 `++failureCount()`
- `test_main.cpp` 里做差，算出"这个用例自己错了几处"
- `main` 返回值靠它判断全绿与否

### `test::reportFailure(file, line, what)`

```cpp
void reportFailure(const char* file, int line, const std::string& what);
```

记一次失败：计数 +1，并打印到 `std::cout`。

**不会**抛异常，**不会**中断当前用例。

### `test::TestCase`

```cpp
struct TestCase
{
    const char* name;   // 用例名
    void (*fn)();       // 用例函数指针
};
```

登记表里的一条记录。`fn` 是**函数指针**。

### `test::Registrar`

```cpp
struct Registrar
{
    Registrar(const char* name, void (*fn)());   // 构造函数里自动登记
};
```

`TEST` 宏会生成一个静态 `Registrar`，它在 `main()` 之间构造 → 自动登记。

### `test::toStr(value)`

```cpp
template <typename T> std::string toStr(const T&);       // 兜底 → "?"

std::string toStr(int);
std::string toStr(long);
std::string toStr(long long);
std::string toStr(unsigned);
std::string toStr(unsigned long);
std::string toStr(double);
std::string toStr(float);
std::string toStr(bool);                                 // → "true"/"false"
std::string toStr(const std::string&);
std::string toStr(const char*);                          // nullptr → "(null)"
```

把值转成字符串，**仅用于失败信息**。

普通重载优先于模板，所以常见类型都能正确打印；其他类型落回 `"?"`。

---

## 4. 怎么跑

### 只编测试

```bash
cmake --build build-check --config Release --target dps_unit_tests dps_golden_tests
```

### 跑全部（ctest）

```bash
cd build-check
ctest -C Release --output-on-failure
```

`--output-on-failure` 很重要：**只有失败时才打印屏幕输出**，成功时安静。

### MinGW 双工具链（推荐每次机制改动都跑一遍）

MSVC 能过不代表 MinGW 能过——`WeaponConfig(args...)` 那次（`c829cd4`）就是 MSVC 过、GCC 断。

```bash
# 1) 首次：配置一个 MinGW 构建目录（GUI 会因 Qt 是 MSVC 版而自动跳过，属正常）
cmake -S . -B build-mingw -G "MinGW Makefiles" \
  -DCMAKE_MAKE_PROGRAM=C:/MinGW/mingw64/bin/mingw32-make.exe \
  -DCMAKE_CXX_COMPILER=C:/MinGW/mingw64/bin/g++.exe \
  -DCMAKE_C_COMPILER=C:/MinGW/mingw64/bin/gcc.exe

# 2) 构建 + 跑测试
cmake --build build-mingw --target dps_unit_tests dps_golden_tests -- -j8
./build-mingw/bin/dps_unit_tests.exe
./build-mingw/bin/dps_golden_tests.exe
```

> `build-mingw` / `build-check` 都被 `.gitignore` 里的 `build-*/` 忽略，不进版本库。

### 直接跑某个 exe（想看完整输出时）

```bash
build-check\bin\Release\dps_unit_tests.exe
echo %ERRORLEVEL%        :: Windows cmd 查退出码
```

```powershell
.\build-check\bin\Release\dps_unit_tests.exe
$LASTEXITCODE            # PowerShell 查退出码
```

### 两个测试目标的分工

| 目标 | 文件 | 跑什么 | 耗时 |
|---|---|---|---|
| `dps_unit_tests` | `tests/test_attributes.cpp` | 属性换算、边界、守恒等**不变量** | 毫秒级 |
| `dps_golden_tests` | `tests/test_golden_beam.cpp` | 完整 180 秒模拟，对比**钉值** | 十几秒 |

**为什么拆两个 exe**：项里有大量进程级 `static`（`TypedAction<...>::listeners`、`Buff::ID`、`AutoAttack::timer`），拆进程是最彻底的隔离。

---

## 5. 加一个新测试

### 第 1 步：在对应 `.cpp` 里写

```cpp
// tests/test_attributes.cpp
#include "TestHarness.h"
#include "core/Person.h"

TEST(proficient_count_roundtrip, "精通点数往返")
{
    // 准备：造一个 Person（示例，实际参数按需要填）
    // ...
    const double y = 0.478;
    const int count = p->getProficientCount(y);
    const double back = static_cast<double>(count) / (count + 200000.0);

    CHECK_NEAR(back, y, 1e-6);   // ← 不能写 1e-9，int 截断误差约 7e-7
}
```

**标识符（第一个参数）必须全 ASCII**，中文只能放第二个参数。

### 第 2 步：直接跑

```bash
cmake --build build-check --config Release --target dps_unit_tests
cd build-check && ctest -C Release --output-on-failure
```

**不用改 `main`，不用改 CMake。**

### 如果测试要用到新的 `.cpp` 文件

才需要改 `CMakeLists.txt`：

```cmake
add_executable(dps_unit_tests
    tests/test_attributes.cpp
    tests/test_new_file.cpp      # ← 加这行
    tests/test_main.cpp
    ${BEAM_SOURCES}
)
```

---

## 6. 约定与限制

### ✅ 该做的

| 做法 | 原因 |
|---|---|
| 一个 `TEST` 只测一件事 | 失败时一眼知道哪里坏了 |
| 用例名写"测什么现象" | `proficient_count_roundtrip` / `"精通点数往返"`，比 `test1` 有用一百倍 |
| **标识符用 ASCII，中文放显示名** | GCC 8.1 不支持中文标识符 |
| 容差按物理下限推 | 见 2.5 节 |
| 断言失败信息里包含**实际值** | 宏已自动带上 |
| 需要多个断言就写多个 | 失败不中断，一次全看到 |

### ❌ 不该做的

| 反模式 | 问题 |
|---|---|
| 在测试里 `std::cout` 打印调试信息 | 会混进测试输出；失败信息已经不缺上下文 |
| `TEST` 后面加分号 | 展开会出错 |
| 浮点用 `CHECK_EQ` | 几乎必然失败 |
| 用 `assert()` 代替 `CHECK_*` | `assert` 在 Release（`NDEBUG`）下**会被完全移除**，测试静默失效 |
| 在 `TEST` 里改全局静态状态而不恢复 | 会影响后续用例（同一进程内顺序执行） |
| 依赖用例执行顺序 | 顺序由链接顺序决定，不可靠 |

### 关于"失败不中断"

```cpp
TEST(some_case, "某测试")
{
    CHECK_EQ(a, 1);    // 错 → 记 1 笔，继续
    CHECK_EQ(b, 2);    // 错 → 记 1 笔，继续
    CHECK_EQ(c, 3);    // 对
}
```

**一次能看到所有问题。** 但这也意味着：**如果某个断言失败会导致后面的代码崩溃（比如空指针解引用），测试进程会直接挂掉**，后面的断言就没机会跑了。

所以：**会崩的操作前后，先用 `CHECK_TRUE(ptr != nullptr)` 挡一下。**

---

## 7. 常见错误

### `TEST 后面加分号`

```cpp
TEST(some_name, "名字");   // ❌
{
}
```
报错形如 `语法错误: 意外的标记';'`。

### 用例名写成中文标识符

```cpp
TEST(精通换算)              // ❌ MinGW GCC 8.1: stray '\344' in program
TEST(proficient_roundtrip, "精通换算")   // ✅
```

MSVC 能过、GCC 8.1 不能过——**这就是双工具链必须都跑的原因**。

### 只写一个参数

```cpp
TEST(proficient_roundtrip)   // ❌ 宏要两个参数
```
报错形如 `宏 "TEST" 需要 2 个参数，但只提供了 1 个`。

### 忘记 include 被测的头文件

```cpp
TEST(some_case, "某人")
{
    CHECK_EQ(Beam::name, ...);   // ❌ 没 include "Mage/Beam/Skill.h"
}
```
报错：`"Beam": 未声明的标识符`。

### `CHECK_NEAR` 容差写成百分比

```cpp
CHECK_NEAR(0.84, 0.85, 0.01);      // ✅ 0.01 是绝对差，通过
CHECK_NEAR(0.84, 0.85, 1.0);       // ⚠️ 差 1.0 也能过，等于没断言
CHECK_NEAR(0.84, 0.85, 1e-9);      // ❌ 浮点误差就超了
```

### 测试"永远绿"

最常见的原因：**断言根本没跑**。

排查手段：

```cpp
TEST(self_check, "自检")
{
    CHECK_EQ(1 + 1, 3);    // ← 故意写错，如果它还是绿，说明套件是空的
}
```

这叫**"测试的测试"（meta-test）**：故意弄坏，看测试会不会叫。
**没做过这一步的测试套件不算存在。**

### 退出码恒为 0

`main` 里的这行不能写错：

```cpp
return test::failureCount() == 0 ? 0 : 1;
```

`ctest` **只看退出码**。写成 `return 0;` → 永远 Pass。

---

## 附：完整最小示例

```cpp
// tests/test_attributes.cpp
#include "TestHarness.h"
#include "core/Person.h"

// 正好相等的用 CHECK_EQ
TEST(count_pin_values, "点数钉值")
{
    CHECK_EQ(p->getCriticalCount(),   10792);
    CHECK_EQ(p->getQuicknessCount(),  94898);
    CHECK_EQ(p->getProficientCount(), 337634);
}

// 浮点的用 CHECK_NEAR
TEST(count_percent_roundtrip, "换算往返")
{
    for (double y : {0.0, 0.1, 0.478, 0.628, 0.9})
    {
        const int c = p->getProficientCount(y);
        const double back = static_cast<double>(c) / (c + 200000.0);
        CHECK_NEAR(back, y, 1e-6);
    }
}

// 布尔条件用 CHECK_TRUE / CHECK_FALSE
TEST(no_negative_count_on_low_panel, "低面板不产生负点数")
{
    // ... 构造低面板 Person
    CHECK_TRUE(p->getCriticalCount()   >= 0);
    CHECK_TRUE(p->getQuicknessCount()  >= 0);
    CHECK_TRUE(p->getProficientCount() >= 0);
}
```

---

**相关文件**

| 文件 | 作用 |
|---|---|
| `tests/TestHarness.h` | 框架本体（宏 + 内部函数） |
| `tests/test_main.cpp` | 运行器（`main`，两个测试程序共用） |
| `tests/test_attributes.cpp` | 数值单元测试 |
| `tests/test_golden_beam.cpp` | 黄金基线测试 |
| `CMakeLists.txt` 第 6 节 | `enable_testing()` + 两个测试目标 + `add_test` |
