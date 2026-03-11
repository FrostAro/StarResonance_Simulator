# 星痕共鸣可拓展DPS模拟

这是一个基于`C++17`、`cmake`、`Qt6`构建的可扩展DPS模拟器，主要用于模拟《星痕共鸣》中的3分钟输出循环。  

程序模拟数据与游戏内实际数据会有所出入，并且由于程序内有随机算法，使得数据会有所波动，在运行时建议调高循环次数，目前20的循环次数能保证在2%范围的数据波动，如需更精确数据，则应再次调高模拟次数

本程序不能用于对自身配置的预测，而应用于对于后续未知配置的提升推测，并根据游戏变动适当拓展。  

程序目前仅支持冰矛，但是支持拓展(PS:射线的还在写喵)。  

本程序有一个简单的图形化页面，没有c++基础的可以使用此图形化页面，但是目前为beta版本可能会导致一系列问题(例如现在实时日志框中无法输出debug信息)，有c++基础者仍建议自行编译并运行dps_simulator_icicle.exe  

(孩子第一次写这种东西，代码写的烂，多多见谅谢谢喵)

## 快速开始(简单版)

1.本文件目录找到Icicle_Simulator.exe双击运行  
2.勾选随机种子  
3.填入对应数据(数据应为站街面板)
4.点击下方的运行模拟  
5.滑动下方的统计数据栏查看数据  

### 环境要求

支持C++17 或更高版本  
Visual C++ 2015-2022 Redistributable  
下载地址：<https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170>  

## 快速开始(复杂版)

### 环境要求

支持C++17 或更高版本  
CMake 3.10+  
(补充：若不使用cmake等构建工具而使用vscode的某些runner插件，例如compile runner，需自行修改程序链接，或者怕麻烦也可以include对应的.cpp文件(bushi))  
GCC/Clang/MSVC 支持C++17  
Qt 6.10.2及以上  

### 编译运行

```bash
# 创建构建目录
mkdir build && cd build

# 配置和编译
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
```

## 程序当前配置

### 冰矛

#### 因子效果

- 通用攻击：幸运值+9.27%，急速值-5.56%  
- 通用攻击：暴击值+10%，精通值-6%  
- 通用攻击：智力+70，智力+1.84%  
- 职业专属：冰之灌注持续期间，幸运一击梦境伤害+58.3%  
- 职业专属：贯穿冰矛，冰霜彗星梦境伤害+35%  

#### 心相仪

- 幻想冲击
- 时阶：幻想冲击内置cd+5s，幻想冲击梦境伤害+100%  
- 二重：幻想冲击有50%固定几率额外触发一次  
- 极运：每触发10次幸运一击，主属性+10%，持续5s  

#### 幻想

- 姆头+尖兵
- 姆头+博伊斯
- 姆头+风龙

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
Skill: Spear, Damage: XXXXX, Damage attack count: XXX, Lucky Damage: XXXXX, Lucky Attack count: XXX, DPS: XXXX
...

total Damage Statistics:
Total Damage: XXXXXXX, Total Lucky Damage: XXXXXX, Total DPS: XXXXXX
Total Damage Count: XXXX, Total Lucky Damage Count: XXX, Lucky rate: XX.XX%, Crit rate: XX.XX%
```

### 性能优化建议

**减少DEBUG输出：**如需提高性能，可注释掉各cpp文件中的日志打印，或将日志类型改为INFO
**批量模拟：**在main.cpp中使用循环进行多次模拟，比较属性提升

## 如何拓展

详见ExtendExample文件夹
若使用cmake在写完后记得在cmake中链接

### 拓展的一些建议

1.对于爆发前开的技能，例如幻想，建议将其releasingTime设置较低，以防延迟进入爆发导致爆发次数不足
2.对于自动战斗，如果职业战斗逻辑为三段式，即大爆发，小爆发，空窗期，可以参考冰矛的autoAttack内的逻辑，不然就需要自己构思自动战斗逻辑了
