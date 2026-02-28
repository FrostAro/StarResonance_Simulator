#pragma once
#include "../../core/AutoAttack.h"
#include "Skill.h"
#include "../../FightingFantasy/Skill.h"
#include "../../core/Person.h"
#include "../../core/Logger.h"
#include <cassert>

class AutoAttack_Mage_Beam_MukuScout : public AutoAttack
{
    int nextOutBurstSignal = 1;
    bool nextIsWindow = false;

    void outBurst1()
    {
        this->maniAddPriorSkillList(MukuChief::name);
        this->maniAddPriorSkillList(MukuScout::name);
        this->maniAddPriorSkillList(Vortex::name);
        this->maniAddPriorSkillList(FrostWind::name);
        this->maniAddPriorSkillList(Ultimate_Beam::name);
        this->maniAddPriorSkillList(Flood_Beam::name);
        this->maniAddPriorSkillList(Beam::name);
        this->maniAddPriorSkillList(WaterSpout::name);
    }

    void checkAndTriggerOutBurst1()
    {
        if (nextOutBurstSignal == 1 && !this->isOutBurst)
        {
            // 更新技能CD，buffCD，释放技能判断，爆发判断
            int mukuChiefIndex = this->p->findSkillInSkillCDList(MukuChief::name);
            int mukuScoutIndex = this->p->findSkillInSkillCDList(MukuScout::name);
            int vortexIndex = this->p->findSkillInSkillCDList(Vortex::name);
            int frostWindIndex = this->p->findSkillInSkillCDList(FrostWind::name);
            int ultimateIndex = this->p->findSkillInSkillCDList(Ultimate_Beam::name);
            int floodIndex = this->p->findSkillInSkillCDList(Flood_Beam::name);

            // 检查索引有效性
            bool canLargeOutBurst = false;

            // 检查所有索引都有效
            if (mukuChiefIndex != -1 && mukuScoutIndex != -1 &&
                vortexIndex != -1 && frostWindIndex != -1 && ultimateIndex != -1 && floodIndex != -1)
            {

                // 检查 CD 是否为 0
                canLargeOutBurst =
                    ((this->p->getSkillCDListRef().at(mukuChiefIndex)->getCurrentCD_Value() == 0 || this->p->getSkillCDListRef().at(mukuChiefIndex)->getStackValue() > 0) 
                    && this->p->getSkillCDListRef().at(mukuScoutIndex)->getCurrentCD_Value() == 0 
                    && this->p->getSkillCDListRef().at(vortexIndex)->getCurrentCD_Value() == 0 
                    && this->p->getSkillCDListRef().at(frostWindIndex)->getCurrentCD_Value() == 0 
                    && this->p->getSkillCDListRef().at(ultimateIndex)->getCurrentCD_Value() == 0 
                    && this->p->getSkillCDListRef().at(floodIndex)->getCurrentCD_Value() == 0);
            }

            if (canLargeOutBurst)
            {
                Logger::debugAutoAttack(this->timer, "outBurst1 started");
                this->outBurst1();
                this->nextOutBurstSignal = 2;
                this->isOutBurst = true;
            }
        }
    }

    void outBurst2()
    {
        this->maniAddPriorSkillList(MukuChief::name);
        this->maniAddPriorSkillList(Vortex::name);
        this->maniAddPriorSkillList(FrostWind::name);
        this->maniAddPriorSkillList(Flood_Beam::name);
        this->maniAddPriorSkillList(Beam::name);
        this->maniAddPriorSkillList(WaterSpout::name);
    }

    void checkAndTriggerOutBurst2()
    {
        if (nextOutBurstSignal == 2 && !this->isOutBurst)
        {
            // 更新技能CD，buffCD，释放技能判断，爆发判断
            int mukuChiefIndex = this->p->findSkillInSkillCDList(MukuChief::name);
            int vortexIndex = this->p->findSkillInSkillCDList(Vortex::name);
            int frostWindIndex = this->p->findSkillInSkillCDList(FrostWind::name);
            int floodIndex = this->p->findSkillInSkillCDList(Flood_Beam::name);

            // 检查索引有效性
            bool canLargeOutBurst = false;

            // 检查所有索引都有效
            if (mukuChiefIndex != -1 && vortexIndex != -1 && frostWindIndex != -1 && floodIndex != -1)
            {

                // 检查 CD 是否为 0
                canLargeOutBurst =
                    ((this->p->getSkillCDListRef().at(mukuChiefIndex)->getCurrentCD_Value() == 0 || this->p->getSkillCDListRef().at(mukuChiefIndex)->getStackValue() > 0) 
                    && this->p->getSkillCDListRef().at(vortexIndex)->getCurrentCD_Value() == 0 
                    && this->p->getSkillCDListRef().at(frostWindIndex)->getCurrentCD_Value() == 0 
                    && this->p->getSkillCDListRef().at(floodIndex)->getCurrentCD_Value() == 0);
            }

            if (canLargeOutBurst)
            {
                Logger::debugAutoAttack(this->timer, "outBurst2 started");
                this->outBurst2();
                this->nextOutBurstSignal = 3;
                this->isOutBurst = true;
            }
        }
    }

