// ============================================================================
// 数值单元测试
//
// 只测「不变量」——无论数值怎么调都该成立的关系：
//   · 百分比 → 点数 → 百分比 的往返误差有物理下限
//   · 面板低于 基础值+武器额外 时不能产生负点数
//   · 点数加减同一量后总量守恒
//   · 默认面板的点数钉值（防止有人手改换算公式）
//
// 不测「具体 DPS」那类钉值 —— 那是黄金基线（test_golden_beam.cpp）的职责。
// 分工：这里抓「算错了」，黄金基线抓「变了」。
//
// 全部只用 public 接口，不改动任何产品代码。
// ============================================================================

#include "TestHarness.h"
#include "Mage/Beam/Person.h"

#include <memory>

namespace {

// ---- 与 main_Beam.cpp 一致的站街面板 -------------------------------------
constexpr double kPrimaryAttr = 9272;
constexpr double kCrit        = 10.12;
constexpr double kQuickness   = 38.18;
constexpr double kLucky       = 23.21;
constexpr double kProficient  = 74.8;
constexpr double kAlmighty    = 20.88;

constexpr int kAtk        = 8151;
constexpr int kRefineAtk  = 1100;
constexpr int kElementAtk = 430;

constexpr int kTotalTime     = 180000;
constexpr int kFantasyConfig = 7;   // 机器人 + 嗜血毛球

constexpr double kRoundTripTol = 1e-6;   // 推导见 test_count_percent_roundtrip

/**
 * @brief 构造一个 Mage_Beam，武器固定为「精通 + 急速」各占面板 6%
 *
 * 只构造、不跑 Initializer：这样拿到的是 Person 构造函数刚算完的点数，
 * 也就是固定其行为的那个时刻（后面装备幻想被动还会把精通重新归类一次）。
 */
std::unique_ptr<Mage_Beam> makeBeam(double crit, double quickness, double lucky, double proficient)
{
    return std::make_unique<Mage_Beam>(
        kPrimaryAttr, crit, quickness, lucky, proficient, kAlmighty,
        kAtk, kRefineAtk, kElementAtk,
        /*attackSpeed*/ 0.0, /*castingSpeed*/ 0.0,
        /*critDmgSet*/ 0.0, /*incSet*/ 0.0, /*eleIncSet*/ 0.0,
        kTotalTime,
        kFantasyConfig,
        WeaponConfig{SecondaryAttributeConfig::Proficient,
                     SecondaryAttributeConfig::Quickness,
                     0.06, 0.08});
}

std::unique_ptr<Mage_Beam> makeDefaultBeam()
{
    return makeBeam(kCrit, kQuickness, kLucky, kProficient);
}

// 面板远低于「基础值 + 武器 6%」的一组输入：
//   暴击 4% < 基础 5%
//   急速 4% < 武器 6%
//   精通 10% < 基础 6% + 武器 6% = 12%
std::unique_ptr<Mage_Beam> makeLowPanelBeam()
{
    return makeBeam(/*crit*/ 4.0, /*quickness*/ 4.0, /*lucky*/ 4.0, /*proficient*/ 10.0);
}

} // namespace

// ---------------------------------------------------------------------------
// 1. 百分比 ⇄ 点数 往返
// ---------------------------------------------------------------------------
// 反解公式：count = K·y/(1−y)；换回来：y' = count/(count+K)
//
// 容差不能拍脑袋写 1e-6 或 1e-9，要按截断误差的物理下限推：
//
//   count 是 int，static_cast<int> 向下截断，最多丢掉 1 个点数。
//   dy/dc = K/(c+K)²，代入 c = K·y/(1−y) 得 (c+K) = K/(1−y)，于是
//
//       最坏误差 = K/(c+K)² = (1−y)²/K
//
//   它随 y 变化：y=0.1 时约 4.1e-6，y=0.628 时只有 6.9e-7。
//   所以固定容差要么对低 y 太严（误报），要么对高 y 太松（漏报）。
//   这里按每个样本自己的上界来断言，再留 1.5 倍余量。
//
// （实测佐证：y=0.478 的误差 1.04e-6 恰好超过固定 1e-6 —— 就是固定容差不可靠的证据。）
TEST(count_percent_roundtrip, "点数 ⇄ 百分比 往返")
{
    auto p = makeDefaultBeam();
    const double K = static_cast<double>(p->getPropertyTransformationCoeffcient_General());

    // 覆盖：0、低、中、低面板可点数化上限附近、高饱和
    const double samples[] = {0.0, 0.1, 0.3, 0.478, 0.628, 0.9};
    for (double y : samples)
    {
        const int count = p->getProficientCount(y);                    // 百分比 → 点数（int 截断）
        const double back = static_cast<double>(count) / (count + K);  // 点数 → 百分比

        const double bound = (1.0 - y) * (1.0 - y) / K * 1.5;          // 理论下限 × 1.5 余量
        CHECK_NEAR(back, y, bound);
    }
}

