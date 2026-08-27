#ifndef SQLDBM_H
#define SQLDBM_H

#include <QObject>     // 基类
#include <QSqlDatabase>// DB 为值成员，需要完整类型
#include <QSqlQuery>   // 多个方法按值返回 QSqlQuery，需要完整类型
#include <QString>
#include <QStringList>

// sqlDBM 类继承自 QObject，用于管理 SQLite 数据库的操作
class sqlDBM:public QObject
{
    Q_OBJECT
public:
    // 构造函数
    sqlDBM();
    // 析构函数
    ~sqlDBM();

    // 定义事件分类的枚举类型
    enum eventClass {
        LeisureandEntertainment = 0,  // 休闲娱乐
        SocialInteraction,           // 社交互动
        FamilyAffection,             // 亲情关怀
        WorkandStudy,                // 工作学习
        SleepandExercise             // 睡眠锻炼
    };

    // 检查指定名称的表是否存在
    bool tableExits(QString &tableName);
    // 获取指定表的行数
    int getRowCount(const QString &tableName);
    // 创建一个新表
    void creatTable(QString &newTableName);
    // 向指定表中插入数据
    void insertData(QString tableName, QString startTime, QString endTime,
                    int consumTime, int counter, QString detail, QString eventClass);
    // 向指定表中插入数据（若表不存在则先创建，业务封装：建表+插入的单一入口）
    void insertDataEnsureTable(QString tableName, QString startTime, QString endTime,
                               int consumTime, int counter, QString detail, QString eventClass);
    // 获取指定表中按开始时间排序的记录
    QSqlQuery sortedRecord(QString tableName);
    // 获取数据库中全部用户表名（批量接口：供日历一次性判断哪些日期有记录）
    QStringList allTableNames();
    // 获取指定日期中按事件分类分组并汇总消耗时间的记录
    QSqlQuery returnConsuTime(QString selectedDay);
    // 获取指定日期的汇总消耗时间记录（业务封装：表不存在时返回空查询）
    QSqlQuery consuTimeOfDay(const QString &selectedDay);
    // 获取指定一周内每天的总计数时间
    QSqlQuery returnSumCounterTime(QStringList selectedWeek);
    // 删除指定表中指定事件 ID 的记录
    void deleteRecord(QString tableName, int eventId);
    // 更新指定表中指定事件 ID 的记录
    void updateRecord(QString tableName, int eventId, QString startTime, QString endTime,
                      int consumTime, int counter, QString detail, QString eventClass);
    // 格式化时间字符串
    QString formatTime(const QString& input);

private:
    // 数据库对象
    QSqlDatabase DB;
};

#endif // SQLDBM_H
