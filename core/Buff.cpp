#include "Buff.h"
#include <string>

// 静态成员初始化
int Buff::ID = 0;
std::string Buff::name = "Buff";

// 构造函数
Buff::Buff(Person *p)
    : buffID(Buff::getID()),  // 分配唯一ID
      p(p)
{
    ID++;  // 递增静态ID计数器
}

// 添加层数（堆叠）
void Buff::addStack(const double n)
{
    this->lastStack = stack;  // 保存上一次的层数
    if (this->stack < this->maxStack)
    {
        this->stack += n;     // 增加层数
    }
    else
    {
        this->stack = this->maxStack;  
    }
}

// 减少持续时间
bool Buff::reduceDuration(int deltaTime)
{
    if (this->duration >= 0)
    {
        this->duration -= deltaTime;
        return true;  // 成功减少
    }
    return false;  // 持续时间已耗尽或无效
}

void Buff::stop()
{
    this->duration = 0;
}

// 重载等于操作符：通过ID比较两个Buff是否相同
bool Buff::operator==(const Buff &) const { return ID == this->getBuffID(); }

// 获取下一个可用的BuffID
int Buff::getID() { return Buff::ID; }

// 重置持续时间到最大值
void Buff::resetDuration() { this->duration = this->maxDuration; }

// 访问器实现
std::string Buff::getBuffName() const { return this->name; }
int Buff::getBuffID() const { return this->buffID; }
double Buff::getStack() const { return this->stack; }
double Buff::getMaxStack() const { return this->maxStack; }
double Buff::getLastStack() const { return this->lastStack; }
double Buff::getNumber() const { return this->number; }
double Buff::getDuration() const { return this->duration; }
double Buff::getMaxDuration() const { return this->maxDuration; }
bool Buff::getIsStackable() const { return this->isStackable; }
bool Buff::getAllowDuplicates() const { return this->allowDuplicates; }
bool Buff::getIsInherent() const { return this->isInherent; }

// 重置静态ID计数器（用于重新开始计数）
void Buff::resetID() { Buff::ID = 0; }

// buff何时应该被移除，默认为持续时间结束
bool Buff::shouldBeRemoved() { return this->duration < 0; }
