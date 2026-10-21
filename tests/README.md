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
   - [两个测试目标的分工](#两个测试目标的分工)
   - [两套测试抓的东西不一样](#核心概念两套测试抓的东西不一样)
   - [黄金基线的口径](#黄金基线的口径很容易搞错)
   - [钉值优先级：整数计数 &gt; 浮点 DPS](#钉值优先级整数计数--浮点-dps)
   - [金丝雀：为什么必须顺序跑](#金丝雀为什么必须顺序跑)
   - [重钉（re-pin）流程](#重钉re-pin流程)
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
              0.5 ≈ 0.6 失败: 实际=0.5 期望=0.6 容差=0.01
```

> 浮点值用 `std::numeric_limits<double>::max_digits10`（=17 位有效数字）打印，
> 保证「打出来的数读回去 == 原值」。所以只差 1e-9 的两个值也能看出区别 ——
> 这对金丝雀（逐位比较）尤其重要。

**容差怎么定**（重要）：

| 场景 | 建议容差 | 理由 |
|---|---|---|
| 纯数学往返 | `1e-9` | 双精度有效位约 15~17 位 |
| **经过 `int` 截断的换算** | **`(1−y)² / 系数`** × 1.5 | 见下方推导，**不能写固定值** |
| 总 DPS（大数值） | `±0.01%` 相对 | 用 `totalDps * 0.0001`，别写死绝对值 |
| 技能伤害占比 | `±0.5pp` | 百分点，写成 `0.5`（占比本身已是 0~100 的数） |

> **容差不是拍脑袋定的，要按"物理下限"推。** 定太严 → 跨编译器永远红；定太松 → 抓不到真实漂移。

## 为什么 int 截断的容差**不能写固定值**

`count = coeff·y/(1−y)` 反算时 `static_cast<int>` 向下截断，最多丢 1 个点数。
而 1 个点数对应的百分比误差是 `dy/dc = coeff/(c+coeff)²`，代入 `c+coeff = coeff/(1−y)` 得：

```
最坏误差 = (1−y)² / coeff
```

**它随 y 变化**，所以固定容差必然出错（对低 y 太严、对高 y 太松）。实测（coeff = 200000）：

| y | count | 实际误差 | `(1−y)²/coeff` |
|---|---|---|---|
| 0.0 | 0 | 0 | 5.0e-6 |
| 0.1 | 22222 | 9.0e-7 | **4.05e-6** |
| 0.3 | 85714 | 7.0e-7 | 2.45e-6 |
| **0.478** | 183141 | **1.04e-6** | 1.36e-6 |
| 0.628 | 337634 | 2.8e-7 | 6.9e-7 |
| 0.9 | 1800000 | 0 | 5.0e-8 |

> ⚠️ 注意 `y=0.478` 那行：误差 **1.04e-6 > 固定 1e-6** —— 这就是"固定容差不可靠"的直接证据。
> 正确写法是按每个样本自己的上界断言：
>
> ```cpp
> const double bound = (1.0 - y) * (1.0 - y) / K * 1.5;   // 理论下限 × 1.5 余量
> CHECK_NEAR(back, y, bound);
> ```

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

**浮点特殊处理**：`double` / `float` 用 `std::numeric_limits<T>::max_digits10`
（double = 17 位、float = 9 位有效数字）而不是 `std::to_string`。

```cpp
inline std::string toStr(double v)
{
    std::ostringstream oss;
    oss << std::setprecision(std::numeric_limits<double>::max_digits10) << v;
    return oss.str();
}
```

**为什么不能用 `std::to_string`**：它固定 6 位小数，
`5411840.9800661234` 和 `5411840.9800661299` 会打印成一模一样，
断言失败时你会盯着两条相同的数字发呆 —— 金丝雀（逐位比较）尤其受影响。

对比：

| | `std::to_string` | `max_digits10` |
|---|---|---|
| 实际值 | `5411840.980066` | `5411840.9800661234` |
| 期望值 | `5372804.289026` | `5372804.2890256951` |

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

| 目标 | 文件 | 跑什么 | 当前内容 | 耗时 |
|---|---|---|---|---|
| `dps_unit_tests` | `tests/test_attributes.cpp` | 属性换算、边界、守恒等**不变量** | 6 个用例 / 16 条断言 | 毫秒级 |
| `dps_golden_tests` | `tests/test_golden_beam.cpp`<br>`tests/test_parallel_isolation.cpp` | 完整 180 秒模拟：对比**钉值** + 金丝雀 + 并行隔离 | 4 个用例 | **0.7 秒** |

**为什么拆两个 exe**：拆进程是最彻底的隔离手段。`dps_golden_tests` 里还要跑双线程并行，
单独一个可执行文件能保证它不受单元测试留下的状态影响。

> 注意：监听器表已经下放到 `Person`（`core/ActionListeners.hpp`），
> 不再是 `TypedAction` 的 `inline static`。但**库里仍有进程级状态**会跨模拟留存
> （`Buff::ID` / `Skill::ID` 计数器、`creatorMap`、`Logger` 配置），
> 所以拆 exe 与金丝雀仍然有必要。

### 核心概念：两套测试抓的东西不一样

| | 不变量测试 | 黄金基线 |
|---|---|---|
| 问的问题 | 「还算得对吗？」 | 「变了吗？」 |
| 改**数值**（如 `0.08` → `0.09`） | ✅ 应该仍然绿 | ❌ **必然红**（这就是它的职责） |
| 改**逻辑但行为不变**（重构） | ✅ 绿 | ✅ 绿 |
| 引入 bug 导致**算错** | 🎯 **第一时间红** | 红 |
| 引入 bug 只**偏移数值** | 绿（抓不到） | 🎯 **只有它能抓** |

**两者缺一不可**，别指望用一套覆盖另一套。

### 黄金基线的口径（很容易搞错）

照抄 `printDamageStatistics`，**不要"统一"成一样的**，它们本来就不一样：

| 量 | 公式 | 注意 |
|---|---|---|
| 总 DPS | `Σ(damage + luckyDamage) / 秒数` | **含**幸运 |
| 技能占比 | `skill.damage / Σ(damage + luckyDamage)` | 分子**不含**幸运 |
| 暴击率 | `Σ CritDamageCount / Σ damageCount` | **次数比次数**，不是次数比伤害 |

校验方法：各技能占比之和 ≈ `1 − 幸运伤害占比`（我实测 86.93% + 13.07% = 100%）。

### 钉值优先级：整数计数 > 浮点 DPS

除了 DPS，一定要钉**精确整数计数**：

```cpp
CHECK_EQ(totalDamageCount,      6872);
CHECK_EQ(totalLuckyDamageCount, 1406);
CHECK_EQ(totalCritCount,        4318);
CHECK_EQ(totalReleasedTimes,    2594);
```

理由：

1. **整数无浮点误差** → 精确比较，不需要容差
2. **比 DPS 敏感几百倍** → `releasedTimes` 从 2594 变 2593 立刻红，而 DPS 的 ±537 容差可能吸收掉
3. **覆盖"整体乘区"盲区** → 占比是比值，全局乘数变化时比值不变；只有 DPS + 计数能抓住那类改动

> ⚠️ 注意这些是**所有技能之和**，不是某个技能的。单技能量级完全不同
> （水龙卷 `damageCount` = 1793、Beam `luckyDamageCount` = 385）。
> 别写成 `stats.at(WaterSpout::name).damageCount == 6872`。

### 金丝雀：检测「进程级状态残留」

```cpp
// 两次调用各自创建、各自销毁 Person，结果必须逐位相等
const auto first  = runBeamSimulationOnce(cfg, 42, 180000, 10);
const auto second = runBeamSimulationOnce(cfg, 42, 180000, 10);
CHECK_EQ(first.size(), second.size());   // 再逐技能比对 6 个字段
```

它盯的是**跨模拟残留**。监听器表已经下放到 `Person`（`core/ActionListeners.hpp`）
后，原先"两个 Person 同时存活 → 栈溢出 `0xC00000FD`"的问题已消失，
但库里仍有进程级状态会跨模拟留存：

| 状态 | 说明 |
|---|---|
| `Buff::ID` / `Skill::ID` | 只增不清的计数器（已 atomic，唯一性 OK，但号码会继续增长） |
| `SkillCreator` / `BuffCreator::creatorMap` | 每次 `Initialize()` 都会 insert（`Initializer` 里已加锁） |
| `Logger` / `SimulationLog` | 全局配置与文件句柄 |

**典型症状**：某个 Buff 忘了在析构里 `remove<T>` 监听器 → 第二遍模拟时
上一次的回调仍在 → 出伤次数偏移 → 金丝雀红。

### 并行隔离：多 Person 同时跑必须和单线程一致

`tests/test_parallel_isolation.cpp` 里两个线程各跑一遍 seed=42 的完整模拟，
结果与单线程**逐位**比对。

```cpp
std::thread workerA([&]{ threadA = runBeamSimulationOnce(cfgA, 42, 180000, 10); });
std::thread workerB([&]{ threadB = runBeamSimulationOnce(cfgB, 42, 180000, 10); });
workerA.join(); workerB.join();
// 比对放在主线程做
```

⚠️ **三个约束**：

1. `CHECK_*` 会改全局失败计数器，**不是线程安全的** → 线程里只算，主线程比对
2. 线程函数里抛异常会 `std::terminate` → 测试里不允许抛
3. `Logger` 是全局的，并发写会交错 → 数值不受影响，但屏幕会花

**这条测试是「监听器表下放 Person」的毕业证**。把 `ActionListeners` 的表改回
进程级共享（`inline static`），它会立刻崩溃 —— 已验证。

### 重钉（re-pin）流程

黄金基线变红 **不等于测试失败**，而是"数值变了"。流程：

```
① 红线出现
② 确认是不是有意的机制改动
③ 是 → 重跑记录新值 → 更新钉值常量 → 提交信息写明「旧值 → 新值」
   否 → 真 bug，去改代码
```

**这一步是给"机制改动"立的规矩**：让每一次数值变化都留下证据（哪个提交、因为什么、变了多少），
而不是几个月后拉表才发现、且查不出是哪次改动导致的。

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
    const double K = static_cast<double>(p->getPropertyTransformationCoeffcient_General());
    const double y = 0.478;
    const int count = p->getProficientCount(y);
    const double back = static_cast<double>(count) / (count + K);

    CHECK_NEAR(back, y, (1.0 - y) * (1.0 - y) / K * 1.5);   // ← 容差随 y 变化，见 2.5 节的推导
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
    const double K = static_cast<double>(p->getPropertyTransformationCoeffcient_General());
    for (double y : {0.0, 0.1, 0.478, 0.628, 0.9})
    {
        const int c = p->getProficientCount(y);
        const double back = static_cast<double>(c) / (c + K);
        CHECK_NEAR(back, y, (1.0 - y) * (1.0 - y) / K * 1.5);
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
