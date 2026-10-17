# 技能与 Buff 逻辑总览

> 本文逐类说明每个 `Skill` / `Buff` / 幻想类的**触发条件、关键参数、联动关系与副作用**。
> 所有数值以代码为准（1 tick = 1ms，模拟时长默认 180000ms = 180s）。
> 阅读顺序建议：第一节先看框架套路，后面各节按职业查具体类。
>
> **同步状态**：已对齐到「属性双通道 + 机器人幻想」这一版——
> `FantasyPassiveBonus` / `reclassifyXxxPercentToExtra`（见 7.4）、`weaponConfig` 重新归类、
> `JQR` + `JQRBuff`、施法速度重做（`baseDamageTriggerInterval` / `canAffectedBySpeed`）、
> `FloodLuckyBuff` 随灌注开关、`EquipmentSetEffectBuff_Beam` 的消费即清除、11 段爆发轴。
> 改动后若数值有变，请顺手更新对应小节。

---

## 第一节 框架层：技能与 Buff 是怎么被驱动的

### 1.1 三种技能形态（决定 `use()` 何时被调用）

| 基类 | 形态 | `use()` 的行为 | 典型用途 |
| --- | --- | --- | --- |
| `InstantSkill` | 立即触发 | 只触发**一次**（`triggered` 置位后不再触发） | 大招、灌注、瞬发技能 |
| `ContinuousSkill` | 持续触发 | 每 tick 累加 `damageTriggerTimer`，满 `damageTriggerInterval` 就 `trigger()` 一次，并把 interval 扣除 | 涡流、水龙卷 |
| `FacilitationSkill` | 引导型 | 同上按间隔触发；另有 `canEndFacilitation()` 决定何时结束引导 | 射线（Beam） |

**关键点**：`InstantSkill` 的"只触发一次"由 `triggered` 标志保证——所以**同一个技能实例被复用时要小心 `triggered` 状态**。

### 1.2 技能的两个"前摇/间隔"概念

| 字段 | 含义 |
| --- | --- |
| `releasingTime` | **前摇**：从"创建技能"到"开始出伤"的等待。为 0 表示立即出伤 |
| `baseDamageTriggerInterval` | **基础出伤间隔**（不含速度），构造时定死，之后不再变 |
| `damageTriggerInterval` | **实际出伤间隔** = `baseDamageTriggerInterval / (1 + 施法速度)`，由 `Person::modifySkillInterval()` 在施法速度变化时重算 |
| `canAffectedBySpeed` | 标记该技能是否参与速度重算（目前只有 `Beam` 和 `WaterSpout` 开启） |
| `isNoReleasing` | 标记"无前摇"：`Person::createSkill` 会直接把它塞进 `continuousSkillList`，不占用正在释放槽 |

**施法速度如何影响出伤频率（重要）**

- 只有标了 `canAffectedBySpeed = true` 的技能才会跟着施法速度变快；
- `Person::modifySkillInterval()` 在 `addCastingSpeed()` 与 `recalcSpeedFromQuickness()` 里被调用，重算范围是**正在释放的技能** + **`continuousSkillList` 里的技能**；
- 所以 `Beam`（引导中）与 `WaterSpout`（持续列表）能实时受益，而 `Vortex` 等**未标记**的技能只有构造时除一次速度、之后不再更新。

**注意**：技能的构造里对时间做 `/= (1 + p->castingSpeed)` 只是设"初始值"，真正持续生效的是上面那条重算链。

### 1.3 Buff 的三类行为模式（这是读懂所有 Buff 的钥匙）

**模式 A：改面板 —— 在构造/析构里增减属性，事件协议保证配对**

```cpp
// 构造函数
p->triggerAction<AttackIncreaseModifyAction>(0.15);
// 析构函数
p->triggerAction<AttackIncreaseModifyAction>(-0.15);
```

> ⚠️ **这是最容易出错的地方**：构造加多少，析构必须减多少。少减一次就是"永久增益"，多加一次会累积漂移。
> 本文后面每提到"对称"，指的就是这种加减配对。

**模式 B：监听事件 —— 注册回调，等某事件发生再反应**

```cpp
auto info = std::make_unique<DamageListener>(
    this->getBuffID(), [this](DamageInfo& d){ this->listenerCallback(d); });
AttackAction::addListener(std::move(info));
// 析构里必须：AttackAction::deleteListener(this->getBuffID());
```

可监听的事件（`core/Action.h` 里的 Action 类型）与对应回调签名：

| Action | 监听器类型 | 回调参数 | 触发时机 |
| --- | --- | --- | --- |
| `AttackAction` | `DamageListener` | `DamageInfo&` | 每次出伤 |
| `CreateSkillAction` | `CreateSkillListener` | `Skill* const` | 创建技能时 |
| `CreateBuffAction` | `CreateBuffListener` | `Buff*` | 创建 Buff 时 |
| `EnergyConsumeAction` | `EnergyListener` | `double` | 消耗能量 |
| `EnergyRevertAction` | `EnergyListener` | `double` | 回复能量 |
| `ResourceConsumeAction` | `ResourceListener` | `double` | 消耗玄冰 |
| `ResourceRevertAction` | `ResourceListener` | `double` | 回复玄冰 |
| `CDReduceAction` / `CDRefreshAction` | `CDListener` | `double` | 减 CD / 刷新 CD |
| `*CountModifyAction` / `*PercentModifyAction` | `SecondaryAttributeListener` / `PrimaryAttributeListener` | `double` | 属性被修改时 |

**模式 C：定时更新 —— 在 `update(deltaTime)` 里自己数时间**

```cpp
this->timer += deltaTime;
if (this->timer >= this->triggerInterval) { /* 做事 */ this->timer -= this->triggerInterval; }
```

### 1.4 两个必要的实现约定

1. **每个注册了监听器的 Buff，析构里必须 `deleteListener(this->getBuffID())`**，否则静态监听器表里会留下指向已析构对象的回调。
2. **回调里不要直接改容器**。`Person::cleanupFinishedBuffs` 之所以先把待销毁 Buff 移出容器再析构，就是因为 Buff 析构可能触发新 Buff 创建。

---

## 第二节 core 层的类

### 2.1 `SkillReleasedTimesStatistics`（`core/Buff.cpp`）

