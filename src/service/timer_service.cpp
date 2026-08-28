#include "service/timer_service.h"
#include "timer/timermodel.h"
#include "config/config_service.h"

TimerService::TimerService(QObject *parent)
    : QObject(parent)
    , m_timerModel(new TimerModel(this))
{
    connect(m_timerModel, &TimerModel::sig_timeUpdated,
            this, &TimerService::onTimerTick);
    connect(m_timerModel, &TimerModel::sig_alreadyWorking,
            this, &TimerService::sig_alreadyWorking);
    connect(m_timerModel, &TimerModel::sig_notWorking,
            this, &TimerService::sig_notWorking);
    connect(m_timerModel, &TimerModel::sig_timerEnded,
            this, &TimerService::onTimerEnded);
}

TimerService::~TimerService()
{
    // m_timerModel 有 this 作为 parent，Qt 自动释放
}

// 注入配置服务，转发给 TimerModel
void TimerService::setConfigService(ConfigService *configService)
{
    m_configService = configService;
    if (m_timerModel) {
        m_timerModel->setConfigService(configService);
    }
}

// ---------- 计时控制 ----------

void TimerService::timerStart()
{
    if (!m_timerModel) return;
    m_timerModel->start();
    emit sig_stateChanged();
}

void TimerService::timerPause()
{
    if (!m_timerModel) return;
    m_timerModel->pause();
    emit sig_stateChanged();
}

void TimerService::timerEnd()
{
    if (!m_timerModel) return;
    m_timerModel->end();
    emit sig_stateChanged();
}

void TimerService::timerIncrementCounter()
{
    if (!m_timerModel) return;
    m_timerModel->incrementCounter();
}

void TimerService::timerMarkSaved()
{
    if (!m_timerModel) return;
    m_timerModel->markSaved();
}

qint64 TimerService::getCurrentTimerMs() const
{
    return m_timerModel ? m_timerModel->elapsedMs() : 0;
}

bool TimerService::isTimerWorking() const
{
    return m_timerModel ? m_timerModel->isWorking() : false;
}

bool TimerService::isTimerRunning() const
{
    return m_timerModel ? m_timerModel->isRunning() : false;
}

int TimerService::timerConsumMinutes() const
{
    return m_timerModel ? m_timerModel->consumMinutes() : 0;
}

int TimerService::timerCounter() const
{
    return m_timerModel ? m_timerModel->counter() : 0;
}

QString TimerService::timerStartTime() const
{
    return m_timerModel ? m_timerModel->startTimeStr() : QString();
}

QString TimerService::timerEndTime() const
{
    return m_timerModel ? m_timerModel->endTimeStr() : QString();
}

QString TimerService::timerFormatElapsed() const
{
    return m_timerModel ? m_timerModel->formatElapsed() : QString();
}

// ---------- TimerModel 信号转发 ----------

void TimerService::onTimerTick(qint64 ms)
{
    emit sig_timeUpdated(ms);
}

void TimerService::onTimerEnded(qint64 elapsedMs, int consumMin, int counter,
                                  const QString &startTime, const QString &endTime)
{
    TimerEndSnapshot snap;
    snap.elapsedMs = elapsedMs;
    snap.consumMin = consumMin;
    snap.counter = counter;
    snap.startTime = startTime;
    snap.endTime = endTime;
    emit sig_timerEnded(snap);
}
