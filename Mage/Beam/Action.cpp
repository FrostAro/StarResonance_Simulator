#include "Action.h"
#include "../../core/Person.h"
#include "../../core/Logger.h"
#include "Person.h"

// 射线部分
// 能量回复
std::string EnergyRevertAction_Beam::name = "EnergyRevertAction_Beam";

EnergyRevertAction_Beam::EnergyRevertAction_Beam(const Skill* const skill)
    : EnergyRevertAction(skill) {}

EnergyRevertAction_Beam::EnergyRevertAction_Beam()
    : EnergyRevertAction() {}

void EnergyRevertAction_Beam::execute(double n, Person *p)
{
    double proficient = p->getProficient();
    Mage_Beam* Beam_p = static_cast<Mage_Beam*>(p);
    n *= (1 + proficient * Beam_p->proficientToEnergyRatio);
    Logger::debugAction(AutoAttack::getTimer(), this->getActionName(),
                        "proficient energy bonus applied, value: " + std::to_string(n));
    EnergyRevertAction::execute(n,p);
}

std::string EnergyRevertAction_Beam::getActionName()
{
    return EnergyRevertAction_Beam::name;
}