| 项 | 内容 |
| --- | --- |
| 类型 | Buff（`isInherent = true`，永久） |
| 监听 | `CreateSkillAction` → 每次创建技能给对应技能名计数 +1 |
| 数据 | `skillReleasedTimesMap[name] = 次数` |
| 用途 | 统计"释放次数"，`Person::calculateDamageStatistics` 把结果写进 `DamageStats.releasedTimes` |
| 注意 | 它不产生任何伤害，纯粹是统计。是 `Initializer::Initialize()` 里唯一**硬编码注册**的 Buff |

### 2.2 `Factor` / `RealFactor`（`core/Buff.h`）

两个空壳基类，用于表达"因子"语义：

- `Factor`：因子 Buff 的通用基类（`ExtendedLuck`、职业因子等都继承它）
- `RealFactor`：带自身能量池的因子（成员 `maxEnergy/presentEnergy`，默认 1200），`changeEnergy(n)` 带上下限夹取

**注意**：目前 `RealFactor` 的能量池并未参与伤害计算，只是数据结构预留。

---

## 第三节 射线（Beam）技能

| 技能 | 形态 | 倍率 / 固定值 | 前摇 | CD | 关键行为 |
| --- | --- | --- | --- | --- | --- |
| `Beam` 寒冰射线 | Facilitation | 0.42 / 150 | 850 ÷(1+施法) | 0 | 见下 |
| `Vortex` 涡流 | Continuous | 0.2 / 120 | — | 100 | 无前摇，持续 10s，间隔 500÷(1+施法)（**构造时除一次，不随速度更新**），回能 2 |
| `WaterSpout` 水龙卷 | Continuous | 1.4448 / 514 | — | 100 | 无前摇，持续 20s，间隔 500÷(1+施法)，**`canAffectedBySpeed`**；射线一停立刻 `duration = 0` |
| `FrostWind` 冻结寒风 | Instant | 1.75 / 1050 | 10 | 1000 | 触发 4 个联动 |
| `Flood_Beam` 灌注 | Instant | 0 | **75** | 42290 | 只负责上灌注 Buff |
| `CrystalsHail` 冰晶坠落 | Instant | 6 / 0 | — | — | 无前摇，回能 30 |
| `FrostDecreePulse` 冰令脉冲 | Instant | 5 / 0 | — | — | 无前摇 |
| `IceArrow_Beam` 冰箭 | Instant | 0.28 / 0 | — | 0 | 无前摇，**打两段**，`canTriggerLucky = false`（无幸运伤害） |
| `FrostBurst` 霜爆 | Instant | 0.28 / 0 | — | 0 | 无前摇，可幸运 |
| `Ultimate_Beam` 大招 | Instant | 12.6 / 5400 | 100 | 54000 | 上大招增伤 Buff |
| `FantasyImpact_Beam` 幻想冲击 | Instant | 13.75 / 0 | — | — | 无前摇，**不可暴击** |

### 3.1 `Beam` 寒冰射线（核心输出）

```
构造：canTriggerLucky = true,  canAffectedBySpeed = true
      multiplying = 0.42, fixedValue = 150        （记下 baseXxx 用于叠加）
      baseDamageTriggerInterval = 300
      damageTriggerInterval = 300 / (1 + castingSpeed)   ← 之后由 modifySkillInterval 持续重算
      releasingTime = 850 / (1 + castingSpeed)           ← 前摇，只在构造时算一次
      damageIncreaseAdd = 0.16                    （自带 16% 增伤）
      energyReduce = 12 * (baseDamageTriggerInterval / 300) = 12  ← 恒为 12，不随速度变化
      duration = 永久,  noEnergyConsumeTime = 5
```

**每次出伤（`trigger`）依次做三件事：**

1. `count++`，然后 `AttackAction` 出伤
2. **`count > 5` 之后才开始耗能**：`EnergyConsume(energyReduce)` + `EnergyRevert(energyReduce)`
   → 注意这里是"先扣再回"，实际上是通过这两个事件让监听者感知（多个 Buff 挂着能量事件）
3. **每次出伤叠 8% 倍率与固定值，最多叠 10 次**（`addMultiplyingTimes < 10`）
   → 实测结算里 `Beam` 的最终倍率远高于 0.42，就是这 10 层叠出来的

**结束引导条件（`canEndFacilitation`）**：`当前能量 < 本次耗能` **或** `duration <= 0`
→ 也就是"能量断了就停"。这是"射线能量稳定不断"这一简化假设的实现位置。

### 3.2 `Vortex` 涡流 / `WaterSpout` 水龙卷

两者都是**无前摇持续性技能**，区别在：

- `Vortex`：间隔 500ms，持续 10s，每次回能 2
- `WaterSpout`：间隔 `500/(1+施法)`，持续 20s，不回能

**`WaterSpout` 有一个自我终止逻辑**：每次出伤时检查"当前正在释放的技能是不是 `Beam`"，**如果不是就把自己的 `duration` 置 0 结束**。

> 这意味着：**水龙卷的生命周期绑定了射线**。一旦射线停下（或者被别的技能打断），龙卷立即消失。
> 这是"龙卷风无限持续时间"以外的一个隐含约束，改动射线节奏时要留意。

### 3.3 `FrostWind` 冻结寒风（联动最多的技能）

`trigger()` 一次引发 4 件事：

```cpp
EnergyRevert(5 * maxResourceNum)      // 按玄冰上限回能（满玄冰时回能巨大）
AttackAction(this)                    // 本体出伤
ResourceConsume(6)                    // 扣 6 玄冰
CreateBuff(EnergySurgeLawBuff)        // 触发"涌能法则"
CreateBuff(FrostwindFocusBuff)        // 触发"寒风凝聚"（减耗能 27%）
CreateSkill(CrystalsHail)             // 触发"冰晶坠落"
```

**注意顺序**：先回能再扣玄冰。而 `CrystalsHail` 自己又会再上一次 `FrostwindFocusBuff`。

### 3.4 `IceArrow_Beam` 冰箭（高频触发源）

