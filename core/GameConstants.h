// ============================================================================
// 文件: GameConstants.h
// 描述: 游戏数值的共享常量，集中管理避免魔法数字散落各处
// 被 core/Buff.h 引入，因此所有 buff/技能/人物文件均可直接使用
// ============================================================================

#pragma once

// 永久Buff/技能的时长占位值（配合 isInherent=true 使用，表示"不随时间衰减"）
inline constexpr double kPermanentBuffDuration = 9999990;        // 常规永久时长（1 tick = 1ms）
inline constexpr double kPermanentBuffDurationLarge = 99999990;  // 较大的永久时长（个别buff使用）

// 面板基础百分比（从站街面板换算属性点数时扣除的部分）
inline constexpr double kBaseCriticalPercent = 0.05;   // 基础暴击
inline constexpr double kBaseLuckyPercent = 0.05;      // 基础幸运
inline constexpr double kBaseProficientPercent = 0.06; // 基础精通

// 属性转化系数（游戏数值平衡）
inline constexpr int kPropertyTransformCoeffGeneral = 200000;  // 一般属性转化系数
inline constexpr int kPropertyTransformCoeffAlmighty = 89600;  // 全能属性转化系数
