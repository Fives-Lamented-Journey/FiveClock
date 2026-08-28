#ifndef IDB_ACCESS_H
#define IDB_ACCESS_H

#include <QObject>     // 基类：信号机制需要 QObject
#include <QSqlQuery>  // 查询接口返回值需要完整类型
#include <QString>
#include <QStringList>

// IDbAccess：数据访问抽象接口
// 定义 RecordService 所需的全部数据库操作，由 sqlDBM 实现
// 依赖方向：Service → IDbAccess → (sqlDBM 实现)
// 本接口仅依赖 Qt 基础类型，严禁依赖任何业务/UI头文件
//
// 设计说明：
//   - 写操作统一为 void 返回值，成功/失败通过 dataChanged 信号通知
//   - 查询操作返回 QSqlQuery，调用方负责迭代结果集
//   - dataChanged 信号：任何写操作成功后由实现类发出
class IDbAccess : public QObject
{
    Q_OBJECT
public:
    explicit IDbAccess(QObject *parent = nullptr) : QObject(parent) {}
    ~IDbAccess() override = default;

    // ================================================================
    // 写操作接口（RecordService 写路径调用）
    // ================================================================

    // 向指定表插入数据（表不存在则自动创建）
    virtual void insertDataEnsureTable(const QString &tableName,
                                       const QString &startTime,
                                       const QString &endTime,
                                       int consumTime, int counter,
                                       const QString &detail,
                                       const QString &eventClass) = 0;

    // 删除指定表中指定 eventId 的记录
    virtual void deleteRecord(const QString &tableName, int eventId) = 0;

    // 更新指定表中指定 eventId 的记录
    virtual void updateRecord(const QString &tableName, int eventId,
                              const QString &startTime,
                              const QString &endTime,
                              int consumTime, int counter,
                              const QString &detail,
                              const QString &eventClass) = 0;

    // ================================================================
    // 查询操作接口（RecordService 查询路径调用）
    // ================================================================

    // 获取指定表中按开始时间排序的全部记录
    virtual QSqlQuery sortedRecord(const QString &tableName) = 0;

    // 获取指定日期按事件分类分组汇总的消耗时间
    virtual QSqlQuery consuTimeOfDay(const QString &tableName) = 0;

    // 获取指定一周内每天的分神计数汇总
    virtual QSqlQuery returnSumCounterTime(const QStringList &weekDays) = 0;

    // 获取数据库中全部用户表名
    virtual QStringList allTableNames() = 0;

    // 获取指定表的行数
    virtual int getRowCount(const QString &tableName) = 0;

signals:
    // 数据变更通知：任何写操作成功后发出
    void dataChanged();
};

#endif // IDB_ACCESS_H