```
构造：isNoReleasing = true
      criticalAdd = 1                      （+100% 暴击率 → 必暴）
      criticalIncreaseAdd = 0.1            （爆伤 +10%）
      finalIncreaseAdd = Proficient * 1.6  （最终增伤 = 精通 × 1.6，构造时快照）
      canTriggerLucky = false              （默认值，未开启 → 幸运伤害恒为 0）
      energyAdd = 2
      skillType = NORMAL + WITCHCRAFT      （不是 SPECIALIZED → 吃不到 SXMQBuff 的 fixedValue）
      构造时还会 ResourceRevert(1)          ← 创建即回 1 玄冰
```

> ⚠️ **冰箭拿不到幸运伤害**：`canTriggerLucky` 保持默认 `false`，唯一打开它的是 `IceArrowLuckyRealBuff`
> （**仅幸运流**注册）。所以在急速精通流下冰箭的`Lucky Damage` 恒为 0，而同样倍率的 `FrostBurst`
> （构造里有 `canTriggerLucky = true`）能打出大量幸运伤害。

`trigger()` **连打两次 `AttackAction`**（注释写的是"冰箭本体伤害"两次），然后 `EnergyRevert(energyAdd)`。

> ⚠️ **这是全项目最高频的技能**（实测 3 分钟近 2000 次命中）。它的触发源有 4 个：
> `FrostCrystalResonanceBuff`（射线期间每 0.5s 自动）、`ExtensiveArrow`（专精技能 10 次）、`Meteorite`（无"无尽苦寒"时）、`SpearCritialToRevertIceBuff`（冰矛侧暴击）。
> 改它等于改全局数值。

### 3.5 `FrostBurst` 霜爆

与冰箭同倍率（0.28）、同样必暴（`criticalAdd = 1`）、可幸运（`canTriggerLucky = true`），单段出伤。

触发源：`BeamBuildBuff` 监听"冰箭暴击"时创建它。

### 3.6 `Ultimate_Beam` 大招

`trigger()`：先 `CreateBuff(UltiIncreaseBuff_Beam)`，再 `AttackAction`。没有别的联动。

### 3.7 `FantasyImpact_Beam` 幻想冲击

```
multiplying = 13.75, fixedValue = 0
damageIncreaseAdd = p->Lucky      （构造时快照玩家幸运值）
dreamIncreaseAdd = 0.15           （注意：上一行先被赋 1.0 又立刻被 0.15 覆盖）
canTriggerCrit = false            （不可暴击）
```

> ⚠️ `dreamIncreaseAdd` 被连续赋值两次（`1.0` 然后 `0.15`），**只有 0.15 生效**。这大概率是历史遗留，改数值时别被第一行误导。
> 另：`IceArrow_Icicle` 的"二重触发"（50% 再打一次）在 Beam 版里是**注释掉的**，而冰矛版 `FantasyImpact` 是**启用的**——两个职业在这点上不对称。

### 3.8 自动战斗的硬编码爆发轴（`AutoAttack.cpp`）

`setupHardcodedOutBurstAxis()` 把阶段按固定顺序入队，**随机性几乎被压没**。当前队列（共 **11 条**）：

```
twoFantasyAndUlti → window → burst2 → window → twoFantasyAndUlti → window
                  → burst5 → window → pureBurst → window → twoFantasyAndUlti
```

- `burst2` / `burst5` 由两个形参决定：`simpleBurstInSecondBurst` / `simpleBurstInFifthBurst`
  （`true` → `simpleFantasyOnly`，`false` → `pureBurst`）；
- ⚠️ 目前**所有 7 个调用点都只传第 1 个实参**，所以第 5 波恒为 `pureBurst`；
- 阶段时长：4 种爆发阶段都是 **18000ms**，`window` 是 **5000ms**（比早期版本的 20000 / 6000 都缩短了）。

| 改动点 | 位置 |
| --- | --- |
| 阶段时长 | `registerOutBurstLogic()` 里每个 `m_outBurstLogicMap[...] = { 时长, lambda }` |
| 阶段顺序 | `setupHardcodedOutBurstAxis()` 里连续的 `addOutBurstLogicToQueue` |
| 每阶段放哪些技能 | `AutoAttack_Mage_Beam_*` 构造函数的 `m_stages` 表 |

---

## 第四节 射线 Buff

### 4.1 灌注体系（射线节奏的核心）

#### `FloodBuff_Beam` 灌注（触发源：`Flood_Beam` 技能）

| 项 | 值 |
| --- | --- |
| 持续时间 | 15000ms |
| 回能 | 每 100ms 回 `number = 0.6`（即每秒 6 点） |
| 构造副作用 | `QuicknessCountModifyAction(1040)` + `QuicknessPercentModifyAction(0.21)` |
| 析构回滚 | ✅ **对称**：`~FloodBuff_Beam` 里 `-1040` / `-0.21` |

> ⚠️ 灌注抬高的急速会改变 `p->castingSpeed`，进而触发 `Person::modifySkillInterval()` 重算射线与水龙卷的出伤间隔。
> 另外灌注还是若干"条件型 Buff"的开关（见下表），以及 `FloodLuckyBuff` 的应用/回滚依据。

**灌注是全局开关**：下面这些 Buff 都以"灌注是否在场"作为条件——

| Buff | 灌注期效果 |
| --- | --- |
| `ChillPersistenceBuff` | 射线耗能 **-25%** |
| `FrostCrystalResonanceBuff` | 冰箭回能 **×2**；射线期间每 0.5s 自动触发冰箭，且灌注期**一次触发两支** |
| `SwiftCastBuff` | 施法速度 **+15%** |
| `EquipmentSetEffectBuff_Beam` | 每秒 +2% 施法（累计上限 10%），满 10% 后改为 +5% 主属性 |

#### `EquipmentSetEffectBuff_Beam` 装备套装（最复杂的一个）

| 项 | 值 |
| --- | --- |
| 类型 | 永久固有 |
| 监听 | `CreateSkillAction` |

逻辑分两条：

1. **龙卷必爆**：每来一次 `Flood_Beam` 让 `count` +1，满 `triggerNum`（=2）就把 `canForWSpt` 置真；之后**下一个水龙卷**被赋予 `criticalAdd = 1`（必暴）并开放幸运，**并在赋值的同一处立刻把 `canForWSpt` 置回 false（消费即清除）**。
2. **施法速度成长**：灌注期间每 1s 加 2% 施法，累计到 10% 后改为加 5% 主属性；灌注结束全部回滚。

