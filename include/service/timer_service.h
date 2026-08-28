#ifndef TIMER_SERVICE_H
#define TIMER_SERVICE_H

#include <QObject>
#include <QString>

#include "dto/time_record.h"   // TimerEndSnapshot DTO 定义

// 仅以指针形式使用，前向声明替代 include（TimerService 持有 TimerModel*）
class TimerModel;
class ConfigService;

// TimerService：计时业务服务层
// 依赖方向：UI → TimerService → TimerModel
// 纯业务层：禁止 include 任何 Widget / UI / 数据库头文件，不持有任何 UI/DB 指针
class TimerService : public QObject
{
    Q_OBJECT
public:
    explicit TimerService(QObject *parent = nullptr);
    ~TimerService();

    // 注入配置服务（转发给 TimerModel）
    void setConfigService(ConfigService *configService);

    // ================================================================
    // 计时控制接口（封装 TimerModel，UI 通过以下接口操作计时）
    // ================================================================
    void timerStart();              // 开始计时；若已在工作中则 emit sig_alreadyWorking
    void timerPause();              // 暂停/恢复切换；若未在工作中则 emit sig_notWorking
    void timerEnd();                // 结束计时；停止定时器、记录时间、emit sig_timerEnded
    void timerIncrementCounter();   // 分神计数 +1
    void timerMarkSaved();          // 计时记录落库成功后确认，清 isWorking
    qint64 getCurrentTimerMs() const;          // 当前瞬时流逝毫秒数
    bool isTimerWorking() const;               // 是否已开始工作
    bool isTimerRunning() const;               // 是否正在运行（非暂停）
    int timerConsumMinutes() const;            // 消耗分钟数
    int timerCounter() const;                  // 分神次数
    QString timerStartTime() const;            // 开始时间字符串
    QString timerEndTime() const;              // 结束时间字符串
    QString timerFormatElapsed() const;        // 格式化流逝时间 "hh:mm:ss"

signals:
    // 计时相关信号（转发自 TimerModel，供 UI 订阅）
    void sig_timeUpdated(qint64 ms);                          // 每 10ms 发出，驱动 LCD 刷新
    void sig_alreadyWorking();                                // 重复 start() 时提醒
    void sig_notWorking();                                    // 未工作时 pause/end 提醒
    void sig_timerEnded(const TimerEndSnapshot &snap);        // 计时结束，携带快照数据
    void sig_stateChanged();                                  // 计时状态变更（start/pause/end），驱动按钮状态刷新

private slots:
    void onTimerTick(qint64 ms);                              // TimerModel::sig_timeUpdated 转发
    void onTimerEnded(qint64 elapsedMs, int consumMin, int counter,
                      const QString &startTime, const QString &endTime); // TimerModel::sig_timerEnded 转发

private:
    TimerModel *m_timerModel = nullptr;                       // 计时业务状态模型（内部持有，Qt 父子关系自动释放）
    ConfigService *m_configService = nullptr;                  // 注入的配置服务指针
};

#endif // TIMER_SERVICE_H
