#ifndef SQLDBM_H
#define SQLDBM_H

#include "db/idb_access.h"   // 继承 IDbAccess，实现数据访问抽象接口
#include <QSqlDatabase>      // DB 为值成员，需要完整类型

// sqlDBM 类继承自 IDbAccess（→ QObject），实现 SQLite 数据访问
class sqlDBM : public IDbAccess
{
    Q_OBJECT
public:
    // 构造函数
    sqlDBM();
    // 析构函数
    ~sqlDBM() override;

    // 检查指定名称的表是否存在（辅助方法，未纳入接口）
    bool tableExits(QString &tableName);
    // 创建一个新表（辅助方法，未纳入接口）
    void creatTable(QString &newTableName);
    // 向指定表中插入数据（辅助方法，未纳入接口）
    void insertData(QString tableName, QString startTime, QString endTime,
                    int consumTime, int counter, QString detail, QString eventClass);
    // 格式化时间字符串（辅助方法，未纳入接口）
    QString formatTime(const QString& input);

    // ================================================================
    // IDbAccess 接口实现
    // ================================================================

    // 向指定表中插入数据（若表不存在则先创建）
    void insertDataEnsureTable(const QString &tableName,
                               const QString &startTime,
                               const QString &endTime,
                               int consumTime, int counter,
                               const QString &detail,
                               const QString &eventClass) override;

    // 删除指定表中指定事件 ID 的记录
    void deleteRecord(const QString &tableName, int eventId) override;

    // 更新指定表中指定事件 ID 的记录
    void updateRecord(const QString &tableName, int eventId,
                      const QString &startTime, const QString &endTime,
                      int consumTime, int counter,
                      const QString &detail, const QString &eventClass) override;

    // 获取指定表中按开始时间排序的记录
    QSqlQuery sortedRecord(const QString &tableName) override;

    // 获取指定日期的汇总消耗时间记录（业务封装：表不存在时返回空查询）
    QSqlQuery consuTimeOfDay(const QString &tableName) override;

    // 获取指定一周内每天的总计数时间
    QSqlQuery returnSumCounterTime(const QStringList &weekDays) override;

    // 获取数据库中全部用户表名
    QStringList allTableNames() override;

    // 获取指定表的行数
    int getRowCount(const QString &tableName) override;

private:
    // 数据库对象
    QSqlDatabase DB;
};

#endif // SQLDBM_H