> 这是**唯一一个把"灌注次数"和"水龙卷"耦合起来**的地方。
>
> - `count` 初值为 1，所以**第 1 次灌注就会解锁**，之后稳态是隔一轮一次；
> - 资格不再依赖 `update()` 的时序（早期版本靠"灌注在场就撤销"，会出现跨轮多给一次的问题）；
> - 5% 主属性的判据是 `totalChange >= 0.1` —— **必须用 `>=`**：5 次 `+= 0.02` 的累加结果与字面量 `0.1` 是**完全相同的 double**，写成 `>` 会永远不成立（那段曾长期是死代码）。

### 4.2 能量相关

| Buff | 机制 |
| --- | --- |
| `IntellectCrystalBuff` | **智力 → 能量上限**：`max_energy += min(getPrimaryAttributes()/50, 100)`；监听智力数量/百分比修改事件，**重算时先减旧的再加新的**（正确的对称做法） |
| `EnergySurgeLawBuff` | 每支冰箭 +2 层计数，**满 50 层**时让接下来一次 `FrostWind` 伤害 **×3**，并上 `EnergyRevertBuff_EnergySurgeLaw` |
| `EnergyRevertBuff_EnergySurgeLaw` | 持续 10s，每 `100/(1+施法)` ms 回 `3% 最大能量` |
| `IceInfiniteBuff` | 玄冰上限 +3；累计消耗能量每满 50 回 1 玄冰；**每 1 玄冰给射线 +5% 增伤**（在玄冰变化时重算，先减旧的再加新的） |
| `FrostCrystalPowerBuff` | 能量 ≥ 50% 上限时 +8% 元素增伤，掉下去就撤（用 `triggered` 标志防重复加减） |
| `UltiIncreaseBuff_Beam` | 大招 Buff：**+45% 冰伤**（构造加、析构减，对称），持续 10s，每秒回 22 点能量 |

### 4.3 玄冰与增伤

| Buff | 机制 |
| --- | --- |
| `IceTideBuff` | 射线出伤**概率回玄冰**：初始 20%，失败则每次 +10% 直到成功；**玄冰已满时改为上 3s 的 `ElementIncreaseBuff_IceTide`** |
| `ElementIncreaseBuff_IceTide` | 玄冰满溢转化来的元素增伤：**+5% 元素，持续 3s**（对称回滚） |
| `NaturalEnergyRegenBuff` | **自然回能**：每 `1000/(1+急速)` ms 回 2 点能量（常驻固有） |
| `BeamMagnumOpusBuff` | 射线每次出伤都尝试上 `DoubleProficientBuff`（精通翻倍） |
| `DoubleProficientBuff` | 持续 3s；把当前精通点数**再加一遍**（即翻倍），并监听精通修改事件做重算 |
| `FrostwindFocusBuff` | 持续 15s，**射线耗能 -27%**（`number = 0.03 × 9`） |
| `BeamBuildBuff` | 射线出伤 13 次减 1s 灌注 CD；**冰箭暴击时创建 `FrostBurst`** |
| `IcePromiseBuff` | 让 `Vortex` 的回能 ×3 |
| `FrostCrystalResonanceBuff` | 见上（灌注期冰箭 ×2 回能 + 自动冰箭） |

### 4.4 幸运相关（`flowConfig = 1` 时启用）

> 注意：本节里 `IceArrowLuckyRealBuff` 是**幸运流专属**，而 `FloodLuckyBuff` 与 `VortexLuckyDoubledBuff`
> 在**两个流派都注册**。

| Buff | 机制 |
| --- | --- |
| `IceArrowLuckyRealBuff` | 构造：`p->luckyFinalIncrease += 0.3`（析构 -0.4，**注意不对称**）；监听创建技能，命中冰箭时 `setCanTriggerLucky(true)`。它原本还负责在灌注时创建 `FloodLuckyBuff`，现在那段已注释（`FloodLuckyBuff` 已改为固有 Buff），但构造函数仍注册着 `CreateBuffListener` → 死监听器 |
| `FloodLuckyBuff` | **固有 Buff，随灌注开关生效**：监听 `CreateBuffAction`，当灌注（`FloodBuff_Beam`）被创建时 `luckyDreamIncrease += 0.35`、暴击 **-5%**、幸运 **+8%**；`update()` 里发现灌注消失就全部回滚。在**两个流派都注册** |
| `VortexLuckyDoubledBuff` | 把当前幸运点数**再加一遍**（幸运值翻倍），并让 `luckyMultiplyingExtra += 0.2`；监听幸运修改事件重算 |
| `IceRealBuff` | 射线每 15 次出伤 → 上 `NineIceBuff` |
| `NineIceBuff` | 持续 15s，玄冰上限 +3，射线能量消耗 -2.5%（每次创建技能时改技能） |

### 4.5 心相仪与因子

| Buff | 机制 |
| --- | --- |
| `IllusoryDreamBuff`（迷幻梦境/征服者） | 精炼攻击 +15%；**计数**：射线出伤 +1，且"不在释放技能"时每 0.5s 额外 +1（模拟普攻）；满 12 层给一层 `ATKIncreaseBuff_IllusoryDream` |
| `ATKIncreaseBuff_IllusoryDream` | 可叠 13 层，每层 +50 攻击，持续 8s；用 `lastStack` 比较实现"层数变化才重算" |
| `ConquerorBuff`（叠势迸破） | 射线出伤 8 次 → 上一层 `StackMomentumBuff` |
| `StackMomentumBuff`（叠势） | 最多 5 层，持续 8s，`number = 74` |
| `OccupationalFactorBuff_Beam`（职业因子） | 给冰箭/霜爆 `dreamIncreaseAdd += 0.211`、给射线 `+= 0.097`；冰箭灌注期 +6 层（否则 +2 层），满则回因子能量；灌注技能回 400 因子能量；每秒回 20 因子能量。**当前未注册**（`registerBuffs` 里没有它）；析构曾有多余的 `PrimaryAttributesPercent/Count` 回滚，已删除 |
| `FloatingExtraSecondaryAttributesBuff_Beam` | 找出**当前数值最高的副属性**并给它 +800 点；监听 5 种副属性修改事件，变化时重算 |
| `OtherExtraEnhanceBuff` | 通用额外增效：构造 `dreamIncrease += 0.03×3`、`attackIncrease += 0.03×7`（析构对称回滚）；创建技能时给**水龙卷** `dreamIncreaseAdd += 0.112×10`，给**射线** `+= 0` |

