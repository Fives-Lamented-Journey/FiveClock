#ifndef TIME_RECORD_H
#define TIME_RECORD_H

#include <QDate>
#include <QString>
#include <QStringList>

// ================================================================
// EventClass：事件分类枚举（唯一可信源）
// 顺序与 infodialog.ui 中 QComboBox 条目一致
// ================================================================
enum class EventClass {
    WorkStudy = 0,          // 工作学习
    SummaryReflect,         // 总结反思
    FamilyAffection,        // 家庭亲情
    SportsSleep,            // 运动睡眠
    SocialInterpersonal,    // 社交人际
    LeisureEntertainment    // 休闲娱乐
};

// 枚举值总数（用于边界检查）
inline constexpr int kEventClassCount = 6;

// 枚举值 → UI 显示名（中文）
inline QString eventClassDisplayName(EventClass ec)
{
    switch (ec) {
    case EventClass::WorkStudy:          return QStringLiteral("工作学习");
    case EventClass::SummaryReflect:      return QStringLiteral("总结反思");
    case EventClass::FamilyAffection:     return QStringLiteral("家庭亲情");
    case EventClass::SportsSleep:         return QStringLiteral("运动睡眠");
    case EventClass::SocialInterpersonal: return QStringLiteral("社交人际");
    case EventClass::LeisureEntertainment:return QStringLiteral("休闲娱乐");
    }
    return QStringLiteral("工作学习");
}

// 枚举值 → 数据库存储标识（英文 key）
inline QString eventClassDbKey(EventClass ec)
{
    switch (ec) {
    case EventClass::WorkStudy:          return QStringLiteral("WorkStudy");
    case EventClass::SummaryReflect:      return QStringLiteral("SummaryReflect");
    case EventClass::FamilyAffection:     return QStringLiteral("FamilyAffection");
    case EventClass::SportsSleep:         return QStringLiteral("SportsSleep");
    case EventClass::SocialInterpersonal: return QStringLiteral("SocialInterpersonal");
    case EventClass::LeisureEntertainment:return QStringLiteral("LeisureEntertainment");
    }
    return QStringLiteral("WorkStudy");
}

// UI 显示名 → 枚举值（找不到返回 WorkStudy）
inline EventClass eventClassFromDisplayName(const QString &name)
{
    if (name == QStringLiteral("工作学习"))   return EventClass::WorkStudy;
    if (name == QStringLiteral("总结反思"))   return EventClass::SummaryReflect;
    if (name == QStringLiteral("家庭亲情"))   return EventClass::FamilyAffection;
    if (name == QStringLiteral("运动睡眠"))   return EventClass::SportsSleep;
    if (name == QStringLiteral("社交人际"))   return EventClass::SocialInterpersonal;
    if (name == QStringLiteral("休闲娱乐"))   return EventClass::LeisureEntertainment;
    return EventClass::WorkStudy;
}

// 获取全部事件分类显示名列表（顺序固定，与 UI combobox 一致）
inline QStringList eventClassDisplayNameList()
{
    return {
        QStringLiteral("工作学习"),
        QStringLiteral("总结反思"),
        QStringLiteral("家庭亲情"),
        QStringLiteral("运动睡眠"),
        QStringLiteral("社交人际"),
        QStringLiteral("休闲娱乐")
    };
}

// TimeRecord：记录业务 DTO
// 一条计时记录的结构化表达，字段顺序与 sqlDBM::sortedRecord 查询列一致：
// eventID, startTime, endTime, consumTime, counter, detail, eventclass
// 本头文件仅定义数据结构，不依赖任何业务/数据库/UI 头文件
struct TimeRecord
{
    int eventId = 0;          // 事件 ID（按日分表内自增）
    QString begTime;          // 开始时间（HH:mm:ss）
    QString endTime;          // 结束时间（HH:mm:ss）
    int consumTime = 0;       // 消耗时长（分钟）
    int counter = 0;          // 分神次数
    QString detail;           // 详细描述
    QString eventClass;       // 事件分类（中文显示名，如 "工作学习"）
};

// TimerEndSnapshot：计时结束快照
// TimerModel::end() 发出的数据结构，供 RecordService 转发给 UI 层
struct TimerEndSnapshot
{
    qint64 elapsedMs = 0;     // 本轮累计流逝毫秒数
    int consumMin = 0;        // 消耗分钟数（elapsedMs / 60000）
    int counter = 0;          // 分神次数
    QString startTime;        // 开始时间字符串（HH:mm:ss）
    QString endTime;          // 结束时间字符串（HH:mm:ss）
};

#endif // TIME_RECORD_H
