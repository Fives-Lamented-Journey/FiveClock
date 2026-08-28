#include "service/record_service.h"
#include "db/idb_access.h"

#include <QDebug>     // qWarning() 日志输出
#include <QtGlobal>   // Q_ASSERT 宏

RecordService::RecordService(IDbAccess *dbm, QObject *parent)
    : QObject(parent)
    , m_dbM(dbm)
{
    if (dbm == nullptr) {
        qWarning() << "RecordService: dbm 为空，服务层无法正常工作";
        return;
    }

    connect(m_dbM, &IDbAccess::dataChanged, this, &RecordService::onDbDataChanged);
}

RecordService::~RecordService()
{
    // m_dbM 为外部注入，不负责释放
}

// ---------- 写路径 ----------

bool RecordService::saveTimeRecord(const QString &date, const QString &startTime, const QString &endTime,
                                   int consumTime, int counter, const QString &detail, const QString &eventClass)
{
    if (m_dbM == nullptr) {
        qWarning() << "RecordService::saveTimeRecord: dbm 为空";
        return false;
    }
    m_suppressForward = true;
    m_opNotified = false;
    m_dbM->insertDataEnsureTable(date, startTime, endTime, consumTime, counter, detail, eventClass);
    m_suppressForward = false;
    if (!m_opNotified) {
        qWarning() << "RecordService::saveTimeRecord: 插入失败";
        return false;
    }
    emit dataChanged();
    return true;
}

bool RecordService::saveFromSnapshot(const TimerEndSnapshot &snap, const QString &date,
                                      const QString &detail, const QString &eventClass)
{
    // 从快照提取计时数据，调用通用保存接口
    return saveTimeRecord(date, snap.startTime, snap.endTime,
                          snap.consumMin, snap.counter, detail, eventClass);
}

bool RecordService::deleteRecord(const QDate &date, int eventId)
{
    if (m_dbM == nullptr) {
        qWarning() << "RecordService::deleteRecord: dbm 为空";
        return false;
    }
    m_suppressForward = true;
    m_opNotified = false;
    m_dbM->deleteRecord(date.toString("yyyy-MM-dd"), eventId);
    m_suppressForward = false;
    if (!m_opNotified) {
        qWarning() << "RecordService::deleteRecord: 删除失败";
        return false;
    }
    emit dataChanged();
    return true;
}

bool RecordService::updateRecord(const QDate &date, const TimeRecord &rec)
{
    if (m_dbM == nullptr) {
        qWarning() << "RecordService::updateRecord: dbm 为空";
        return false;
    }
    m_suppressForward = true;
    m_opNotified = false;
    m_dbM->updateRecord(date.toString("yyyy-MM-dd"), rec.eventId,
                        rec.begTime, rec.endTime, rec.consumTime,
                        rec.counter, rec.detail, rec.eventClass);
    m_suppressForward = false;
    if (!m_opNotified) {
        qWarning() << "RecordService::updateRecord: 更新失败";
        return false;
    }
    emit dataChanged();
    return true;
}

bool RecordService::addEmptyRecord(const QDate &date, const TimeRecord &rec)
{
    if (m_dbM == nullptr) {
        qWarning() << "RecordService::addEmptyRecord: dbm 为空";
        return false;
    }
    m_suppressForward = true;
    m_opNotified = false;
    m_dbM->insertDataEnsureTable(date.toString("yyyy-MM-dd"),
                                  rec.begTime, rec.endTime, rec.consumTime,
                                  rec.counter, rec.detail, rec.eventClass);
    m_suppressForward = false;
    if (!m_opNotified) {
        qWarning() << "RecordService::addEmptyRecord: 插入失败";
        return false;
    }
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

// ---------- 周换算与统计工具（纯业务规则，不触数据库） ----------

QPair<QDate, QDate> RecordService::weekRangeForIndex(int index)
{
    // 周公式与旧版 chartPage 完全一致：索引 0 = 最近一周（today-7 ~ today-1），
    // 索引每 +1 向前推一周，区间连续且不重叠
    const QDate today = QDate::currentDate();
    const QDate start = today.addDays((index + 1) * (-7));
    const QDate end = today.addDays(index * (-7) - 1);
    return {start, end};
}

QStringList RecordService::weekDaysForIndex(int index)
{
    // 该周的 7 天日期列表，从区间起始日起逐日递增
    const QDate start = weekRangeForIndex(index).first;
    QStringList days;
    for (int i = 0; i < 7; ++i) {
        days << start.addDays(i).toString("yyyy-MM-dd");
    }
    return days;
}

int RecordService::sumMinutes(const QMap<QString, int> &classConsume)
{
    int sum = 0;
    for (auto it = classConsume.constBegin(); it != classConsume.constEnd(); ++it) {
        sum += it.value();
    }
    return sum;
}

void RecordService::onDbDataChanged()
{
    if (m_suppressForward) {
        m_opNotified = true;
        return;
    }
    emit dataChanged();
}
