# 星痕共鸣可拓展DPS模拟

这是一个基于`C++17`、`cmake`、`Qt6`构建的可扩展DPS模拟器，主要用于模拟《星痕共鸣》中的3分钟输出循环。  

程序模拟数据与游戏内实际数据会有所出入。伤害计算中暴击/幸运采用**期望值模型**（不逐次掷骰），数据波动主要来自技能触发、Buff生效时机等程序随机；为提高精度，建议调高循环次数。

射线因为爆发轴已经硬编码成固定顺序（见 `AutoAttack_Mage_Beam_Base::setupHardcodedOutBurstAxis`），程序随机几乎被压没，单次就很稳；冰矛的随机触发更多，20 次循环的波动约 2.7%，要提高精度请继续加大循环次数。

如果你要比较**两个配置的提升**，不要靠"多跑几次比均值"——用下文的**对比模式**，同种子配对能把噪声抵消掉几个数量级。

本程序不能用于对自身配置的预测，而应用于对于后续未知配置的提升推测，并根据游戏变动适当拓展。  

程序内置射线与冰矛两套职业实现（控制台），框架支持继续拓展；图形化界面目前仅接入射线。  

本程序有一个简单的图形化页面，没有c++基础的可以使用此图形化页面，但是目前为beta版本可能会导致一系列问题(例如现在实时日志框中无法输出debug信息)，有c++基础者仍建议自行编译并运行dps_simulator_icicle.exe  

## 注意

1.填入属性时，直接按原始站街面板填入即可，关于天赋等各部分带来的面板影响均已考虑进去（除3.5%，3.5%需自行扣除），无需担心
2.此程序为简便自动战斗逻辑做出了不少取舍，例如稳定7灌注，龙卷风无限持续时间，射线能量稳定不断等，因此根据此程序得到的提升需考虑实际情况

## 快速开始(简单版)

> **注意**：仓库源码不附带编译好的 exe，clone 下来后需自行编译（见下文「快速开始(复杂版)」）。
> 若拿到的是已编译版本，GUI 程序位于 `build-msvc-review\bin\Release\dps_simulator_gui.exe`
> （该目录同时带有运行所需的 `Qt6*.dll`，**请保持 exe 在原目录内运行，不要单独拷走**）。
> 也可直接用 `build-msvc-review\bin\Release\dps_simulator_icicle.exe` 运行控制台版冰矛。

1.双击 `build-msvc-review\bin\Release\dps_simulator_gui.exe`  
2.勾选随机种子  
3.填入对应数据(数据应为站街面板，且应自行扣除3.5%)
4.点击下方的运行模拟  
5.滑动下方的统计数据栏查看数据  

### 环境要求

支持C++17 或更高版本  
Visual C++ 2015-2022 Redistributable  
下载地址：<https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170>  

## 快速开始(复杂版)

### 环境要求

支持C++17 或更高版本  
CMake 3.16+  
(补充：若不使用cmake等构建工具而使用vscode的某些runner插件，例如compile runner，需自行修改程序链接，或改为直接include对应的.cpp文件)  
GCC/Clang/MSVC 支持C++17  
Qt 6.10.2及以上（**必须与编译器匹配**：MSVC 编译器配 MSVC 版 Qt，MinGW 编译器配 MinGW 版 Qt）  
（控制台程序已在 MinGW-w64 GCC 8.1.0 下验证通过；GUI 需配套的 MinGW 版 Qt）

> **MinGW 现状（实测）**
>
> - 控制台目标（`beam` / `beam_max` / `icicle` / `compare`）用 MinGW-w64 GCC 8.1.0 + Qt 6.10.2 mingw_64 **已可正常编译运行**（配置方式见下文 MinGW 段）。
> - **GUI 目标在 GCC 8.1.0 下编不过**，会停在 Qt 头文件里的
>   `static assertion failed: Use *_NON_NOEXCEPT version of the macro`（`qmetatype.h` / `qvariant.h` / `qurl.h` / `qpen.h` / `qlocale.h`）。
>   原因是 Qt 6.10 要求编译器支持相应的 `__cpp_lib_*` 特性宏，而 GCC 8.1 太旧——**这是工具链版本问题，不是程序代码问题**。
> - 想用 MinGW 编 GUI，需要 **GCC 11 及以上**的 MinGW，并且 Qt 也要用同一个编译器重新构建（混用不同 GCC 大版本的 Qt 会在链接期报 ABI 不兼容）。
> - 只想跑控制台（或手头只有 GCC 8.1）时，GUI 编不过不影响使用，也可以直接用 MSVC 编 GUI。

