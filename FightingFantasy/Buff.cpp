#include "Buff.h"
#include "../core/Action.h"
#include "../core/Person.h"
#include "../core/AutoAttack.h"
#include "../core/Logger.h"

// 姆克头目
std::string MukuChiefBuff::name = "MukuChiefBuff";

MukuChiefBuff::MukuChiefBuff(Person *p, double) : Buff(p)
{

    this->duration = 2000;
    this->maxDuration = this->duration;

    Logger::debugBuff(AutoAttack::getTimer(),
                        this->getBuffName(),
                        " - before person Crit Count + 4480, persent: " +
                        std::to_string(p->Critical));

    //p->changeCriticalCount(4480);
    this->p->triggerAction<CriticalCountModifyAction>(4480);

    Logger::debugBuff(AutoAttack::getTimer(),
                        this->getBuffName(),
                        " - after person Crit Count + 4480, persent: " +
                        std::to_string(p->Critical));

    //p->changeCriticalDamage(0.4);
    this->p->triggerAction<CriticalDamageModifyAction>(0.4);
}

void MukuChiefBuff::listenerCallback(double) {}
void MukuChiefBuff::update(const double) {}
bool MukuChiefBuff::shouldBeRemoved() { return this->duration < 0; }
std::string MukuChiefBuff::getBuffName() const { return MukuChiefBuff::name; }

MukuChiefBuff::~MukuChiefBuff()
{
    //this->p->changeCriticalCount(-4480);
    this->p->triggerAction<CriticalCountModifyAction>(-4480);
    //this->p->changeCriticalDamage(-0.4);
    this->p->triggerAction<CriticalDamageModifyAction>(-0.4);
}

// 姆克尖兵
std::string MukuScoutBuff::name = "MukuScoutBuff";

MukuScoutBuff::MukuScoutBuff(Person *p, double) : Buff(p)
{
    this->number = 0.15; // 用作增攻数值
    this->duration = 2000;
    this->maxDuration = this->duration;

    //p->changeAattackIncrease(this->number);
    this->p->triggerAction<AttackIncreaseModifyAction>(this->number);
}

void MukuScoutBuff::listenerCallback(const DamageInfo &) {}
void MukuScoutBuff::update(const double) {}
bool MukuScoutBuff::shouldBeRemoved() { return this->duration < 0; }
std::string MukuScoutBuff::getBuffName() const { return MukuScoutBuff::name; }

MukuScoutBuff::~MukuScoutBuff()
{
    //this->p->changeAattackIncrease(-this->number);
    this->p->triggerAction<AttackIncreaseModifyAction>(-this->number);
}

// 博伊斯
std::string BYSBuff::name = "BYSBuff";

BYSBuff::BYSBuff(Person *p, double) : Buff(p)
{
    this->number = 0; // 用作增攻数值
    this->duration = 2000;
    this->maxDuration = this->duration;
    this->p->triggerAction<AttackIncreaseModifyAction>(0.26);
    this->p->triggerAction<AttackSpeedPercentModifyAction>(0.1);
}

void BYSBuff::listenerCallback(const DamageInfo &) {}

void BYSBuff::update(const double) {}
bool BYSBuff::shouldBeRemoved() { return this->duration < 0; }
std::string BYSBuff::getBuffName() const { return BYSBuff::name; }

BYSBuff::~BYSBuff()
{
    this->p->triggerAction<AttackIncreaseModifyAction>(-0.26);
    this->p->triggerAction<AttackSpeedPercentModifyAction>(-0.1);
}

// 伊戈雷乌斯
std::string YGLWSBuff::name = "YGLWSBuff";

YGLWSBuff::YGLWSBuff(Person *p, double) : Buff(p)
{
    this->number = 0; // 用作增攻数值
    this->duration = 2000;
    this->maxDuration = this->duration;
    this->p->triggerAction<CriticalCountModifyAction>(6800);
    this->p->triggerAction<CriticalPercentModifyAction>(0.12);
}

void YGLWSBuff::listenerCallback(const DamageInfo &) {}
void YGLWSBuff::update(const double) {}
bool YGLWSBuff::shouldBeRemoved() { return this->duration < 0; }
std::string YGLWSBuff::getBuffName() const { return YGLWSBuff::name; }

YGLWSBuff::~YGLWSBuff()
{
    this->p->triggerAction<CriticalCountModifyAction>(-6800);
    this->p->triggerAction<CriticalPercentModifyAction>(-0.12);
}