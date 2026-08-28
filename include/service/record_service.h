#ifndef RECORD_SERVICE_H
#define RECORD_SERVICE_H

#include <QObject>
#include <QDate>
#include <QMap>
#include <QPair>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

#include "dto/time_record.h"   // TimeRecord / TimerEndSnapshot DTO 定义

// 仅以指针形式使用，前向声明替代 include（依赖倒置：Service → IDbAccess）
class IDbAccess;

// RecordService：记录业务服务层（纯记录管理，不含计时逻辑）
// 依赖方向：UI → RecordService → IDbAccess（抽象接口，实际注入 sqlDBM）
// 纯业务层：禁止 include 任何 Widget / UI / 具体数据库实现头文件
// 计时业务已拆分至 TimerService，本服务只负责数据库读写与统计
class RecordService : public QObject
{
    Q_OBJECT
public:
    // dbm 为外部注入的原始指针（不在内部 new），注入时 dbm 必须已完全构造完成
    explicit RecordService(IDbAccess *dbm, QObject *parent = nullptr);
    ~RecordService();

    // ================================================================
    // 记录管理接口（写路径）
    // ================================================================
    // 保存一条计时记录（对应计时结束落库逻辑；表不存在时由 dbm 自动建表）
    bool saveTimeRecord(const QString &date, const QString &startTime, const QString &endTime,
                        int consumTime, int counter, const QString &detail, const QString &eventClass);

    // 从 TimerEndSnapshot 保存记录（assempage 桥接 TimerService 时调用）
    // snapshot 提供计时数据，date/detail/eventClass 由外部对话框补充
    bool saveFromSnapshot(const TimerEndSnapshot &snap, const QString &date,
                          const QString &detail, const QString &eventClass);

    // 删除某日的一条记录（eventID 按日分表各自自增，必须日期+ID 共同定位）
    bool deleteRecord(const QDate &date, int eventId);

    // 更新某日的一条记录（date 定位表，rec.eventId 定位行；其他字段为新值）
    bool updateRecord(const QDate &date, const TimeRecord &rec);

    // 手动新增一条空记录（对应 assAddEmptyRecord 业务逻辑；表不存在时由 dbm 自动建表）
    bool addEmptyRecord(const QDate &date, const TimeRecord &rec);

    // ================================================================
    // 记录管理接口（查询路径，仅做业务封装，复用底层 SQL）
    // ================================================================
    QVector<TimeRecord> queryRecordsOfDay(const QDate &date);     // 查询某日全部记录（按开始时间排序）
    QMap<QString, int> dayClassConsume(const QDate &date);       // 某日按事件分类汇总的消耗时长（分钟）
    QMap<QString, int> weekCounterStats(const QStringList &weekDays); // 某周每天的分神次数
    QSet<QString> markedDaysOfMonth(int year, int month);        // 某月有记录的日期集合（yyyy-MM-dd）

    // ================================================================
    // 周换算与统计工具（纯业务规则，不触数据库；static 供构造期/测试直接使用）
    // ================================================================
    // 周索引 → 日期区间（索引 0 = 最近一周：today-7 ~ today-1）
    static QPair<QDate, QDate> weekRangeForIndex(int index);
    // 周索引 → 该周 7 天的日期列表（yyyy-MM-dd）
    static QStringList weekDaysForIndex(int index);
    // 分类时长汇总求和（分钟）
    static int sumMinutes(const QMap<QString, int> &classConsume);

signals:
    // 业务层数据变更通知：UI 统一订阅本信号，不再直连 dbm::dataChanged
    void dataChanged();

private slots:
    void onDbDataChanged();                                   // 监听底层 IDbAccess::dataChanged 并转发

private:
    IDbAccess *m_dbM = nullptr;                               // 底层数据访问抽象接口（外部注入，不负责释放）

    bool m_suppressForward = false;                           // 写接口执行期间为 true：dbm 通知仅作成功标记
    bool m_opNotified = false;                                // 写接口执行期间收到 dbm 通知即视为成功
};

#endif // RECORD_SERVICE_H