### 编译运行

#### Windows / MSVC

```powershell
# 在项目根目录
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release --target all --parallel 4

# 运行程序
# 冰矛
./build/bin/Release/dps_simulator_icicle.exe
```

#### Windows / MinGW-w64 GCC

```powershell
# 在项目根目录
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target all --parallel 4

# 运行程序
# 冰矛
./build/bin/dps_simulator_icicle.exe
```

> **注意**：Qt 库必须与编译器配套。如果使用 MinGW 编译器，请安装 MinGW 版 Qt
> （路径通常形如 `E:/Qt/6.10.2/mingw_64`），并在配置时通过 `-DCMAKE_PREFIX_PATH=E:/Qt/6.10.2/mingw_64` 指定。
> 若 CMake 找到的是 MSVC 版 Qt，会**自动跳过 GUI 目标**并给出警告，仅构建控制台程序
> （icicle/beam/compare），此时不需要 Qt 也能编译。
>
> 不要混用构建目录：同一 `build` 目录只能使用一种生成器。若之前用 VS 生成器配置过，
> 再切换到 MinGW Makefiles 前请删除 `build/` 后重新配置。

#### Linux / macOS

```bash
mkdir build && cd build
cmake ..
make

# 运行程序
# 冰矛
./bin/dps_simulator_icicle
```

### 选择对应main.cpp

`main_icicle.cpp`对应冰矛
`main_beam.cpp`对应射线

### 配置角色属性

以冰矛为例，在`main_icicle.cpp`中修改`Mage_Icicle`构造函数的参数，例：
（填入数据同上文要求）

```cpp
/*main.cpp*/
std::unique_ptr<Mage_Beam> p = std::make_unique<Mage_Beam>(
    /*三维属性*/ xxx,
    /*暴击(%)*/ xx.xx,/*例如51.63*/
    /*急速(%)*/ xx.xx,
    /*幸运(%)*/ xx.xx,
    /*精通(%)*/ xx.xx,
    /*全能(%)*/ xx.xx,
    /*攻击(物理攻击/魔法攻击)*/ xxx,
    /*精炼攻击*/ xxx,
    /*元素攻击*/ xx,
    /*攻击速度(%)*/ xx.xx,
    /*施法速度(%)*/ xx.xx,
    /*爆伤额外值*/ 0.x,/*例如0.2*/
    /*增伤额外值*/ 0.x,
    /*元素增伤额外值*/ 0.x,
    /*程序运行总tick*/ maxTime,
    /*幻想配置*/ 0
);
// 由于冰法的3.5会直接加在站街面板中，在向模拟输入数据时，记得去掉3.5
```

## 对比模式（同种子配对 · CRN）

用于比较"基准配置"与多个"候选配置"的DPS提升，例如判断"+5%暴击值不值"、"+100攻击还是+10%精通收益高"。

### 原理

每次模拟因程序随机（技能触发/Buff时机/能量循环）会有波动。普通比较是各自独立跑N次再比均值，**两份噪声叠加**，1%的小提升会被淹没。

本模式让**基准与每个候选在相同随机种子下配对模拟**（CRN，公共随机数）：

- 同一秒内两份模拟共享同一串随机"天气"，逐对相减 `Δᵢ = 候选DPS(sᵢ) − 基准DPS(sᵢ)` 时这份随机互相抵消
- Δ 中只剩配置本身带来的差异 → **方差骤降**（实测配对σ 比 基准σ 小 50~100 倍）
- 因此 1% 级的小提升也能稳定测出（20 对时 t 值可达数千）

配对只改变**方差**、不改变均值：Δ 的平均就是两个配置真实 DPS 之差的无偏估计。

### 控制台用法

```bash
./bin/dps_simulator_compare [配对数] [起始种子]
```

- `配对数`：默认 20，建议 20+，越多越稳
- `起始种子`：默认 42，配对使用的种子为 `起始种子 .. 起始种子+配对数-1`

修改 `main_compare.cpp` 三处：

**1. 切职业** —— `useBeam`（`true`=射线，`false`=冰矛）

**2. 填基准** —— `base` 的字段填你的站街面板（可直接从 `main_beam.cpp` / `main_icicle.cpp` 复制构造参数）

**3. 加候选** —— 在 `buildCandidates()` 里按行添加要对比的改动，例如：