### 4.6 计数触发型

| Buff | 触发条件 | 结果 |
| --- | --- | --- |
| `FrostDecreePulseBuff` | 射线出伤累计 **50 次** | 创建 `FrostDecreePulse`（5 倍率瞬发） |
| `InstantCooldownBuff_Beam` | 能量回复累计 **35 点** | 冻结寒风 CD **-1300ms** |
| `ExtensiveArrow`（广域冰箭） | **只在水龙卷 / 冻结寒风 / 涡流的命中里计数**（`count` 上限 `triggerMax = 20`，超出丢弃）；命中累计满 `triggerNum = 10` 且距上次触发 ≥`triggerInterval = 500ms` → `count -= 10`、`timer = 0` 并创建冰箭。判定已收在技能过滤**内部** | 创建 `IceArrow_Beam` |
| `FantasyImpactBuff_Beam` | **幸运命中**叠层（非幻想冲击自身），且计时器累计 ≥10s；层数满 20+20 才触发 | 创建 `FantasyImpact_Beam`；每 10 层额外上 `ExtremeLuckBuff_Beam` |
| `ExtremeLuckBuff_Beam` | 由上面触发 | `luckyFinalIncrease += 0.4` |

> ⚠️ `IllusoryDreamBuff` 的析构是 `RefineATKCountModifyAction(-number * 0.1)`，而构造是 `+number * 0.15` —— **加减不对称（0.15 vs 0.1）**，看起来是笔误。

### 4.7 其它因子类

| Buff | 机制 |
| --- | --- |
| `WaterSpoutRealBuff`（3 风因子） | 创建水龙卷时把 **`baseDamageTriggerInterval /= 3`**，再按当前施法速度同步 `damageTriggerInterval`（这就是"三风"的来源：间隔变 1/3 → 出伤次数约 ×3）。两个**未实装项**：`number`（注释写"龙卷伤害增加"，实际恒为 `0.0`）、`listenerCallback2`（原位是"每次命中额外打两段"，现已整段注释） |
| `VortexLuckyDoubledBuff` | 见 4.4 |
| `CoefficientAdjustmentBuff_Beam` | 系数修正（当前未启用） |

---

## 第五节 冰矛（Icicle）技能

| 技能 | 形态 | 倍率 / 固定值 | 前摇 | CD | 关键行为 |
| --- | --- | --- | --- | --- | --- |
| `Flood_Icicle` 灌注 | Instant | 0 | 1000 | 40500 | 上 `FloodBuff_Icicle`，回能 6 |
| `Ultimate_Icicle` 大招 | Instant | 12.6 / 5400 | 100 | 54000 | 上 `UltiIncreaseBuff_Icicle`，回能 22 |
| `IceArrow_Icicle` 冰箭 | Instant | 0.28 / 0 | — | 0 | 无前摇，打两段，创建时回 1 玄冰 |
| `Spear` 冰矛 | Instant | 2.1 / 870 | 400÷(1+攻速) | 0 | 耗能 10、耗玄冰 1 |
| `PierceSpear` 贯穿冰矛 | Instant | 2.5 / 0 | — | 0 | 无前摇，**打三段** |
| `Meteorite` 陨星 | Instant | 1.0266 / 405 | 1000÷(1+攻速) | 13500（2 层充能，充能 CD 500） | **打四段** |
| `SynergyMeteorite` 陨星连携 | Instant | 1.0266 / 405 | 0 | 0 | 无前摇，**打四段**，`finalIncreaseAdd = -0.5` |
| `WaterDrop` 水珠 | Instant | 0 | 800÷(1+攻速) | 22500（2 层充能） | 上 `IceRevertBuff` |
| `FrostComet` 冰霜彗星 | Instant | 2.5 / 0 | — | 0 | 无前摇 |
| `FantasyImpact` 幻想冲击 | Instant | 12.5 / 0 | — | 0 | 无前摇，**50% 概率再打一次** |

### 5.1 `Spear` 冰矛（核心）

```
energyReduce = 10,  canTriggerLucky = true
damageIncreaseAdd = 1.06, elementIncreaseAdd = 0.1, criticalIncreaseAdd = 0.13
构造时 ResourceConsume(1)
```

`trigger()` 顺序：

1. `EnergyConsume(10)`
2. **如果场上有 `DoubleSpearBuff`，先额外打一段**
3. 再打一段 `AttackAction`（本体）
4. `EnergyRevert(5)`（耗能的一半）

> 所以 `DoubleSpearBuff` 是"冰矛双倍"的实现方式——**不是改倍率，而是多一次出伤**。

### 5.2 `Meteorite` 陨星

```
canCharge = true, maxStack = 2, MaxCD = 13500, MaxchargeCD = 500
multiplying = 1.0266, fixedValue = 405
```

`trigger()` **有分支**：

- **如果场上有 `EndlessColdBuff`**：把它的 `number` 写到自己的 `damageIncreaseAdd`，把它的层数 × 0.05 写到 `finalIncreaseAdd`（**临时加成，出伤后立刻清零**）
- **否则**：创建一支冰箭

然后**连打四段** `AttackAction`。

> 注意 `damageIncreaseAdd` / `finalIncreaseAdd` 在结束时被置 0 —— 这是"只对本技能这一次释放生效"的写法，但**因为技能实例是复用的，要小心重入**。

### 5.3 `SynergyMeteorite` 陨星连携

由 `MeteoriteSynergyBuff` 触发（冰矛"暴击且幸运"且陨星层数已满时）。`finalIncreaseAdd = -0.5` 表示它自带 **-50% 最终增伤**（比陨星弱）。

### 5.4 `FrostComet` 冰霜彗星

`damageIncreaseAdd = p->luckyDamageIncrease`（构造快照幸运），由 `FrostCometBuff` 以 21% 概率在冰矛命中时触发。

### 5.5 `FantasyImpact` 幻想冲击（冰矛）

`damageIncreaseAdd = p->luckyDamageIncrease`、`dreamIncreaseAdd = 1.0`，**50% 概率二重触发**（与 Beam 版相反，那边是注释掉的）。

---

## 第六节 冰矛 Buff

### 6.1 灌注与核心增伤

