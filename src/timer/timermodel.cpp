#include "timer/timermodel.h"
#include <QTimer>
#include <QDateTime>

TimerModel::TimerModel(QObject *parent)
    : QObject(parent)
    , m_tickTimer(new QTimer(this))
{
    m_tickTimer->setInterval(10);
    m_tickTimer->setTimerType(Qt::PreciseTimer);
    m_tickTimer->stop();
    connect(m_tickTimer, &QTimer::timeout, this, &TimerModel::onTick);
}

TimerModel::~TimerModel()
{
    // m_tickTimer 有 this 作为 parent，Qt 自动释放
}

// ---------- 计时控制 ----------

void TimerModel::start()
{
    if (m_isWorking) {
        // 已在工作中，发出提醒后忽略
        emit sig_alreadyWorking();
        return;
    }
    m_isRunning = true;
    m_isWorking = true;
    m_counter = 0;
    m_pausedAccum = 0;
    m_startTime = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_endTime.clear();
    m_tickTimer->start();
    m_elapsedTimer.start();
}

void TimerModel::pause()
{
    if (!m_isWorking) {
        emit sig_notWorking();
        return;
    }

    if (m_isRunning) {
        // 运行中 → 暂停：停止 tickTimer，累加已流逝时间
        m_tickTimer->stop();
        m_pausedAccum += m_elapsedTimer.restart();
        m_isRunning = false;
    } else {
        // 暂停中 → 恢复：重启 tickTimer 与 elapsedTimer
        m_tickTimer->start();
        m_elapsedTimer.restart();
        m_isRunning = true;
    }
    // 每次 pause 切换都累加一次分神计数
    m_counter++;
}

bool TimerModel::end()
{
    if (!m_isWorking) {
        emit sig_notWorking();
        return false;
    }
    // 停止定时器
    m_tickTimer->stop();
    // 记录结束时间
    m_endTime = QDateTime::currentDateTime().toString("HH:mm:ss");
    // 计算最终流逝毫秒数
    qint64 finalMs = elapsedMs();
    int consumMin = consumMinutes();
    int cnt = m_counter;
    QString startStr = m_startTime;
    QString endStr = m_endTime;

    // 重置暂停累加，准备下一轮
    m_pausedAccum = 0;
    m_isRunning = false;

    // 发出结束信号，携带完整快照数据
    emit sig_timerEnded(finalMs, consumMin, cnt, startStr, endStr);
    return true;
}

// ---------- 状态查询 ----------

qint64 TimerModel::elapsedMs() const
{
    // 暂停中：累加值 + 当前 elapsedTimer 已流逝部分
    // 运行中：累加值 + 当前 elapsedTimer 已流逝部分
    // 未开始/已结束：仅累加值
    qint64 current = m_elapsedTimer.isValid() ? m_elapsedTimer.elapsed() : 0;
    return m_pausedAccum + current;
}

int TimerModel::consumMinutes() const
{
    return int(elapsedMs() / 60000);
}

int TimerModel::counter() const
{
    return m_counter;
}

QString TimerModel::formatElapsed() const
{
    qint64 ms = elapsedMs();
    int h = int(ms / (1000 * 60 * 60));
    int m = int((ms % (1000 * 60 * 60)) / (1000 * 60));
    int s = int((ms % (1000 * 60)) / 1000);
    return QString("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

void TimerModel::incrementCounter()
{
    m_counter++;
}

void TimerModel::markSaved()
{
    // 保存确认：记录已成功落库，清除 isWorking 状态
    m_isWorking = false;
}

// ---------- 私有槽 ----------

void TimerModel::onTick()
{
    // 10ms 刷新：发出当前累计毫秒数
    emit sig_timeUpdated(elapsedMs());
}
