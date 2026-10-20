#pragma once
// ============================================================================
// 极简测试框架（零依赖、单头文件、约 100 行）
//
// 用法（见 tests/test_attributes.cpp）：
//   TEST(换算往返) {
//       CHECK_EQ(实际, 期望);
//       CHECK_NEAR(实际, 期望, 1e-6);
//       CHECK_TRUE(条件);
//   }
// 入口在 tests/test_main.cpp：它遍历所有已注册用例，全绿返回 0，否则返回 1。
// ctest 就是靠这个**退出码**判断成功失败的。
// ============================================================================

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace test {

// ---------------------------------------------------------------------------
// 1. 用例登记表
// ---------------------------------------------------------------------------
// 每个 TEST(名字) 都会生成一个静态 Registrar 对象，它在 main() 之前
// （静态初始化阶段）把自己塞进这张表。这样我们就不用把测试函数名
// 一个个手写进 main——加一个 TEST 就自动被跑到。
//
// 为什么用**函数内 static** 而不是全局变量：
//   全局变量在不同翻译单元（.cpp）之间的初始化顺序未定义，
//   而"函数内 static 局部变量"保证第一次调用时初始化，天然规避这个坑。
// ---------------------------------------------------------------------------

struct TestCase
{
    const char* name;   // 用例名（用于打印）
    void (*fn)();       // 用例函数指针
};

inline std::vector<TestCase>& registry()
{
    static std::vector<TestCase> all;
    return all;
}

// 登记器：构造时就把用例写进表里
struct Registrar
{
    Registrar(const char* name, void (*fn)())
    {
        registry().push_back(TestCase{name, fn});
    }
};

// ---------------------------------------------------------------------------
// 2. 失败计数与报告
// ---------------------------------------------------------------------------
// 设计要点：断言失败**不中断**当前用例，只记一笔继续往下跑。
// 好处是一次就能看到该用例的所有问题，而不是"修一个跑一次"。
// ---------------------------------------------------------------------------

inline int& failureCount()
{
    static int count = 0;
    return count;
}

inline void reportFailure(const char* file, int line, const std::string& what)
{
    ++failureCount();
    std::printf("       [FAIL] %s:%d\n              %s\n", file, line, what.c_str());
}

// ---------------------------------------------------------------------------
// 3. 把值转成字符串（仅用于失败信息）
// ---------------------------------------------------------------------------
// 一组非模板重载优先于泛型模板，所以 int/double/string 都能正确打印；
// 其他类型落回模板，打 "?"（不为了一个失败信息引入复杂 SFINAE）。
// ---------------------------------------------------------------------------

template <typename T>
inline std::string toStr(const T&) { return "?"; }

inline std::string toStr(int v)                { return std::to_string(v); }
inline std::string toStr(long v)               { return std::to_string(v); }
inline std::string toStr(long long v)          { return std::to_string(v); }
inline std::string toStr(unsigned v)           { return std::to_string(v); }
inline std::string toStr(unsigned long v)      { return std::to_string(v); }
inline std::string toStr(double v)             { return std::to_string(v); }
inline std::string toStr(float v)              { return std::to_string(v); }
inline std::string toStr(bool v)               { return v ? "true" : "false"; }
inline std::string toStr(const std::string& v) { return v; }
inline std::string toStr(const char* v)        { return std::string(v ? v : "(null)"); }

} // namespace test

// ---------------------------------------------------------------------------
// 4. 宏
// ---------------------------------------------------------------------------

// TEST(标识符, "描述") 展开成三样东西：
//   ① 前置声明 static void 标识符();
//   ② 一个静态 Registrar 对象，在 main 之前完成登记；
//   ③ 函数体开头 —— 所以 TEST(...) 后面直接跟 { ... } 就是用例体。
//
// 用 static 是为了让不同 .cpp 里同名用例互不冲突（内部链接）。
//
// ⚠️ 为什么两个参数、以及为什么第二个**不能加 #**：
//   · 第一个参数参与生成函数名和静态变量名，必须是标识符；MinGW 的 GCC 8.1
//     不支持 UTF-8 标识符（GCC 10 才支持），写中文会报 "stray '\344' in program"。
//   · 第二个参数是**字符串字面量**，直接原样传给 Registrar，中文没问题。
//     如果写成 #testDescription，预处理器会把源码里的 "描述"（含引号）整体字符串化，
//     于是打印出来会变成  【"描述"】——多出一对引号。所以这里不能加 #。
//
// 例：TEST(harness_assert_macros, "骨架自检：断言宏可用") { ... }
#define TEST(testName, testDescription)                                       \
    static void testName();                                                   \
    static ::test::Registrar test_registrar_##testName(testDescription, testName);  \
    static void testName()

// 三个断言都包在 do{...}while(0) 里：
// 让它表现得像一条语句，避免 if (x) CHECK_EQ(...); else ... 这种写法出问题。
#define CHECK_TRUE(expr)                                                      \
    do {                                                                      \
        if (!(expr))                                                          \
            ::test::reportFailure(__FILE__, __LINE__, "期望为真: " #expr);     \
    } while (0)

#define CHECK_FALSE(expr)                                                     \
    do {                                                                      \
        if ((expr))                                                           \
            ::test::reportFailure(__FILE__, __LINE__, "期望为假: " #expr);     \
    } while (0)

// 精确相等（用于整数点数、布尔、字符串）
#define CHECK_EQ(actual, expected)                                            \
    do {                                                                      \
        const auto test_A_ = (actual);                                        \
        const auto test_E_ = (expected);                                      \
        if (!(test_A_ == test_E_))                                            \
            ::test::reportFailure(__FILE__, __LINE__,                         \
                std::string(#actual " == " #expected " 失败: 实际=") +         \
                ::test::toStr(test_A_) + " 期望=" + ::test::toStr(test_E_));   \
    } while (0)

// 近似相等（用于浮点百分比）。tol 是绝对容差，单位与期望值一致。
#define CHECK_NEAR(actual, expected, tol)                                     \
    do {                                                                      \
        const double test_A_ = static_cast<double>(actual);                   \
        const double test_E_ = static_cast<double>(expected);                 \
        const double test_T_ = static_cast<double>(tol);                      \
        if (!(std::fabs(test_A_ - test_E_) <= test_T_))                       \
            ::test::reportFailure(__FILE__, __LINE__,                         \
                std::string(#actual " ≈ " #expected " 失败: 实际=") +          \
                ::test::toStr(test_A_) + " 期望=" + ::test::toStr(test_E_) +   \
                " 容差=" + ::test::toStr(test_T_));                            \
    } while (0)