// ---------------------------------------------------------------------------
// 2. 低面板不产生负点数（507f351 的回归测试）
// ---------------------------------------------------------------------------
// 修复前：括号内为负 → getXxxCount 直接把负值存进字段。
// 修复后：构造函数 5 处反解都套了 ClampCountablePercent，负数被夹到 0。
TEST(no_negative_count_on_low_panel, "低面板不产生负点数")
{
    auto p = makeLowPanelBeam();

    // 下界：绝不能是负数
    CHECK_TRUE(p->getCriticalCount()   >= 0);
    CHECK_TRUE(p->getQuicknessCount()  >= 0);
    CHECK_TRUE(p->getProficientCount() >= 0);

    // 更严格：可点数化部分被夹到 0，所以点数应该正好是 0
    CHECK_EQ(p->getCriticalCount(),   0);
    CHECK_EQ(p->getQuicknessCount(),  0);
    CHECK_EQ(p->getProficientCount(), 0);
}

// ---------------------------------------------------------------------------
// 3. 点数触碰后，急速 / 精通 的总量不漂移
// ---------------------------------------------------------------------------
// 不变量：base + ExtraPercent <= 总值（由 reclassifyXxx 里的 MaxExtraPercent 保证）
// 满足时，changeXxxCount 的重算式 总值 = base + conv + extra 不会把总值抬高。
//
// 低面板这一组里：
//   急速：extra 被夹到 4%，可点数化 0  → 0    + 0 + 0.04 = 0.04  守恒
//   精通：extra 被夹到 4%，可点数化 0  → 0.06 + 0 + 0.04 = 0.10  守恒
TEST(totals_stable_after_touch, "点数触碰后急速与精通总量守恒")
{
    auto p = makeLowPanelBeam();

    const double quicknessBefore  = p->getQuickness();
    const double proficientBefore = p->getProficient();

    p->changeQuicknessCount(0);     // 模拟 DoubleProficientBuff 之类的「零增益触碰」
    p->changeProficientCount(0);

    CHECK_NEAR(p->getQuickness(),  quicknessBefore,  kRoundTripTol);
    CHECK_NEAR(p->getProficient(), proficientBefore, kRoundTripTol);
}

// ---------------------------------------------------------------------------
// 4. 面板低于基础值时，暴击会漂到基础值（已知限制，固化成活的文档）
// ---------------------------------------------------------------------------
// changeCriticalCount 的重算式是 暴击 = base + conv + extra。
// 面板 4% < 基础 5% 时，可点数化部分被夹到 0，于是重算得到 0.05 + 0 + 0 = 0.05，
// 也就是 4% 被抬到 5%。507f351 只夹了 extra 通道、没有动 base，所以这个行为保留。
//
// 这属于「面板填得比基础值还低」的 nonsense 输入。
// 若将来修掉（比如把 base 也纳入夹取），本断言会变红 —— 那时请同步改成
//   CHECK_NEAR(p->getCritical(), 0.04, kRoundTripTol);
TEST(crit_drifts_to_base_when_below_base, "面板低于基础值时暴击漂到基础值（已知限制）")
{
    auto p = makeLowPanelBeam();

    CHECK_NEAR(p->getCritical(), 0.04, kRoundTripTol);   // 构造后仍是 4%

    p->changeCriticalCount(0);                           // 触碰一次

    CHECK_NEAR(p->getCritical(), 0.05, kRoundTripTol);   // 被重算式抬到 base 5%
}

// ---------------------------------------------------------------------------
// 5. 默认面板的点数钉值
// ---------------------------------------------------------------------------
// 作用：防止有人手改 换算系数 / base 值 / 武器 6% 的归类逻辑 而没被察觉。
// 红了先确认是不是有意的机制改动；是的话更新这三个数字。
//
// 数值来源（武器第一属性=精通、第二属性=急速，各 6%）：
//   暴击 可点数化 = 10.12 − 5(base) − 0       = 5.12%   → 10792
//   急速 可点数化 = 38.18 − 0 − 6(武器)        = 32.18%  → 94898
//   精通 可点数化 = 74.8  − 6(base) − 6(武器)  = 62.80%  → 337634
TEST(count_pin_default_panel, "默认面板点数钉值")
{
    auto p = makeDefaultBeam();

    CHECK_EQ(p->getCriticalCount(),   10792);
    CHECK_EQ(p->getQuicknessCount(),  94898);
    CHECK_EQ(p->getProficientCount(), 337634);
}

// ---------------------------------------------------------------------------
// 6. 点数加 N 再减 N，总量守恒
// ---------------------------------------------------------------------------
// 覆盖「点数操作偷改总量」这一族 bug：加进去和拿出来的必须是同一份。
// 容差仍取 1e-6，因为中间过一次 int 截断（最坏差 1 个点数 ≈ 6e-7）。
TEST(count_add_remove_conservation, "点数加 N 再减 N 总量守恒")
{
    auto p = makeDefaultBeam();
    const double before = p->getProficient();

    p->changeProficientCount(50000);
    p->changeProficientCount(-50000);

    CHECK_NEAR(p->getProficient(), before, kRoundTripTol);
}
