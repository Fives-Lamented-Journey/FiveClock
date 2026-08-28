#ifndef MOCK_DB_ACCESS_H
#define MOCK_DB_ACCESS_H

#include "db/idb_access.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>

#include <gmock/gmock.h>

// MockDbAccess：IDbAccess 的 GMock 测试桩
// 使用内存数据库创建合法的 QSqlQuery 返回值，避免真实磁盘 IO
class MockDbAccess : public IDbAccess
{
public:
    explicit MockDbAccess(QObject *parent = nullptr)
        : IDbAccess(parent)
    {
        // 创建独立的内存数据库连接（MockDbAccess 专用）
        // 使用唯一连接名，避免多实例冲突
        m_connName = QString("MockDbAccess_%1").arg(quintptr(this), 0, 16);
        m_testDb = QSqlDatabase::addDatabase("QSQLITE", m_connName);
        m_testDb.setDatabaseName(":memory:");
        if (!m_testDb.open()) {
            qWarning() << "MockDbAccess: 无法打开内存数据库";
        }
    }

    ~MockDbAccess() override
    {
        if (m_testDb.isOpen()) {
            m_testDb.close();
        }
        QSqlDatabase::removeDatabase(m_connName);
    }

    // 辅助：发射 dataChanged 信号（供测试手动触发写成功路径）
    void emitDataChanged() { emit dataChanged(); }

    // 辅助：在内存库中创建表（测试数据准备）
    void createTable(const QString &tableName)
    {
        QSqlQuery q(m_testDb);
        QString sql = QString(
            "CREATE TABLE IF NOT EXISTS `%1` ("
            "eventID INTEGER PRIMARY KEY AUTOINCREMENT,"
            "startTime TEXT,"
            "endTime TEXT,"
            "consumTime INT,"
            "counter INT,"
            "detail TEXT,"
            "eventclass TEXT);"
        ).arg(tableName);
        q.exec(sql);
    }

    // 辅助：向内存库插入一行测试数据
    void insertTestRow(const QString &tableName,
                       const QString &startTime,
                       const QString &endTime,
                       int consumTime, int counter,
                       const QString &detail,
                       const QString &eventClass)
    {
        QSqlQuery q(m_testDb);
        QString sql = QString(
            "INSERT INTO `%1` (startTime, endTime, consumTime, counter, detail, eventclass) "
            "VALUES ('%2', '%3', %4, %5, '%6', '%7');"
        ).arg(tableName, startTime, endTime)
         .arg(consumTime).arg(counter)
         .arg(detail, eventClass);
        q.exec(sql);
    }

    // 辅助：清空内存库
    void clearDatabase()
    {
        QStringList tables = m_testDb.tables();
        for (const QString &t : tables) {
            QSqlQuery q(m_testDb);
            q.exec(QString("DROP TABLE IF EXISTS `%1`").arg(t));
        }
    }

    // ================================================================
    // IDbAccess 接口 Mock 实现
    // ================================================================

    void insertDataEnsureTable(const QString &tableName,
                               const QString &startTime,
                               const QString &endTime,
                               int consumTime, int counter,
                               const QString &detail,
                               const QString &eventClass) override
    {
        // 默认行为：真正写入内存库 + 发射 dataChanged
        createTable(tableName);
        insertTestRow(tableName, startTime, endTime, consumTime, counter, detail, eventClass);
        emit dataChanged();
    }

    void deleteRecord(const QString &tableName, int eventId) override
    {
        QSqlQuery q(m_testDb);
        QString sql = QString("DELETE FROM `%1` WHERE eventID = %2")
                          .arg(tableName).arg(eventId);
        q.exec(sql);
        emit dataChanged();
    }

    void updateRecord(const QString &tableName, int eventId,
                      const QString &startTime, const QString &endTime,
                      int consumTime, int counter,
                      const QString &detail, const QString &eventClass) override
    {
        QSqlQuery q(m_testDb);
        QString sql = QString(
            "UPDATE `%1` SET startTime='%2', endTime='%3', consumTime=%4, counter=%5, detail='%6', eventclass='%7' "
            "WHERE eventID=%8"
        ).arg(tableName, startTime, endTime)
         .arg(consumTime).arg(counter)
         .arg(detail, eventClass)
         .arg(eventId);
        q.exec(sql);
        emit dataChanged();
    }

    QSqlQuery sortedRecord(const QString &tableName) override
    {
        QSqlQuery q(m_testDb);
        QString sql = QString(
            "SELECT eventID, startTime, endTime, consumTime, counter, detail, eventclass "
            "FROM `%1` ORDER BY startTime"
        ).arg(tableName);
        q.exec(sql);
        return q;
    }

    QSqlQuery consuTimeOfDay(const QString &tableName) override
    {
        QSqlQuery q(m_testDb);
        // 检查表是否存在
        QStringList tables = m_testDb.tables();
        if (!tables.contains(tableName)) {
            return QSqlQuery();  // 返回空查询
        }
        QString sql = QString(
            "SELECT eventclass, SUM(consumTime) AS totalTime "
            "FROM `%1` GROUP BY eventclass ORDER BY totalTime DESC"
        ).arg(tableName);
        q.exec(sql);
        return q;
    }

    QSqlQuery returnSumCounterTime(const QStringList &weekDays) override
    {
        QSqlQuery q(m_testDb);
        QString sql;
        for (int i = 0; i < weekDays.size(); ++i) {
            if (i > 0) sql += " UNION ALL ";
            sql += QString("SELECT '%1' AS table_name, SUM(counter) AS total FROM `%1` WHERE counter > 0")
                       .arg(weekDays.at(i));
        }
        q.exec(sql);
        return q;
    }

    QStringList allTableNames() override
    {
        return m_testDb.tables();
    }

    int getRowCount(const QString &tableName) override
    {
        QSqlQuery q(m_testDb);
        QString sql = QString("SELECT COUNT(*) FROM `%1`").arg(tableName);
        if (q.exec(sql) && q.next()) {
            return q.value(0).toInt();
        }
        return 0;
    }

private:
    QSqlDatabase m_testDb;   // 内存数据库连接（测试数据）
    QString m_connName;     // 唯一连接名
};

#endif // MOCK_DB_ACCESS_H