| Buff | 机制 |
| --- | --- |
| `FloodBuff_Icicle` | 灌注；`isInherent = true`（是常驻**监听型**，不是限时） |
| `SpearCritialToRevertIceBuff` | **冰矛暴击时以 30% 概率创建一支冰箭** |
| `SpearCritialBuff` | 冰矛暴击相关增伤（非固有，由其他链路触发） |
| `EndlessColdBuff`（无尽苦寒） | 最多 40 层、持续 10s；`update()` **每 tick 刷新陨星 CD**（所以只要它在场，陨星几乎无 CD）；同时被陨星读取，给陨星加增伤 |
| `UltiIncreaseBuff_Icicle` | 大招：+45% 冰伤，持续 10s（对称加减） |
| `MeteoriteRefreshBuff` | **冰矛"暴击且幸运"且陨星层数未满时，刷新陨星 CD** |
| `MeteoriteSynergyBuff` | **冰矛"暴击且幸运"且陨星层数已满时，创建陨星连携** |
| `FrostCometBuff` | 冰矛命中时 **21% 概率** 触发冰霜彗星 |
| `DoubleSpearBuff` | 让冰矛多打一段 |
| `IceRevertBuff` | 由水珠触发（回玄冰） |
| `ConsumedEnergyCountBuff` | 累计消耗能量，**每满 25 点减少水珠 CD 1000ms** |
| `EquipmentSetEffectBuff_Icicle` | 套装效果（监听出伤） |
| `FantasyImpactBuff` | 幸运叠层触发幻想冲击；`triggerInterval` 额外 **+5000ms 内置 CD**（与 Beam 版的 10s 不同）；每 10 层上 `ExtremeLuckFactor` |
| `ExtremeLuckFactor` | `PrimaryAttributesPercentModifyAction(+0.1)`，持续 5s（对称回滚） |
| `OccupationalFactor_Icicle` | **灌注期幸运伤害 ×1.583**；给冰霜彗星/贯穿冰矛 `dreamIncreaseAdd += 0.35` |
| `FloatingExtraSecondaryAttributesBuff_Icicle` | 最高副属性 +800 |
| `InstantCooldownBuff_Icicle` | 能量回复累计 35 → 减 CD |
| `PierceSpearBuff` / `SimulateNormalAttackToRevertIceBuff` / `UltiIncreaseBuff_Icicle` | 见上表 |

> **冰矛与射线在"灌注"上的语义不同**：射线灌注是**限时 15s 的窗口**，冰矛灌注是**常驻监听型**。对比两个职业的节奏时这是关键差异。

---

## 第七节 幻想（FightingFantasy）

幻想的特殊之处：**每个幻想同时是"技能"和"被动"**，用多重继承实现——

```cpp
class MukuChief : public FightingFantasy, public InstantSkill
```

`Person::equipSkill` 会 `dynamic_cast<FightingFantasy*>`，成功后调用 `setPassiveEffect(this)`——**这就是"装上就生效"的被动来源**。

### 7.1 幻想总表

| 幻想 | 主动 CD / 层数 | 主动效果（Buff） | 被动效果（`setPassiveEffect`） |
| --- | --- | --- | --- |
| `MukuChief` 姆克头目 | 60000 / 2 层 | `MukuChiefBuff`：暴击点数 **+44800**、爆伤 **+40%**，20s | 爆伤 **+25%** |
| `MukuScout` 姆克尖兵 | 80000 / 1 层 | `MukuScoutBuff`：攻击增加 **+15%**，20s | 攻击增加 **+15%** |
| `BYS` 博伊斯 | 80000 / 2 层 | `BYSBuff`：攻击增加 +26%、攻速 +10%，20s | 主属性百分比 **+15%** |
| `YGLWS` 伊戈雷乌斯 | 60000 / 2 层 | `YGLWSBuff`：暴击点数 +56000、暴击 **+14%**，20s + 暴击上限修正 | 爆伤 **+35%** |
| `SXMQ` 嗜血毛球 | 80000 / 1 层 | `SXMQBuff`：见 7.2 | **无被动**（但构造时直接上 `SXMQBuff_Passive`） |
| `HYXZ` 幻妖蟹蛛 | 80000 / 1 层 | `HYXZBuff`：精通 **+20%**，20s | 无（代码被注释） |
| `LSZZ` 掠食蜘蛛 | 80000 / 1 层 | `LSZZBuff`：攻击增加 **+35%**，20s | 攻击增加 **+21%** |
| `YZ` 游子 | 80000 / 1 层 | `YZBuff`：幸运 **+20%**，20s | 无（代码被注释） |
| `JQR` 机器人 | 80000 / 2 层 | `JQRBuff`：精炼攻击 = 总攻击 × (精通/5 × 1.2)，且**专精/特定技能 +30% 增伤**，20s | 精通 **+15%**（走「重新归类」，见 7.4） |

**共性**：主动技能本身倍率为 0（不直接出伤），作用全靠上 Buff；`releasingTime` 都是 100ms 且 `/= (1 + attackSpeed)`。

### 7.2 `SXMQ` 嗜血毛球（最特殊的两个）

**`SXMQBuff`（主动）**：监听 `CreateSkillAction`，对每个被创建的技能做：

- 如果技能类型含 `PARTICULAR` 或 `SPECIALIZED` → `skill->fixedValue += 0.78 × ATK × (1+攻击增加)`
- 如果技能能幸运 → `skill->luckyFixedValue += 同样的一坨`

> **这是"按攻击力折算固定值"的挂载点**——把毛球的 78% 攻击力加成塞进特定技能的固定值区。

**`SXMQBuff_Passive`（被动）**：不是"技能触发"，而是**自己造一个临时 Person**用于查技能类型：

```cpp
this->tempPerson = std::make_unique<temp_Person>();   // 只为拿技能类型元信息
...
auto skill = SkillCreator::createSkill(info.skillName, this->tempPerson.get());
// 判断类型：含 PARTICULAR / SPECIALIZED 就叠层，满 10 层打一发 4.0 倍率的临时技能
```

它有 **`inCallback` 防重入标志**（因为它在出伤回调里又创建技能，会再次触发出伤回调）。

### 7.3 `YGLWSBuff` 的暴击上限修正（唯一一处"上限"逻辑）

