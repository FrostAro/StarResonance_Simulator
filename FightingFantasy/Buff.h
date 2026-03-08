#pragma once
#include "../core/Buff.h"
#include <memory>

class Skill;
class Person;

class MukuChiefBuff : public Buff
{
public:
    // 姆克头目
    static std::string name;

public:
    void listenerCallback(double n);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    MukuChiefBuff(Person *p, double n);
    ~MukuChiefBuff() override;
};

class MukuScoutBuff : public Buff
{
public:
    // 姆克尖兵
    static std::string name;
    // int damageCount;       //暂时弃用，仅保留主动

public:
    void listenerCallback(const DamageInfo &info);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    MukuScoutBuff(Person *p, double n);
    ~MukuScoutBuff() override;
};

class BYSBuff : public Buff
{
public:
    // 博伊斯
    static std::string name;

public:
    void listenerCallback(const DamageInfo &info);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    BYSBuff(Person *p, double n);
    ~BYSBuff() override;
};

class YGLWSBuff : public Buff
{
public:
    // 伊戈雷乌斯
    static std::string name;

public:
    void listenerCallback(const DamageInfo &info);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    YGLWSBuff(Person *p, double n);
    ~YGLWSBuff() override;
};

class SXMQBuff : public Buff
{
public:
    // 嗜血毛球
    static std::string name;
    bool inCallback = false;

public:
    void listenerCallback(Skill *const skill);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    SXMQBuff(Person *p, double n);
    ~SXMQBuff() override;
};

class SXMQBuff_Passive : public Buff
{
public:
    // 嗜血毛球(被动)
    static std::string name;
    bool inCallback = false;

private:
    std::unique_ptr<Person> tempPerson; // 用于创建技能对象，避免直接使用this->p导致循环引用

public:
    void listenerCallback(const DamageInfo &info);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    SXMQBuff_Passive(Person *p, double n);
    ~SXMQBuff_Passive() override;
};