```cpp
make([](SimConfig& c){ c.critical += 5; }, "+5%暴击");       // 暴击 +5%
make([](SimConfig& c){ c.atk += 100; }, "+100攻击");          // 攻击 +100
make([](SimConfig& c){ c.proficient += 10; }, "+10%精通");    // 精通 +10%
make([](SimConfig& c){ c.fantasyConfig = 0; }, "换幻想0");     // 幻想：绝对值
```

每个 `make(修改lambda, "标签")` 生成一个候选配置（在基准基础上套用修改）。

**输出表各列含义：**

| 列 | 含义 |
| --- | --- |
| 基准DPS / 候选DPS | 各自 N 对平均 |
| ΔDPS / Δ% | 候选 − 基准，及相对提升 |
| 配对σ | 逐对差值的标准差（CRN 核心指标，**远小于基准σ说明抵消效果好**） |
| t值 | 配对 t 检验统计量，**t 值 ≥ 2 视为差异显著**（N=20 时临界值约 2.09） |

### GUI 用法

1. 在上方面板填好**基准**站街面板
2. 滚动到下方，勾选「**启用对比模式**」（配对次数默认 20）
3. 在「候选配置」框里每行填一个候选，格式 `关键词 增量`（`#` 开头为注释）：

   ```text
   # 每行一个候选：关键词 增量
   暴击 +5
   攻击 +100
   精通 +10
   # 幻想用绝对值：0-7=具体幻想（对应上方"幻想配置"下拉框的序号），如：幻想 5
   ```

4. 点「运行模拟」，结果表显示每个候选的 基准DPS/候选DPS/ΔDPS/Δ%/配对σ/t值（ΔDPS、Δ% 绿涨红跌）

**候选关键词表：**

| 关键词 | 对应属性 | 说明 |
| --- | --- | --- |
| 三维 / primary | 三维属性 | 增量 |
| 暴击 / crit | 暴击(%) | 增量 |
| 急速 / quickness | 急速(%) | 增量 |
| 幸运 / lucky | 幸运(%) | 增量 |
| 精通 / proficient | 精通(%) | 增量 |
| 全能 / almighty | 全能(%) | 增量 |
| 攻击 / atk | 攻击 | 增量 |
| 精炼 / refineAtk | 精炼攻击 | 增量 |
| 元素 / elementAtk | 元素攻击 | 增量 |
| 攻速 / attackSpeed | 额外攻击速度(%) | 增量 |
| 施速 / castingSpeed | 额外施法速度(%) | 增量 |
| 爆伤 / critDmgSet | 爆伤额外值 | 增量 |
| 增伤 / incSet | 增伤额外值 | 增量 |
| 元素增伤 / eleIncSet | 元素增伤额外值 | 增量 |
| 幻想 / fantasy | 幻想配置 | **绝对值**（0-7=具体幻想，见上方"幻想配置"下拉框的8个选项；没有"无幻想"档位） |

> 想看每个序号对应哪两个幻想，见 `Mage/Beam/Initializer.hpp` 的 `equipSkills()`：
> `0`=姆头+尖兵、`1`=姆头+伊戈雷乌斯、`2`=姆头+嗜血毛球、`3`=幻妖蟹蛛+嗜血毛球、`4`=尖兵+嗜血毛球、`5`=掠食蜘蛛+嗜血毛球、`6`=游子+嗜血毛球。

### 使用建议

- 配对数建议 20+；改小属性（暴击/攻击）时路径不变、抵消效果好，改结构（换幻想/自动战斗）时随机流会分叉、抵消打折
- 对比结果用于**提升排序**（哪个配置提升大），而非绝对 DPS 预测
- 基准的 N 对结果只跑一次并缓存（模拟对种子是确定性的），多个候选共享，因此 `候选数 × 配对数` 份模拟量级并不大

## 程序当前配置

### 射线

#### 因子效果

- 固定带9冰、拍地板、冰箭幸运，用于衡量提升，其他因子由于可以直接计算其提升故不考虑

#### 心相仪

- 征服者

## 输出说明

程序运行时会输出以下信息：

### DEBUG信息

```text
[DEBUG,timer=XXXX]: Skill  - skill: Spear - Stack: 1/1
[DEBUG,timer=XXXX]: Buff  - buff: FloodBuff triggered
[DEBUG,timer=XXXX]: Auto  - large out burst
```

### 统计信息