```cpp
targetReduction = (暴击 > 0.6) ? (暴击 - 0.6) * 0.3 : 0;
// 超出 60% 的部分，按 30% 折算成爆伤补偿
```

`update()` 里比较 `lastReduction`，只有变化才加减——**这是项目里少数几处正确做了"变化量重算"的地方**。

### 7.4 `JQR` 机器人：被动为什么要「重新归类」

**问题**：机器人的被动是「精通 +15%」，但游戏里这 15% **会直接加进站街面板**。玩家输入的就是站街面板值，所以输入里**已经包含**了这 15%。

如果就这么拿面板值反解精通点数，会有两个后果：

1. 这 15% 被当成**点数**放大（`count = coeff·y/(1−y)`，高精通区间会被显著放大）；
2. 它会被 `DoubleProficientBuff`（双倍精通）一起翻倍——但它本质是**独立的额外百分比**，不该被翻倍。

**解法**：把这段百分比从「点数通道」挪到「额外百分比通道」，**属性总值保持不变**。

```
属性 = base + conv(count) + ExtraPercent
可点数化部分 = 属性 - base - ExtraPercent
```

对应的 API 在 `Person`：

| 函数 | 作用 |
| --- | --- |
| `FantasyPassiveBonus` | 声明一组「面板已含」的百分比被动（精通/急速/暴击/幸运/全能） |
| `Person::applyFantasyPassiveBonus(bonus, apply)` | 应用 / 移除（内部按属性分派） |
| `Person::reclassifyXxxPercentToExtra(percent)` | 单项重新归类；传负值即反向 |

`JQR` 的实现只有三步：

```cpp
// FightingFantasy/Skill.h
static constexpr double kPassiveProficient = 0.15;

// FightingFantasy/Skill.cpp
void JQR::setPassiveEffect(Person *p)
{
    FantasyPassiveBonus bonus;
    bonus.proficient = JQR::kPassiveProficient;
    p->applyFantasyPassiveBonus(bonus, true);   // 站街面板已含 → 只切通道，不加总值
}
```

**`weaponConfig` 用的是同一套机制**：武器的第一/第二属性也在面板里，同样要 `reclassify` 而不是 `-= 0.06`。

> ⚠️ **`reclassifyXxx` 仅限初始化期调用**（`Person` 构造 / `Initializer::equipSkills`）。
> 它直接改成员变量、**不广播 Action 事件**，因为"总值不变、只是通道切换"不是一次属性变更。
> 若在战斗中调用，`DoubleProficientBuff` / `FloatingExtraSecondaryAttributesBuff*` / `JQRBuff`
> 的缓存值会脱钩，而且**不会报错**。

**数值效果示例**（面板精通 74.8%、武器 6%、机器人 15%、代码 +10%）：

| 步骤 | 精通总值 | `ExtraPercent` | `ProficientCount` | 可点数化 |
| --- | --- | --- | --- | --- |
| 面板 | 74.8% | 0 | — | — |
| 武器归类 6% | 74.8% | 6% | 262,963 | 62.8% |
| 代码 +10% | 84.8% | 16% | 262,963 | 62.8% |
| 机器人归类 15% | 84.8% | 31% | **183,140** | **47.8%** |
| 最高副属性 +3.5% | 88.3% | 34.5% | 183,140 | 47.8% |
| 双倍精通（射线期间） | **105.18%** | 34.5% | 翻倍后 64.68% | — |

---

## 第八节 交叉引用：谁触发谁（改动前的必查表）

这一节是**改数值前最有用的部分**——它回答"我动了这个，什么会跟着变"。

### 8.1 触发关系（上游 → 下游）

```
Flood_Beam ──> FloodBuff_Beam（灌注窗口）
                    ├──> SwiftCastBuff            (+15% 施法)
                    ├──> ChillPersistenceBuff     (射线 -25% 耗能)
                    ├──> FrostCrystalResonanceBuff(冰箭 ×2 回能 + 每0.5s 自动冰箭)
                    ├──> EquipmentSetEffectBuff   (施法成长 + 龙卷必爆计数)
                    └──> FloodLuckyBuff           (+35% 幸运增伤 / +8% 幸运 / -5% 暴击，灌注消失即回滚)

射线出伤 ──┬──> BeamBuildBuff        (13 次 → 减灌注 CD；冰箭暴击 → 霜爆)
           ├──> IceTideBuff           (概率回玄冰 / 满则元素增伤)
           ├──> IceInfiniteBuff       (玄冰数 → 射线增伤)
           ├──> BeamMagnumOpusBuff    (→ 精通翻倍)
           ├──> ConquerorBuff         (8 次 → 叠势)
           ├──> IceRealBuff           (15 次 → 9 冰)
           ├──> FrostDecreePulseBuff  (50 次 → 冰令脉冲)
           └──> IllusoryDreamBuff     (12 层 → 攻击加成)

冰箭 ──┬──> EnergySurgeLawBuff     (每支 +2 层，50 层 → 寒风 ×3)
       ├──> FrostCrystalResonance  (灌注期回能 ×2)
       ├──> OccupationalFactor     (因子能量)
       └──> (冰箭暴击) → BeamBuildBuff → FrostBurst

冰矛暴击 ──┬──> SpearCritialToRevertIceBuff → 冰箭
           ├──> (暴击+幸运, 陨星未满) → MeteoriteRefreshBuff
           └──> (暴击+幸运, 陨星已满) → MeteoriteSynergyBuff → 陨星连携

幸运命中 ──> FantasyImpactBuff  ──> 幻想冲击 / 极运
```

### 8.2 三处"看起来像 bug"的对称性问题（改前先确认是否有意）

| 位置 | 现象 |
| --- | --- |
| `IllusoryDreamBuff` | 构造 `+number × 0.15`，析构 `-number × 0.1` —— **不对称** |
| `IceArrowLuckyRealBuff` | 构造 `luckyFinalIncrease += 0.3`，析构 `-= 0.4` —— **不对称** |
| `FantasyImpact_Beam` | `dreamIncreaseAdd` 连续赋值 `1.0` → `0.15`，**前者无效** |

### 8.3 加减配对的核对结论

