// ============================================================================
// 测试运行器（两个测试程序共用同一个 main）
//
// 职责只有三件：
//   1. 遍历测试框架登记的用例，逐个执行；
//   2. 打印每个用例 OK / FAIL 以及失败原因；
//   3. **用退出码汇报结果**——全绿返回 0，有失败返回 1。
//
// 第 3 点是和 ctest 的契约：ctest 只看进程退出码，不看屏幕输出。
// 所以这里绝不能 return 0 了事，否则测试永远"通过"。
// ============================================================================

#include "TestHarness.h"
#include "core/Logger.h"   // 压低日志级别，避免产品代码的 DEBUG 日志刷屏
#include <iostream>        // 直接用 std::cout，故显式包含（不依赖 TestHarness.h 的传递包含）

int main()
{
    // 被测代码里有大量 Logger::debug（例如 changeXxxCount 每次都打一条），
    // 测试输出只保留 WARNING 及以上，否则报告会被淹掉。
    Logger::setLevel(Logger::Level::WARNING);

    auto& tests = test::registry();

    std::cout << "运行 " << tests.size() << " 个测试\n";
    std::cout << "------------------------------------------------------------\n";

    int failedCases = 0;
    for (const auto& t : tests)
    {
        const int before = test::failureCount();
        t.fn();                                        // 执行用例（失败断言不会抛异常）
        const int diff = test::failureCount() - before;

        if (diff == 0)
        {
            std::cout << "  [ OK ] " << t.name << '\n';
        }
        else
        {
            std::cout << "  [FAIL] " << t.name << "（" << diff << " 处断言失败）\n";
            ++failedCases;
        }
    }

    std::cout << "------------------------------------------------------------\n";
    std::cout << "合计: " << tests.size() << " 个测试, "
              << failedCases << " 个失败, "
              << test::failureCount() << " 处断言失败\n";

    return test::failureCount() == 0 ? 0 : 1;
}