```text
skill Damage Statistics:
Skill: <技能名> | Damage: XXXXX | Damage attack count: XXX | Released Times: XXX | Lucky Damage: XXXXX | Lucky Attack count: XXX | DPS: XXXX | Propotion:X.XX%
...
Lucky: | Damage: XXXXX | Damage attack count: XXX | DPS: XXXX | Propotion:XX.XX%

total Damage Statistics:
 | Total Damage: XXXXXXX | Total DPS: XXXXXX
Total Damage Count: XXXX | Total Lucky Damage Count: XXX | Lucky rate:XX.XX% | Crit rate:XX.XX% | Total Released Times: XXXX
```

> 字段口径说明：
> - `Damage` / `Lucky Damage` 是**期望值**累计（暴击与幸运都按概率折算，不逐次掷骰），`Attack count` / `Lucky Attack count` 是实际触发次数。
> - 循环次数 > 1 时，程序先累加再除以次数，所以汇总表里的每个数字都是**每次的均值**（`Total Damage`/`Total DPS` 同样是均值，不是多次总和）——次数类字段可能出现小数（如 `376.9`）属正常。
> - `Lucky rate` / `Crit rate` 用"幸运/暴击触发次数 ÷ 总命中次数"计算，分母为 0 时按 0 输出（不会出现 `nan`）。

### 性能优化建议

**减少DEBUG输出：**如需提高性能，可注释掉各cpp文件中的日志打印，或将日志类型改为INFO
**批量模拟：**在main.cpp中使用循环进行多次模拟，比较属性提升（更推荐使用"对比模式"，见上文，同种子配对可直接消除程序随机）

## 调试日志（SimulationLog）

程序会把**完整 debug 信息**（技能触发、Buff 创建/移除、爆发阶段、能量变化等）实时写入 `log/` 文件夹下的 txt 文件，方便离线分析一轮模拟的详细过程。

### 行为

- **位置**：`log/` 文件夹固定在**项目根目录**（`StarResonance_Simulator/log/`），Debug/Release 所有模式、从任何目录启动 exe 都统一写到这里
- **文件名**：`log-YYYYMMDD-HHMMSS-<入口>-<编译方式>.txt`
  - 入口：`beam`（射线控制台）、`icicle`（冰矛控制台）、`gui`（图形界面）
  - 编译方式：`Debug` / `Release`
  - 例：`log-20260814-130705-beam-Release.txt`、`log-20260814-130959-icicle-Debug.txt`、`log-20260814-131000-gui-Release.txt`
- **多次模拟只取第一次**：只有首次模拟的 debug 会写入文件，避免文件爆炸
- 文件开头先写**模拟配置头**，再写调试日志，便于对照：
  - Person 构造参数（站街面板）
  - 注册的技能名
  - 注册的Buff名
  - 使用的 AutoAttack 类
- 日志**同步实时写入**（每行即刷），模拟结束后关闭

### 使用

运行 `dps_simulator_beam.exe` 后，到 **项目根目录的 `log/` 文件夹**下用任意文本编辑器打开最新文件：

```text
==== 模拟配置 ====
职业: beam
Person 构造参数: 三维=6760, 暴击=5, 急速=30, ...
注册技能: WaterSpout, IceArrow_Beam, Flood_Beam, ...
注册Buff: SkillReleasedTimeStatistics, ...
AutoAttack: class AutoAttack_Mage_Beam_LSZZ
==== 调试日志 ====
[DEBUG,timer=1]: Auto    - outBurst1 started
[DEBUG,timer=1]: Action  - CreateSkillAction - Skill Created: SXMQ ...
```

典型用途：分析**爆发时间轴**（`outBurstN started` / `Buff Created: FloodBuff_Beam` 的时间戳）、技能释放顺序、能量收支。

> 注意：debug 信息依赖 Logger 的 **DEBUG 级别**（`main_beam.cpp` 已设为 DEBUG）。若关掉 DEBUG，文件里只有配置头、没有调试日志。日志行带 ANSI 颜色码属正常。

## 如何拓展

详见ExtendExample文件夹
若使用cmake在写完后记得在cmake中链接

### 拓展的一些建议

1.对于爆发前开的技能，例如幻想，建议将其releasingTime设置较低，以防延迟进入爆发导致爆发次数不足
2.对于自动战斗，如果职业战斗逻辑为三段式，即大爆发，小爆发，空窗期，可以参考冰矛的autoAttack内的逻辑，不然就需要自己构思自动战斗逻辑了
