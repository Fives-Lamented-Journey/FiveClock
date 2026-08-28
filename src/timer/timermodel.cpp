#include "timer/timermodel.h"
#include "config/config_service.h"
#include <QTimer>
#include <QDateTime>

TimerModel::TimerModel(QObject *parent)
    : QObject(parent)
    , m_tickTimer(new QTimer(this))
{
    // 使用 ConfigService 常量（未注入时使用默认值）
    int tickMs = m_configService ? m_configService->timerTickIntervalMs() : 10;
    m_tickTimer->setInterval(tickMs);
    m_tickTimer->setTimerType(Qt::PreciseTimer);
    m_tickTimer->stop();
    connect(m_tickTimer, &QTimer::timeout, this, &TimerModel::onTick);
}

TimerModel::~TimerModel()
{
    // m_tickTimer 有 this 作为 parent，Qt 自动释放
}

// 注入配置服务
void TimerModel::setConfigService(ConfigService *configService)
{
    m_configService = configService;
    // 若定时器尚未激活，立即应用新的 tick 间隔
    if (m_tickTimer && !m_tickTimer->isActive()) {
        int tickMs = configService ? configService->timerTickIntervalMs() : 10;
        m_tickTimer->setInterval(tickMs);
    }
}

// ---------- 计时控制 ----------

void TimerModel::start()
{
    if (m_isWorking) {
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
        m_tickTimer->stop();
        m_pausedAccum += m_elapsedTimer.restart();
        m_isRunning = false;
    } else {
        m_tickTimer->start();
        m_elapsedTimer.restart();
        m_isRunning = true;
    }
    m_counter++;
}

bool TimerModel::end()
{
    if (!m_isWorking) {
        emit sig_notWorking();
        return false;
    }
    m_tickTimer->stop();
    m_endTime = QDateTime::currentDateTime().toString("HH:mm:ss");
    qint64 finalMs = elapsedMs();
    int consumMin = consumMinutes();
    int cnt = m_counter;
    QString startStr = m_startTime;
    QString endStr = m_endTime;

    m_pausedAccum = 0;
    m_isRunning = false;

    emit sig_timerEnded(finalMs, consumMin, cnt, startStr, endStr);
    return true;
}

// ---------- 状态查询 ----------

qint64 TimerModel::elapsedMs() const
{
    qint64 current = m_elapsedTimer.isValid() ? m_elapsedTimer.elapsed() : 0;
    return m_pausedAccum + current;
}

int TimerModel::consumMinutes() const
{
    int msPerMin = m_configService ? m_configService->msPerMinute() : 60000;
    return int(elapsedMs() / msPerMin);
}

int TimerModel::counter() const
{
    return m_counter;
}

QString TimerModel::formatElapsed() const
{
    qint64 ms = elapsedMs();
    int msPerHour = m_configService ? m_configService->msPerHour() : 3600000;
    int msPerMin = m_configService ? m_configService->msPerMinute() : 60000;
    int msPerSec = m_configService ? m_configService->msPerSecond() : 1000;

    int h = int(ms / msPerHour);
    int m = int((ms % msPerHour) / msPerMin);
    int s = int((ms % msPerMin) / msPerSec);

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
    m_isWorking = false;
}

// ---------- 私有槽 ----------

void TimerModel::onTick()
{
    emit sig_timeUpdated(elapsedMs());
}
