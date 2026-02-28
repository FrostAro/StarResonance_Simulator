#pragma once
#include "../core/Buff.h"

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
    // 博伊斯
    static std::string name;

public:
    void listenerCallback(const DamageInfo &info);
    void update(double deltaTime) override;
    bool shouldBeRemoved() override;
    std::string getBuffName() const override;

    YGLWSBuff(Person *p, double n);
    ~YGLWSBuff() override;
};