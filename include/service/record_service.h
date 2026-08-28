#ifndef RECORD_SERVICE_H
#define RECORD_SERVICE_H

#include <QObject>
#include <QDate>
#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

// 仅以指针形式使用，前向声明替代 include（保持单向依赖：Service → DBM）
class sqlDBM;
class TimerModel;

// 记录业务 DTO：一条计时记录的结构化表达（字段顺序与 sortedRecord 查询列一致）
struct TimeRecord
{
    int eventId = 0;          // 事件 ID（按日分表内自增）
    QString begTime;          // 开始时间
    QString endTime;          // 结束时间
    int consumTime = 0;       // 消耗时长（分钟）
    int counter = 0;          // 分神次数
    QString detail;           // 详细描述
    QString eventClass;       // 事件分类
};

// 计时结束快照：TimerModel::end() 发出的数据结构
struct TimerEndSnapshot
{
    qint64 elapsedMs = 0;
    int consumMin = 0;
    int counter = 0;
    QString startTime;
    QString endTime;
};

// RecordService：记录业务服务层，依赖方向 UI → RecordService → sqlDBM
// 纯业务层：禁止 include 任何 Widget / UI 头文件，不持有任何 UI 指针
class RecordService : public QObject
{
    Q_OBJECT
public:
    // dbm 为外部注入的原始指针（不在内部 new），注入时 dbm 必须已完全构造完成
    explicit RecordService(sqlDBM *dbm, QObject *parent = nullptr);
    ~RecordService();

    // ---------- 计时控制（封装 TimerModel） ----------
    void timerStart();
    void timerPause();
    void timerEnd();          // 结束计时：停止定时器、记录时间、emit sig_timerEnded
    void timerIncrementCounter();
    void timerMarkSaved();  // 计时记录落库成功后确认：清 isWorking
    qint64 getCurrentTimerMs() const;
    bool isTimerWorking() const;
    bool isTimerRunning() const;
    int timerConsumMinutes() const;
    int timerCounter() const;
    QString timerStartTime() const;
    QString timerEndTime() const;
    QString timerFormatElapsed() const;

    // ---------- 写路径 ----------
    bool saveTimeRecord(const QString &date, const QString &startTime, const QString &endTime,
                        int consumTime, int counter, const QString &detail, const QString &eventClass);
    bool deleteRecord(const QDate &date, int eventId);
    bool updateRecord(const QDate &date, const TimeRecord &rec);
    bool addEmptyRecord(const QDate &date, const TimeRecord &rec);

    // ---------- 查询路径 ----------
    QVector<TimeRecord> queryRecordsOfDay(const QDate &date);
    QMap<QString, int> dayClassConsume(const QDate &date);
    QMap<QString, int> weekCounterStats(const QStringList &weekDays);
    QSet<QString> markedDaysOfMonth(int year, int month);

signals:
    // 计时相关信号（转发自 TimerModel，供 UI 订阅）
    void sig_timeUpdated(qint64 ms);         // 每 10ms 发出，驱动 LCD 刷新
    void sig_alreadyWorking();               // 重复 start() 时提醒
    void sig_notWorking();                   // 未工作时 pause/end 提醒
    void sig_timerEnded(const TimerEndSnapshot &snap);  // 计时结束，携带快照数据
    void sig_stateChanged();                 // 计时状态变更（start/pause/end），驱动按钮状态刷新

    // 业务层数据变更通知
    void dataChanged();

private slots:
    void onDbDataChanged();
    // TimerModel 信号转发
    void onTimerTick(qint64 ms);
    void onTimerEnded(qint64 elapsedMs, int consumMin, int counter,
                      const QString &startTime, const QString &endTime);

private:
    sqlDBM *m_dbM = nullptr;
    TimerModel *m_timerModel = nullptr;

    bool m_suppressForward = false;
    bool m_opNotified = false;
};

#endif // RECORD_SERVICE_H