| 类别 | 结论 |
| --- | --- |
| **限时 Buff**（`FloodBuff_Beam`、`MukuChiefBuff`、`MukuScoutBuff`、`BYSBuff`、`YZBuff`、`HYXZBuff`、`LSZZBuff`、`UltiIncreaseBuff_*`、`ElementIncreaseBuff_IceTide` 等） | **都做了析构回滚**，配对正确。例如 `~FloodBuff_Beam` 明确有 `QuicknessCountModifyAction(-1040)` 与 `QuicknessPercentModifyAction(-0.21)` |
| **永久固有 Buff** | 多数不回滚（终身存在），属正常设计 |
| **随条件开关的 Buff** | `FloodLuckyBuff` 不是靠析构，而是 `update()` 里检测"灌注消失"后回滚（`triggered` 标记防重复） |
| ⚠️ `IllusoryDreamBuff` | 构造 `+number × 0.15` vs 析构 `-number × 0.1` —— **不对称** |
| ⚠️ `IceArrowLuckyRealBuff` | 构造 `+= 0.3` vs 析构 `-= 0.4` —— **不对称** |

**关于速度的一条注意**：`FloodBuff_Beam` 的急速加成会改变 `p->castingSpeed`，从而触发 `modifySkillInterval()` 重算
标了 `canAffectedBySpeed` 的技能（射线、水龙卷）的出伤间隔。改灌注频率会连带影响出伤频率。

---

## 附录 A：快速定位表

| 想改的东西 | 去哪里 |
| --- | --- |
| 射线本体倍率/间隔/耗能 | `Mage/Beam/Skill.cpp` → `Beam::Beam` 与 `Beam::trigger` |
| 灌注窗口长短/回能 | `Mage/Beam/Buff.cpp` → `FloodBuff_Beam` |
| 灌注期各种加成 | 搜 `findBuffInBuffList(FloodBuff_Beam::name)` |
| 三风（龙卷出伤频率） | `WaterSpoutRealBuff::listenerCallback1` 的 `baseDamageTriggerInterval /= 3` |
| 龙卷必爆 / 可幸运 | `EquipmentSetEffectBuff_Beam::listenerCallback` |
| **幻想的「面板已含」被动（如机器人 +15% 精通）** | `Person::applyFantasyPassiveBonus` + 各幻想的 `setPassiveEffect`，见 7.4 节 |
| **武器第一/第二属性（面板已含的部分）** | `core/Person.cpp` 构造函数里的 `weaponConfig` 分支（同样走 `reclassify`） |
| 属性「点数 ↔ 额外百分比」的通道切换 | `Person::reclassifyXxxPercentToExtra`（**仅限初始化期调用**） |
| 冰箭本体（倍率 / 必暴 / 精通增伤 / 玄冰回复） | `Mage/Beam/Skill.cpp` → `IceArrow_Beam::IceArrow_Beam` |
| 冰箭触发源 | 搜 `IceArrow_Beam::name` 的 `CreateSkillAction`；广域冰箭见 `ExtensiveArrow` |
| **自动战斗轴（阶段顺序 / 时长）** | `Mage/Beam/AutoAttack.cpp` → `setupHardcodedOutBurstAxis()` 与 `registerOutBurstLogic()`，见 3.8 节 |
| **施法速度对出伤频率的影响** | `core/Person.cpp` → `Person::modifySkillInterval()`；技能侧看 `canAffectedBySpeed` |
| **灌注期的幸运增伤（+35% / +8% 幸运 / -5% 暴击）** | `Mage/Beam/Buff.cpp` → `FloodLuckyBuff`（监听 `CreateBuffAction` 的 `FloodBuff_Beam`） |
| 流派切换（急速精通/幸运） | `Mage/Beam/Initializer.hpp` → `registerBuffs()` 的 `m_flowConfig` 分支 |
| 幻想组合 | `Mage/Beam/Initializer.hpp` → `equipSkills()` 的 `m_fantasyConfig` 分支 |

> ⚠️ **切换流派时注意**：`main_Beam.cpp` 里虽然有个 `int flowConfig = 0;` 变量，但**它没有传给 `Initializer_Mage_Beam`**（构造调用只传了 `fantasyConfig`），所以改那个变量**不生效**——真正的流派来自 `Initializer_Mage_Beam` 构造函数的默认实参 `flowConfig = 0`。要切换必须写成 `std::make_unique<Initializer_Mage_Beam>(p.get(), deltaTime, fantasyConfig, flowConfig)`。

## 附录 B：新增一个 Buff/Skill 的最小步骤

以新增 Buff 为例：

1. 在 `<职业>/Buff.h` 里声明类，**并写 `static std::string name;`**
2. 若是固有 Buff，**必须同时**加 `static constexpr bool kInherent = true;`（编译期判定）**和**构造函数里 `this->isInherent = true;`（运行期标记）——两者缺一都不对
3. 在 `<职业>/Buff.cpp` 里：定义 `name`、写构造函数、**实现 `update` / `shouldBeRemoved` / `getBuffName`**
4. **若构造函数里 `addListener`，析构里必须 `deleteListener`**
5. **若构造函数里 `triggerAction` 加属性，析构里必须减回去**（数值要严格对称）
6. 在 `<职业>/Initializer.hpp` 的 `registerBuffs()` 里注册
7. 固有 Buff 靠 `kInherent` 自动进 `inherentBuffs`，无需手动加
8. `CMakeLists.txt` 已按目录通配，通常无需改动

> Debug 构建下 `Initializer::Initialize()` 末尾会断言 `kInherent` 与 `isInherent` 一致——忘了第 2 步会直接报错。
> ⚠️ 但**只有 `isInherent` 而没有 `kInherent` 时不会报错**：`registerCertainBuff` 里是 `if constexpr`，
> 缺 `kInherent` 就根本不会进 `inherentBuffs`，这个 Buff 会**静默地永不创建**（`FloodLuckyBuff` 曾踩过这个坑）。

**新增 Skill 时额外注意时间字段**：

9. 若要放进 CD 列表（写进 `equipSkills()`），构造里只需设 `baseDamageTriggerInterval`，并让 `damageTriggerInterval = base / (1 + 施法速度)`；
10. **若希望它在战斗中跟着施法速度变快**，必须设 `canAffectedBySpeed = true`，否则只有构造时那一次除法；
11. 纯瞬发技能（`InstantSkill`）的 `damageTriggerInterval` 实际不参与判定，但保持同样的写法以免将来改形态时出错。