    void outBurst3()
    {
        this->maniAddPriorSkillList(MukuChief::name);
        this->maniAddPriorSkillList(Vortex::name);
        this->maniAddPriorSkillList(FrostWind::name);
        this->maniAddPriorSkillList(Ultimate_Beam::name);
        this->maniAddPriorSkillList(Flood_Beam::name);
        this->maniAddPriorSkillList(Beam::name);
        this->maniAddPriorSkillList(WaterSpout::name);
    }

    void checkAndTriggerOutBurst3()
    {
        if (nextOutBurstSignal == 3 && !this->isOutBurst)
        {
            // 更新技能CD，buffCD，释放技能判断，爆发判断
            int mukuChiefIndex = this->p->findSkillInSkillCDList(MukuChief::name);
            int ultimateIndex = this->p->findSkillInSkillCDList(Ultimate_Beam::name);
            int vortexIndex = this->p->findSkillInSkillCDList(Vortex::name);
            int frostWindIndex = this->p->findSkillInSkillCDList(FrostWind::name);
            int floodIndex = this->p->findSkillInSkillCDList(Flood_Beam::name);

            // 检查索引有效性
            bool canLargeOutBurst = false;

            // 检查所有索引都有效
            if (mukuChiefIndex != -1 
                && ultimateIndex != -1
                && vortexIndex != -1
                && frostWindIndex != -1
                && floodIndex != -1)
            {

                // 检查 CD 是否为 0
                canLargeOutBurst =
                    ((this->p->getSkillCDListRef().at(mukuChiefIndex)->getCurrentCD_Value() == 0 || this->p->getSkillCDListRef().at(mukuChiefIndex)->getStackValue() > 0)              
                    && this->p->getSkillCDListRef().at(vortexIndex)->getCurrentCD_Value() == 0 
                    && this->p->getSkillCDListRef().at(frostWindIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(ultimateIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(floodIndex)->getCurrentCD_Value() == 0);
            }

            if (canLargeOutBurst)
            {
                Logger::debugAutoAttack(this->timer, "outBurst3 started");
                this->outBurst3();
                this->nextOutBurstSignal = 4;
                this->isOutBurst = true;
            }
        }
    }

    void outBurst4()
    {
        this->maniAddPriorSkillList(Vortex::name);
        this->maniAddPriorSkillList(FrostWind::name);
        this->maniAddPriorSkillList(Flood_Beam::name);
        this->maniAddPriorSkillList(Beam::name);
        this->maniAddPriorSkillList(WaterSpout::name);
    }

    void checkAndTriggerOutBurst4()
    {
        if (nextOutBurstSignal == 4 && !this->isOutBurst)
        {
            // 更新技能CD，buffCD，释放技能判断，爆发判断
            int floodIndex = this->p->findSkillInSkillCDList(Flood_Beam::name);
            int vortexIndex = this->p->findSkillInSkillCDList(Vortex::name);
            int frostWindIndex = this->p->findSkillInSkillCDList(FrostWind::name);

            // 检查索引有效性
            bool canLargeOutBurst = false;

            // 检查所有索引都有效
            if (floodIndex != -1 && vortexIndex != -1 && frostWindIndex != -1)
            {
                // 检查 CD 是否为 0
                canLargeOutBurst = (this->p->getSkillCDListRef().at(floodIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(vortexIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(frostWindIndex)->getCurrentCD_Value() == 0);
            }

            if (canLargeOutBurst)
            {
                Logger::debugAutoAttack(this->timer, "outBurst4 started");
                this->outBurst4();
                this->nextOutBurstSignal = 5;
                this->isOutBurst = true;
            }
        }
    }

    void outBurst5()
    {
        this->maniAddPriorSkillList(MukuChief::name);
        this->maniAddPriorSkillList(Vortex::name);
        this->maniAddPriorSkillList(FrostWind::name);
        this->maniAddPriorSkillList(Flood_Beam::name);
        this->maniAddPriorSkillList(Beam::name);
        this->maniAddPriorSkillList(WaterSpout::name);
    }

    void checkAndTriggerOutBurst5()
    {
        if (nextOutBurstSignal == 5 && !this->isOutBurst)
        {
            // 更新技能CD，buffCD，释放技能判断，爆发判断
            int mukuChiefIndex = this->p->findSkillInSkillCDList(MukuChief::name);
            int floodIndex = this->p->findSkillInSkillCDList(Flood_Beam::name);
            int vortexIndex = this->p->findSkillInSkillCDList(Vortex::name);
            int frostWindIndex = this->p->findSkillInSkillCDList(FrostWind::name);

            // 检查索引有效性
            bool canLargeOutBurst = false;

            // 检查所有索引都有效
            if (mukuChiefIndex != -1 && floodIndex != -1 && vortexIndex != -1 && frostWindIndex != -1)
            {
                // 检查 CD 是否为 0
                canLargeOutBurst = (this->p->getSkillCDListRef().at(mukuChiefIndex)->getCurrentCD_Value() == 0 || this->p->getSkillCDListRef().at(mukuChiefIndex)->getStackValue() > 0)
                    && this->p->getSkillCDListRef().at(floodIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(vortexIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(frostWindIndex)->getCurrentCD_Value() == 0;
            }

            if (canLargeOutBurst)
            {
                Logger::debugAutoAttack(this->timer, "outBurst5 started");
                this->outBurst5();
                this->nextOutBurstSignal = 6;
                this->isOutBurst = true;
            }
        }
    }

    void outBurst6()
    {
        this->maniAddPriorSkillList(Vortex::name);
        this->maniAddPriorSkillList(FrostWind::name);
        this->maniAddPriorSkillList(Ultimate_Beam::name);
        this->maniAddPriorSkillList(Flood_Beam::name);
        this->maniAddPriorSkillList(Beam::name);
        this->maniAddPriorSkillList(WaterSpout::name);
    }

    void checkAndTriggerOutBurst6()
    {
        if (nextOutBurstSignal == 6 && !this->isOutBurst)
        {
            // 更新技能CD，buffCD，释放技能判断，爆发判断
            int ultimateIndex = this->p->findSkillInSkillCDList(Ultimate_Beam::name);
            int floodIndex = this->p->findSkillInSkillCDList(Flood_Beam::name);
            int vortexIndex = this->p->findSkillInSkillCDList(Vortex::name);
            int frostWindIndex = this->p->findSkillInSkillCDList(FrostWind::name);

            // 检查索引有效性
            bool canLargeOutBurst = false;

            // 检查所有索引都有效
            if (ultimateIndex != -1 && floodIndex != -1 && vortexIndex != -1 && frostWindIndex != -1)
            {
                // 检查 CD 是否为 0
                canLargeOutBurst = (this->p->getSkillCDListRef().at(ultimateIndex)->getCurrentCD_Value() == 0)
                    && this->p->getSkillCDListRef().at(floodIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(vortexIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(frostWindIndex)->getCurrentCD_Value() == 0;
            }

            if (canLargeOutBurst)
            {
                Logger::debugAutoAttack(this->timer, "outBurst6 started");
                this->outBurst6();
                this->nextOutBurstSignal = 7;
                this->isOutBurst = true;
            }
        }
    }

    void outBurst7()
    {
        this->maniAddPriorSkillList(MukuChief::name);
        this->maniAddPriorSkillList(MukuScout::name);
        this->maniAddPriorSkillList(Vortex::name);
        this->maniAddPriorSkillList(FrostWind::name);
        this->maniAddPriorSkillList(Flood_Beam::name);
        this->maniAddPriorSkillList(Beam::name);
        this->maniAddPriorSkillList(WaterSpout::name);
    }

    void checkAndTriggerOutBurst7()
    {
        if (nextOutBurstSignal == 7 && !this->isOutBurst)
        {
            // 更新技能CD，buffCD，释放技能判断，爆发判断
            int mukuChiefIndex = this->p->findSkillInSkillCDList(MukuChief::name);
            int mukuScoutIndex = this->p->findSkillInSkillCDList(MukuScout::name);
            int vortexIndex = this->p->findSkillInSkillCDList(Vortex::name);
            int frostWindIndex = this->p->findSkillInSkillCDList(FrostWind::name);
            int floodIndex = this->p->findSkillInSkillCDList(Flood_Beam::name);

            // 检查索引有效性
            bool canLargeOutBurst = false;

            // 检查所有索引都有效
            if (mukuChiefIndex != -1 && mukuScoutIndex != -1 && vortexIndex != -1 && frostWindIndex != -1 && floodIndex != -1)
            {
                // 检查 CD 是否为 0
                canLargeOutBurst = (this->p->getSkillCDListRef().at(mukuChiefIndex)->getCurrentCD_Value() == 0 || this->p->getSkillCDListRef().at(mukuChiefIndex)->getStackValue() > 0)
                    && this->p->getSkillCDListRef().at(mukuScoutIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(vortexIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(frostWindIndex)->getCurrentCD_Value() == 0;
            }

            if (canLargeOutBurst)
            {
                Logger::debugAutoAttack(this->timer, "outBurst7 started");
                this->outBurst7();
                this->nextOutBurstSignal = 8;
                this->isOutBurst = true;
            }
        }
    }

    void windowPeriodLogic() override 
    {
        if(!this->nextIsWindow)
        {
            return;
        }
        static bool skillTriggered = false;
        if(!skillTriggered)
        {
            this->maniAddPriorSkillList(Vortex::name);
            this->maniAddPriorSkillList(FrostWind::name);
            this->maniAddPriorSkillList(Beam::name);
            skillTriggered = true;  
        }
        static bool secondBeamTriggered = false;
        if(this->p->getNowReleasingSkill() != nullptr
            && this->p->getNowReleasingSkill()->getSkillName() == Beam::name
            && !secondBeamTriggered && skillTriggered)
        {
            Beam* beam = static_cast<Beam*>(this->p->getNowReleasingSkill());
            assert(dynamic_cast<Beam*>(this->p->getNowReleasingSkill()) != nullptr && 
            "NowReleasingSkill type is not Beam!");
            if(beam->count > beam->noEnergyConsumeTime)
            {
                beam->stop();
                this->maniAddPriorSkillList(Beam::name);
                secondBeamTriggered = true;
                Logger::debugAutoAttack(this->timer,"beam has been stopped compulsorily by autoAttack");
            }
        }
        if(skillTriggered && secondBeamTriggered && this->priorSkillList.empty())
        {
            skillTriggered = false;
            secondBeamTriggered = false;
            this->nextIsWindow = false;
        }
    }

    void checkAndFinishOutBurst() override 
    {
        if(this->priorSkillList.empty() && this->isOutBurst 
            && this->p->getNowReleasingSkill() == nullptr)
        {
            this->isOutBurst = false;
            this->nextIsWindow = true;
            Logger::debugAutoAttack(this->timer, "outBurst finished");
        }
    }

    void update(int deltaTime) override
    {
        if(timer > 0 && timer < 500)
        {
            checkAndTriggerOutBurst1();
        }

        if(timer > 2700 && timer < 4000)
        {
            checkAndTriggerOutBurst2();
        }

        if(timer > 5400 && timer < 6500)
        {
            checkAndTriggerOutBurst3();
        }

        if(timer > 8100 && timer < 9500)
        {
            checkAndTriggerOutBurst4();
        }

        if(timer > 10800 && timer < 12800)
        {
            checkAndTriggerOutBurst5();
        }

        if(timer > 13500 && timer < 14500)
        {
            checkAndTriggerOutBurst6();
        }

        if(timer > 16300 && timer < 17500)
        {
            checkAndTriggerOutBurst7();
        }
        
        windowPeriodLogic();  

        checkAndFinishOutBurst();

        createSkillByAuto();
        updatePerson(deltaTime);
    }

public:
    AutoAttack_Mage_Beam_MukuScout(Person *p) : AutoAttack(p) {};
};






class AutoAttack_Mage_Beam_YGLWS : public AutoAttack
{
    int nextOutBurstSignal = 1;
    bool nextIsWindow = false;

    void outBurst1()
    {
        this->maniAddPriorSkillList(MukuChief::name);
        this->maniAddPriorSkillList(YGLWS::name);
        this->maniAddPriorSkillList(Vortex::name);
        this->maniAddPriorSkillList(FrostWind::name);
        this->maniAddPriorSkillList(Ultimate_Beam::name);
        this->maniAddPriorSkillList(Flood_Beam::name);
        this->maniAddPriorSkillList(Beam::name);
        this->maniAddPriorSkillList(WaterSpout::name);
    }

    void checkAndTriggerOutBurst1()
    {
        if (nextOutBurstSignal == 1 && !this->isOutBurst)
        {
            // 更新技能CD，buffCD，释放技能判断，爆发判断
            int mukuChiefIndex = this->p->findSkillInSkillCDList(MukuChief::name);
            int mukuScoutIndex = this->p->findSkillInSkillCDList(YGLWS::name);
            int vortexIndex = this->p->findSkillInSkillCDList(Vortex::name);
            int frostWindIndex = this->p->findSkillInSkillCDList(FrostWind::name);
            int ultimateIndex = this->p->findSkillInSkillCDList(Ultimate_Beam::name);
            int floodIndex = this->p->findSkillInSkillCDList(Flood_Beam::name);

            // 检查索引有效性
            bool canLargeOutBurst = false;

            // 检查所有索引都有效
            if (mukuChiefIndex != -1 && mukuScoutIndex != -1 &&
                vortexIndex != -1 && frostWindIndex != -1 && ultimateIndex != -1 && floodIndex != -1)
            {

                // 检查 CD 是否为 0
                canLargeOutBurst =
                    ((this->p->getSkillCDListRef().at(mukuChiefIndex)->getCurrentCD_Value() == 0 || this->p->getSkillCDListRef().at(mukuChiefIndex)->getStackValue() > 0) 
                    && this->p->getSkillCDListRef().at(mukuScoutIndex)->getCurrentCD_Value() == 0 || this->p->getSkillCDListRef().at(mukuScoutIndex)->getStackValue() > 0
                    && this->p->getSkillCDListRef().at(vortexIndex)->getCurrentCD_Value() == 0 
                    && this->p->getSkillCDListRef().at(frostWindIndex)->getCurrentCD_Value() == 0 
                    && this->p->getSkillCDListRef().at(ultimateIndex)->getCurrentCD_Value() == 0 
                    && this->p->getSkillCDListRef().at(floodIndex)->getCurrentCD_Value() == 0);
            }

            if (canLargeOutBurst)
            {
                Logger::debugAutoAttack(this->timer, "outBurst1 started");
                this->outBurst1();
                this->nextOutBurstSignal = 2;
                this->isOutBurst = true;
            }
        }
    }

    void outBurst2()
    {
        this->maniAddPriorSkillList(MukuChief::name);
        this->maniAddPriorSkillList(Vortex::name);
        this->maniAddPriorSkillList(FrostWind::name);
        this->maniAddPriorSkillList(Flood_Beam::name);
        this->maniAddPriorSkillList(Beam::name);
        this->maniAddPriorSkillList(WaterSpout::name);
    }

    void checkAndTriggerOutBurst2()
    {
        if (nextOutBurstSignal == 2 && !this->isOutBurst)
        {
            // 更新技能CD，buffCD，释放技能判断，爆发判断
            int mukuChiefIndex = this->p->findSkillInSkillCDList(MukuChief::name);
            int vortexIndex = this->p->findSkillInSkillCDList(Vortex::name);
            int frostWindIndex = this->p->findSkillInSkillCDList(FrostWind::name);
            int floodIndex = this->p->findSkillInSkillCDList(Flood_Beam::name);

            // 检查索引有效性
            bool canLargeOutBurst = false;

            // 检查所有索引都有效
            if (mukuChiefIndex != -1 && vortexIndex != -1 && frostWindIndex != -1 && floodIndex != -1)
            {

                // 检查 CD 是否为 0
                canLargeOutBurst =
                    ((this->p->getSkillCDListRef().at(mukuChiefIndex)->getCurrentCD_Value() == 0 || this->p->getSkillCDListRef().at(mukuChiefIndex)->getStackValue() > 0) 
                    && this->p->getSkillCDListRef().at(vortexIndex)->getCurrentCD_Value() == 0 
                    && this->p->getSkillCDListRef().at(frostWindIndex)->getCurrentCD_Value() == 0 
                    && this->p->getSkillCDListRef().at(floodIndex)->getCurrentCD_Value() == 0);
            }

            if (canLargeOutBurst)
            {
                Logger::debugAutoAttack(this->timer, "outBurst2 started");
                this->outBurst2();
                this->nextOutBurstSignal = 3;
                this->isOutBurst = true;
            }
        }
    }

    void outBurst3()
    {
        this->maniAddPriorSkillList(MukuChief::name);
        this->maniAddPriorSkillList(YGLWS::name);
        this->maniAddPriorSkillList(Vortex::name);
        this->maniAddPriorSkillList(FrostWind::name);
        this->maniAddPriorSkillList(Ultimate_Beam::name);
        this->maniAddPriorSkillList(Flood_Beam::name);
        this->maniAddPriorSkillList(Beam::name);
        this->maniAddPriorSkillList(WaterSpout::name);
    }

    void checkAndTriggerOutBurst3()
    {
        if (nextOutBurstSignal == 3 && !this->isOutBurst)
        {
            // 更新技能CD，buffCD，释放技能判断，爆发判断
            int mukuChiefIndex = this->p->findSkillInSkillCDList(MukuChief::name);
            int mukuScoutIndex = this->p->findSkillInSkillCDList(YGLWS::name);
            int ultimateIndex = this->p->findSkillInSkillCDList(Ultimate_Beam::name);
            int vortexIndex = this->p->findSkillInSkillCDList(Vortex::name);
            int frostWindIndex = this->p->findSkillInSkillCDList(FrostWind::name);
            int floodIndex = this->p->findSkillInSkillCDList(Flood_Beam::name);

            // 检查索引有效性
            bool canLargeOutBurst = false;

            // 检查所有索引都有效
            if (mukuChiefIndex != -1 
                && mukuScoutIndex != -1
                && ultimateIndex != -1
                && vortexIndex != -1
                && frostWindIndex != -1
                && floodIndex != -1)
            {

                // 检查 CD 是否为 0
                canLargeOutBurst =
                    ((this->p->getSkillCDListRef().at(mukuChiefIndex)->getCurrentCD_Value() == 0 || this->p->getSkillCDListRef().at(mukuChiefIndex)->getStackValue() > 0)              
                    && (this->p->getSkillCDListRef().at(mukuScoutIndex)->getCurrentCD_Value() == 0 || this->p->getSkillCDListRef().at(mukuScoutIndex)->getStackValue() > 0)
                    && this->p->getSkillCDListRef().at(vortexIndex)->getCurrentCD_Value() == 0 
                    && this->p->getSkillCDListRef().at(frostWindIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(ultimateIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(floodIndex)->getCurrentCD_Value() == 0);
            }

            if (canLargeOutBurst)
            {
                Logger::debugAutoAttack(this->timer, "outBurst3 started");
                this->outBurst3();
                this->nextOutBurstSignal = 4;
                this->isOutBurst = true;
            }
        }
    }

    void outBurst4()
    {
        this->maniAddPriorSkillList(Vortex::name);
        this->maniAddPriorSkillList(FrostWind::name);
        this->maniAddPriorSkillList(Flood_Beam::name);
        this->maniAddPriorSkillList(Beam::name);
        this->maniAddPriorSkillList(WaterSpout::name);
    }

    void checkAndTriggerOutBurst4()
    {
        if (nextOutBurstSignal == 4 && !this->isOutBurst)
        {
            // 更新技能CD，buffCD，释放技能判断，爆发判断
            int floodIndex = this->p->findSkillInSkillCDList(Flood_Beam::name);
            int vortexIndex = this->p->findSkillInSkillCDList(Vortex::name);
            int frostWindIndex = this->p->findSkillInSkillCDList(FrostWind::name);

            // 检查索引有效性
            bool canLargeOutBurst = false;

            // 检查所有索引都有效
            if (floodIndex != -1 && vortexIndex != -1 && frostWindIndex != -1)
            {
                // 检查 CD 是否为 0
                canLargeOutBurst = (this->p->getSkillCDListRef().at(floodIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(vortexIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(frostWindIndex)->getCurrentCD_Value() == 0);
            }

            if (canLargeOutBurst)
            {
                Logger::debugAutoAttack(this->timer, "outBurst4 started");
                this->outBurst4();
                this->nextOutBurstSignal = 5;
                this->isOutBurst = true;
            }
        }
    }

    void outBurst5()
    {
        this->maniAddPriorSkillList(MukuChief::name);
        this->maniAddPriorSkillList(YGLWS::name);
        this->maniAddPriorSkillList(Vortex::name);
        this->maniAddPriorSkillList(FrostWind::name);
        this->maniAddPriorSkillList(Flood_Beam::name);
        this->maniAddPriorSkillList(Beam::name);
        this->maniAddPriorSkillList(WaterSpout::name);
    }

    void checkAndTriggerOutBurst5()
    {
        if (nextOutBurstSignal == 5 && !this->isOutBurst)
        {
            // 更新技能CD，buffCD，释放技能判断，爆发判断
            int mukuChiefIndex = this->p->findSkillInSkillCDList(MukuChief::name);
            int mukuScoutIndex = this->p->findSkillInSkillCDList(YGLWS::name);
            int floodIndex = this->p->findSkillInSkillCDList(Flood_Beam::name);
            int vortexIndex = this->p->findSkillInSkillCDList(Vortex::name);
            int frostWindIndex = this->p->findSkillInSkillCDList(FrostWind::name);

            // 检查索引有效性
            bool canLargeOutBurst = false;

            // 检查所有索引都有效
            if (mukuChiefIndex != -1 && mukuScoutIndex != -1 && floodIndex != -1 && vortexIndex != -1 && frostWindIndex != -1)
            {
                // 检查 CD 是否为 0
                canLargeOutBurst = (this->p->getSkillCDListRef().at(mukuChiefIndex)->getCurrentCD_Value() == 0 || this->p->getSkillCDListRef().at(mukuChiefIndex)->getStackValue() > 0)
                    && this->p->getSkillCDListRef().at(mukuScoutIndex)->getCurrentCD_Value() == 0 || this->p->getSkillCDListRef().at(mukuScoutIndex)->getStackValue() > 0
                    && this->p->getSkillCDListRef().at(floodIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(vortexIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(frostWindIndex)->getCurrentCD_Value() == 0;
            }

            if (canLargeOutBurst)
            {
                Logger::debugAutoAttack(this->timer, "outBurst5 started");
                this->outBurst5();
                this->nextOutBurstSignal = 6;
                this->isOutBurst = true;
            }
        }
    }

    void outBurst6()
    {
        this->maniAddPriorSkillList(Vortex::name);
        this->maniAddPriorSkillList(FrostWind::name);
        this->maniAddPriorSkillList(Ultimate_Beam::name);
        this->maniAddPriorSkillList(Flood_Beam::name);
        this->maniAddPriorSkillList(Beam::name);
        this->maniAddPriorSkillList(WaterSpout::name);
    }

    void checkAndTriggerOutBurst6()
    {
        if (nextOutBurstSignal == 6 && !this->isOutBurst)
        {
            // 更新技能CD，buffCD，释放技能判断，爆发判断
            int ultimateIndex = this->p->findSkillInSkillCDList(Ultimate_Beam::name);
            int floodIndex = this->p->findSkillInSkillCDList(Flood_Beam::name);
            int vortexIndex = this->p->findSkillInSkillCDList(Vortex::name);
            int frostWindIndex = this->p->findSkillInSkillCDList(FrostWind::name);

            // 检查索引有效性
            bool canLargeOutBurst = false;

            // 检查所有索引都有效
            if (ultimateIndex != -1 && floodIndex != -1 && vortexIndex != -1 && frostWindIndex != -1)
            {
                // 检查 CD 是否为 0
                canLargeOutBurst = (this->p->getSkillCDListRef().at(ultimateIndex)->getCurrentCD_Value() == 0)
                    && this->p->getSkillCDListRef().at(floodIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(vortexIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(frostWindIndex)->getCurrentCD_Value() == 0;
            }

            if (canLargeOutBurst)
            {
                Logger::debugAutoAttack(this->timer, "outBurst6 started");
                this->outBurst6();
                this->nextOutBurstSignal = 7;
                this->isOutBurst = true;
            }
        }
    }

    void outBurst7()
    {
        this->maniAddPriorSkillList(MukuChief::name);
        this->maniAddPriorSkillList(YGLWS::name);
        this->maniAddPriorSkillList(Vortex::name);
        this->maniAddPriorSkillList(FrostWind::name);
        this->maniAddPriorSkillList(Flood_Beam::name);
        this->maniAddPriorSkillList(Beam::name);
        this->maniAddPriorSkillList(WaterSpout::name);
    }

    void checkAndTriggerOutBurst7()
    {
        if (nextOutBurstSignal == 7 && !this->isOutBurst)
        {
            // 更新技能CD，buffCD，释放技能判断，爆发判断
            int mukuChiefIndex = this->p->findSkillInSkillCDList(MukuChief::name);
            int mukuScoutIndex = this->p->findSkillInSkillCDList(YGLWS::name);
            int vortexIndex = this->p->findSkillInSkillCDList(Vortex::name);
            int frostWindIndex = this->p->findSkillInSkillCDList(FrostWind::name);
            int floodIndex = this->p->findSkillInSkillCDList(Flood_Beam::name);

            // 检查索引有效性
            bool canLargeOutBurst = false;

            // 检查所有索引都有效
            if (mukuChiefIndex != -1 && mukuScoutIndex != -1 && vortexIndex != -1 && frostWindIndex != -1 && floodIndex != -1)
            {
                // 检查 CD 是否为 0
                canLargeOutBurst = (this->p->getSkillCDListRef().at(mukuChiefIndex)->getCurrentCD_Value() == 0 || this->p->getSkillCDListRef().at(mukuChiefIndex)->getStackValue() > 0)
                    && this->p->getSkillCDListRef().at(mukuScoutIndex)->getCurrentCD_Value() == 0 || this->p->getSkillCDListRef().at(mukuScoutIndex)->getStackValue() > 0
                    && this->p->getSkillCDListRef().at(vortexIndex)->getCurrentCD_Value() == 0
                    && this->p->getSkillCDListRef().at(frostWindIndex)->getCurrentCD_Value() == 0;
            }

            if (canLargeOutBurst)
            {
                Logger::debugAutoAttack(this->timer, "outBurst7 started");
                this->outBurst7();
                this->nextOutBurstSignal = 8;
                this->isOutBurst = true;
            }
        }
    }

    void windowPeriodLogic() override 
    {
        if(!this->nextIsWindow)
        {
            return;
        }
        static bool skillTriggered = false;
        if(!skillTriggered)
        {
            this->maniAddPriorSkillList(Vortex::name);
            this->maniAddPriorSkillList(FrostWind::name);
            this->maniAddPriorSkillList(Beam::name);
            skillTriggered = true;  
        }
        static bool secondBeamTriggered = false;
        if(this->p->getNowReleasingSkill() != nullptr
            && this->p->getNowReleasingSkill()->getSkillName() == Beam::name
            && !secondBeamTriggered && skillTriggered)
        {
            Beam* beam = static_cast<Beam*>(this->p->getNowReleasingSkill());
            assert(dynamic_cast<Beam*>(this->p->getNowReleasingSkill()) != nullptr && 
            "NowReleasingSkill type is not Beam!");
            if(beam->count > beam->noEnergyConsumeTime)
            {
                beam->stop();
                this->maniAddPriorSkillList(Beam::name);
                secondBeamTriggered = true;
                Logger::debugAutoAttack(this->timer,"beam has been stopped compulsorily by autoAttack");
            }
        }
        if(skillTriggered && secondBeamTriggered && this->priorSkillList.empty())
        {
            skillTriggered = false;
            secondBeamTriggered = false;
            this->nextIsWindow = false;
        }
    }

    void checkAndFinishOutBurst() override 
    {
        if(this->priorSkillList.empty() && this->isOutBurst 
            && this->p->getNowReleasingSkill() == nullptr)
        {
            this->isOutBurst = false;
            this->nextIsWindow = true;
            Logger::debugAutoAttack(this->timer, "outBurst finished");
        }
    }

    void update(int deltaTime) override
    {
        if(timer > 0 && timer < 500)
        {
            checkAndTriggerOutBurst1();
        }

        if(timer > 2700 && timer < 4000)
        {
            checkAndTriggerOutBurst2();
        }

        if(timer > 5400 && timer < 6500)
        {
            checkAndTriggerOutBurst3();
        }

        if(timer > 8100 && timer < 9500)
        {
            checkAndTriggerOutBurst4();
        }

        if(timer > 10800 && timer < 12800)
        {
            checkAndTriggerOutBurst5();
        }

        if(timer > 13500 && timer < 14500)
        {
            checkAndTriggerOutBurst6();
        }

        if(timer > 16300 && timer < 17500)
        {
            checkAndTriggerOutBurst7();
        }
        
        windowPeriodLogic();  

        checkAndFinishOutBurst();

        createSkillByAuto();
        updatePerson(deltaTime);
    }

public:
    AutoAttack_Mage_Beam_YGLWS(Person *p) : AutoAttack(p) {};
};





