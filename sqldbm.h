#ifndef SQLDBM_H
#define SQLDBM_H

#include <QMainWindow>
#include <QObject>
#include <QWidget>
#include <QSqlDatabase>
#include <QDataWidgetMapper>
#include <qstring.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <QTime>

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

    // 打开数据库
    void openDataBase();
    // 关闭数据库
    void closeDateBase();
    // 检查指定名称的表是否存在
    bool tableExits(QString &tableName);
    // 获取指定表的行数
    int getRowCount(const QString &tableName);
    // 创建一个新表
    void creatTable(QString &newTableName);
    // 向指定表中插入数据
    void insertData(QString tableName, QString startTime, QString endTime,
                    int consumTime, int counter, QString detail, QString eventClass);
    // 获取指定表中按开始时间排序的记录
    QSqlQuery sortedRecord(QString tableName);
    // 获取指定日期中按事件分类分组并汇总消耗时间的记录
    QSqlQuery returnConsuTime(QString selectedDay);
    // 获取指定日期的总消耗时间
    int returnSumConsuTime(QString selectedDay);
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
