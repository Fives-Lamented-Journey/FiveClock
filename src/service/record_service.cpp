#include "service/record_service.h"
#include "db/sqldbm.h"
#include "timer/timermodel.h"

RecordService::RecordService(sqlDBM *dbm, QObject *parent)
    : QObject(parent)
    , m_dbM(dbm)
    , m_timerModel(new TimerModel(this))
{
    // dbm 必须先于本服务完全构造完成（由 assemPage 保证注入顺序）
    connect(m_dbM, &sqlDBM::dataChanged, this, &RecordService::onDbDataChanged);

    // 连接 TimerModel 信号并转发给 UI 层
    connect(m_timerModel, &TimerModel::sig_timeUpdated,
            this, &RecordService::onTimerTick);
    connect(m_timerModel, &TimerModel::sig_alreadyWorking,
            this, &RecordService::sig_alreadyWorking);
    connect(m_timerModel, &TimerModel::sig_notWorking,
            this, &RecordService::sig_notWorking);
    connect(m_timerModel, &TimerModel::sig_timerEnded,
            this, &RecordService::onTimerEnded);
}

RecordService::~RecordService()
{
    // m_timerModel 有 this 作为 parent，Qt 自动释放
}

// ---------- 计时控制 ----------

void RecordService::timerStart()         { m_timerModel->start(); emit sig_stateChanged(); }
void RecordService::timerPause()          { m_timerModel->pause(); emit sig_stateChanged(); }
void RecordService::timerEnd()            { m_timerModel->end(); emit sig_stateChanged(); }
void RecordService::timerIncrementCounter() { m_timerModel->incrementCounter(); }
void RecordService::timerMarkSaved()       { m_timerModel->markSaved(); }

qint64 RecordService::getCurrentTimerMs() const { return m_timerModel->elapsedMs(); }
bool RecordService::isTimerWorking()     const { return m_timerModel->isWorking(); }
bool RecordService::isTimerRunning()     const { return m_timerModel->isRunning(); }
int  RecordService::timerConsumMinutes() const { return m_timerModel->consumMinutes(); }
int  RecordService::timerCounter()       const { return m_timerModel->counter(); }
QString RecordService::timerStartTime()  const { return m_timerModel->startTimeStr(); }
QString RecordService::timerEndTime()    const { return m_timerModel->endTimeStr(); }
QString RecordService::timerFormatElapsed() const { return m_timerModel->formatElapsed(); }

// ---------- TimerModel 信号转发 ----------

void RecordService::onTimerTick(qint64 ms)
{
    emit sig_timeUpdated(ms);
}

void RecordService::onTimerEnded(qint64 elapsedMs, int consumMin, int counter,
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

// ---------- 写路径 ----------

bool RecordService::saveTimeRecord(const QString &date, const QString &startTime, const QString &endTime,
                                   int consumTime, int counter, const QString &detail, const QString &eventClass)
{
    if (m_dbM == nullptr) return false;
    m_suppressForward = true;
    m_opNotified = false;
    m_dbM->insertDataEnsureTable(date, startTime, endTime, consumTime, counter, detail, eventClass);
    m_suppressForward = false;
    if (!m_opNotified) return false;
    emit dataChanged();
    return true;
}

bool RecordService::deleteRecord(const QDate &date, int eventId)
{
    if (m_dbM == nullptr) return false;
    m_suppressForward = true;
    m_opNotified = false;
    m_dbM->deleteRecord(date.toString("yyyy-MM-dd"), eventId);
    m_suppressForward = false;
    if (!m_opNotified) return false;
    emit dataChanged();
    return true;
}

bool RecordService::updateRecord(const QDate &date, const TimeRecord &rec)
{
    if (m_dbM == nullptr) return false;
    m_suppressForward = true;
    m_opNotified = false;
    m_dbM->updateRecord(date.toString("yyyy-MM-dd"), rec.eventId,
                        rec.begTime, rec.endTime, rec.consumTime,
                        rec.counter, rec.detail, rec.eventClass);
    m_suppressForward = false;
    if (!m_opNotified) return false;
    emit dataChanged();
    return true;
}

bool RecordService::addEmptyRecord(const QDate &date, const TimeRecord &rec)
{
    if (m_dbM == nullptr) return false;
    m_suppressForward = true;
    m_opNotified = false;
    m_dbM->insertDataEnsureTable(date.toString("yyyy-MM-dd"),
                                  rec.begTime, rec.endTime, rec.consumTime,
                                  rec.counter, rec.detail, rec.eventClass);
    m_suppressForward = false;
    if (!m_opNotified) return false;
    emit dataChanged();
    return true;
}

// ---------- 查询路径 ----------

QVector<TimeRecord> RecordService::queryRecordsOfDay(const QDate &date)
{
    QVector<TimeRecord> records;
    if (m_dbM == nullptr) return records;
    QSqlQuery query = m_dbM->sortedRecord(date.toString("yyyy-MM-dd"));
    while (query.next()) {
        TimeRecord r;
        r.eventId = query.value(0).toInt();
        r.begTime = query.value(1).toString();
        r.endTime = query.value(2).toString();
        r.consumTime = query.value(3).toInt();
        r.counter = query.value(4).toInt();
        r.detail = query.value(5).toString();
        r.eventClass = query.value(6).toString();
        records.append(r);
    }
    return records;
}

QMap<QString, int> RecordService::dayClassConsume(const QDate &date)
{
    QMap<QString, int> result;
    if (m_dbM == nullptr) return result;
    QSqlQuery query = m_dbM->consuTimeOfDay(date.toString("yyyy-MM-dd"));
    while (query.next()) {
        result.insert(query.value(0).toString(), query.value(1).toInt());
    }
    return result;
}

QMap<QString, int> RecordService::weekCounterStats(const QStringList &weekDays)
{
    QMap<QString, int> result;
    if (m_dbM == nullptr) return result;
    QSqlQuery query = m_dbM->returnSumCounterTime(weekDays);
    if (!query.next()) return result;
    query.seek(-1);
    while (query.next()) {
        result.insert(query.value(0).toString(), query.value(1).toInt());
    }
    return result;
}

QSet<QString> RecordService::markedDaysOfMonth(int year, int month)
{
    QSet<QString> marked;
    if (m_dbM == nullptr) return marked;
    const QString prefix = QDate(year, month, 1).toString("yyyy-MM-");
    const QStringList tables = m_dbM->allTableNames();
    for (const QString &t : tables) {
        if (t.startsWith(prefix) && m_dbM->getRowCount(t) > 0) {
            marked.insert(t);
        }
    }
    return marked;
}

void RecordService::onDbDataChanged()
{
    if (m_suppressForward) {
        m_opNotified = true;
        return;
    }
    emit dataChanged();
}
