#ifndef TIMERMODEL_H
#define TIMERMODEL_H

#include <QObject>
#include <QElapsedTimer>
#include <QString>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

// TimerModel：计时业务状态模型
// 封装 QElapsedTimer + QTimer(10ms) 实现的累加计时逻辑
// 仅负责计时状态与信号发出，不涉及任何 UI / 数据库
class TimerModel : public QObject
{
    Q_OBJECT
public:
    explicit TimerModel(QObject *parent = nullptr);
    ~TimerModel();

    // ---------- 计时控制 ----------
    // 开始计时（重复调用时 emit workingNow 并忽略）
    void start();
    // 暂停/恢复切换：运行中→暂停，暂停中→恢复
    void pause();
    // 结束计时（停止定时器、记录结束时间、准备下一轮）
    // 返回 true 表示本轮计时有效（isWorking==true），调用方可据此决定是否保存
    bool end();

    // ---------- 状态查询 ----------
    // 当前瞬时流逝毫秒数（含暂停累加）
    qint64 elapsedMs() const;
    // 消耗分钟数（elapsedMs / 60000）
    int consumMinutes() const;
    // 当前分神次数
    int counter() const;
    // 是否正在运行（start_timer 激活中）
    bool isRunning() const { return m_isRunning; }
    // 是否已开始工作（isRunning 或曾在计时中）
    bool isWorking() const { return m_isWorking; }
    // 开始时间字符串
    QString startTimeStr() const { return m_startTime; }
    // 结束时间字符串
    QString endTimeStr() const { return m_endTime; }
    // 格式化流逝时间为 "hh:mm:ss"
    QString formatElapsed() const;

    // 计数（分神+1）
    void incrementCounter();
    // 保存确认：计时记录落库成功后调用，清 isWorking（原代码由 assAddRecord 负责）
    void markSaved();

signals:
    // 计时运行中每 10ms 发出一次，携带当前累计毫秒数
    void sig_timeUpdated(qint64 ms);
    // 调用 start() 时若已在工作中，发出提醒（供 UI 提示）
    void sig_alreadyWorking();
    // 调用 pause() 时若未在工作中，发出提醒
    void sig_notWorking();
    // end() 完成后发出，携带本次计时的快照数据
    void sig_timerEnded(qint64 elapsedMs, int consumMin, int counter,
                        const QString &startTime, const QString &endTime);

private slots:
    // 10ms 定时器超时：累加流逝时间并发出 sig_timeUpdated
    void onTick();

private:
    // 10ms 精度定时器，驱动 LCD 刷新
    QTimer *m_tickTimer = nullptr;
    // 实际流逝时间计时器（暂停时 stop，恢复时 restart）
    QElapsedTimer m_elapsedTimer;

    // 暂停期间已累加的毫秒数
    qint64 m_pausedAccum = 0;
    // 分神次数
    int m_counter = 0;
    // 是否正在计时（tickTimer 激活中）
    bool m_isRunning = false;
    // 是否已开始工作
    bool m_isWorking = false;

    // 本轮开始/结束时间字符串（HH:mm:ss）
    QString m_startTime;
    QString m_endTime;
};

#endif // TIMERMODEL_H
