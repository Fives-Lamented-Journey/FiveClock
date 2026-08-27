#include "sqldbm.h"
#include <QTime>
#include <QSqlError>   // lastError().text() 需要完整类型

// 构造函数，初始化数据库连接
sqlDBM::sqlDBM() {
    // 添加 SQLite 数据库驱动
    DB = QSqlDatabase::addDatabase("QSQLITE");
    // 设置数据库名称
    DB.setDatabaseName("coreTime.db");
    // 尝试打开数据库
    if (!DB.open()) {
        // 若打开失败，输出错误信息
        qDebug() << "无法打开数据库: " << DB.lastError().text();
        return;
    }
}

// 析构函数，关闭数据库连接
sqlDBM::~sqlDBM()
{
    // 仅关闭连接，不 removeDatabase
    if (DB.isOpen()) {
        DB.close();
    }
}

// 检查指定名称的表是否存在
bool sqlDBM::tableExits(QString &tableName) {
    // 通过检查数据库中的表名列表来判断表是否存在
    return DB.tables().contains(tableName);
}

// 获取指定表的行数
int sqlDBM::getRowCount(const QString& tableName) {
    QSqlQuery query;
    // 构建查询行数的 SQL 语句
    QString countQuery = QString("SELECT COUNT(*) FROM `%1`").arg(tableName);
    // 执行查询
    if (query.exec(countQuery)) {
        // 若查询成功，移动到下一条记录
        if (query.next()) {
            // 返回查询结果的整数值
            return query.value(0).toInt();
        }
    }
    // 若查询失败，输出错误信息
    qDebug() << "查询行数失败: " << query.lastError().text();
    return -1;
}

// 创建一个新表
void sqlDBM::creatTable(QString &newTableName) {
    QSqlQuery query;
    // 构建创建表的 SQL 语句
    QString createQuery = QString("CREATE TABLE `%1` ("
                                  "eventID INTEGER PRIMARY KEY AUTOINCREMENT,"
                                  "startTime TEXT ,"
                                  "endTime TEXT,"
                                  "consumTime INT,"
                                  "counter INT,"
                                  "detail TEXT,"
                                  "eventclass TEXT);")
                              .arg(newTableName);
    // 执行创建表的操作
    if (!query.exec(createQuery)) {
        // 若创建失败，输出错误信息
        qDebug() << "创建表失败: " << query.lastError().text();
    }
}

// 向指定表中插入数据
void sqlDBM::insertData(QString tableName, QString startTime, QString endTime,
                        int consumTime, int counter, QString detail, QString eventClass) {
    QSqlQuery query;
    // 格式化开始时间和结束时间
    startTime = formatTime(startTime);
    endTime = formatTime(endTime);
    // 构建插入数据的 SQL 语句
    QString insertQuery = QString("INSERT INTO `%1` "
                                  "(startTime, endTime,consumTime ,counter, detail, eventclass) "
                                  "VALUES "
                                  "(:startTime, :endTime, :consumTime,"
                                  ":counter, :detail, :eventClass);")
                              .arg(tableName);
    // 准备 SQL 语句
    query.prepare(insertQuery);
    // 绑定参数
    query.bindValue(":startTime", startTime);
    query.bindValue(":endTime", endTime);
    query.bindValue(":consumTime", consumTime);
    query.bindValue(":counter", counter);
    query.bindValue(":detail", detail);
    query.bindValue(":eventClass", eventClass);
    // 执行插入操作
    if (!query.exec()) {
        // 若插入失败，输出错误信息
        qDebug() << "插入数据失败: " << query.lastError().text();
    }
}

// 向指定表中插入数据（若表不存在则先创建）
void sqlDBM::insertDataEnsureTable(QString tableName, QString startTime, QString endTime,
                                   int consumTime, int counter, QString detail, QString eventClass)
{
    if (!tableExits(tableName))
    {
        // 如果表不存在，则创建表
        creatTable(tableName);
        // 插入数据到表中
        insertData(tableName, startTime, endTime, consumTime, counter, detail, eventClass);
        qDebug("addRecord02");
    }
    else
    {
        // 插入数据到已存在的表中
        insertData(tableName, startTime, endTime, consumTime, counter, detail, eventClass);
        qDebug("addRecord03");
    }
}

// 删除指定表中指定事件 ID 的记录
void sqlDBM::deleteRecord(QString tableName, int eventId) {
    QSqlQuery query;
    // 输出表名和事件 ID 用于调试
    qDebug() << tableName;
    qDebug() << eventId;
    // 构建删除记录的 SQL 语句
    QString deleteQuery = QString("DELETE FROM `%1` WHERE eventID = :eventID;")
                              .arg(tableName);
    // 准备 SQL 语句
    query.prepare(deleteQuery);
    // 绑定参数
    query.bindValue(":eventID", eventId);
    // 执行删除操作
    if (!query.exec()) {
        // 若删除失败，输出错误信息
        qDebug() << "删除记录失败: " << query.lastError().text();
    }
}

// 更新指定表中指定事件 ID 的记录
void sqlDBM::updateRecord(QString tableName, int eventId, QString startTime, QString endTime,
                          int consumTime, int counter, QString detail, QString eventClass)
{
    QSqlQuery query;
    // 构建更新记录的 SQL 语句
    QString upDateQuery = QString("UPDATE `%1` "
                                  "SET startTime = :startTime,"
                                  "endTime = :endTime,"
                                  "consumTime = :consumTime,"
                                  "counter = :counter,"
                                  "detail = :detail,"
                                  "eventclass =:eventClass "
                                  "WHERE eventID = :eventId;")
                              .arg(tableName);
    // 准备 SQL 语句
    query.prepare(upDateQuery);
    // 绑定参数
    query.bindValue(":startTime", startTime);
    query.bindValue(":endTime", endTime);
    query.bindValue(":consumTime", consumTime);
    query.bindValue(":counter", counter);
    query.bindValue(":detail", detail);
    query.bindValue(":eventClass", eventClass);
    query.bindValue(":eventId", eventId);
    // 执行更新操作
    if (!query.exec()) {
        // 若更新失败，输出错误信息
        qDebug() << "更新记录失败: " << query.lastError().text();
    }
}

// 获取指定表中按开始时间排序的记录
QSqlQuery sqlDBM::sortedRecord(QString tableName) {
    QSqlQuery query;
    // 输出表名用于调试
    qDebug() << tableName;
    // 构建排序记录的 SQL 语句
    QString sortQuery = QString("SELECT eventID, startTime, endTime, consumTime,counter,detail, eventclass "
                                "FROM `%1` "
                                "ORDER BY startTime;")
                            .arg(tableName);
    // 准备 SQL 语句
    query.prepare(sortQuery);
    // 执行查询操作
    if (!query.exec()) {
        // 若查询失败，输出错误信息
        qDebug() << "排序记录失败: " << query.lastError().text();
    }
    return query;
}

// 获取指定日期中按事件分类分组并汇总消耗时间的记录
QSqlQuery sqlDBM::returnConsuTime(QString selectedDay)
{
    QSqlQuery query;
    // 构建分组汇总的 SQL 语句
    QString groupAndSumQuery = QString("SELECT eventclass,SUM(consumTime) AS totalTime "
                                       "FROM `%1`"
                                       "GROUP BY eventclass "
                                       "ORDER BY totalTime DESC;").arg(selectedDay);
    // 执行查询操作
    if (!query.exec(groupAndSumQuery)) {
        // 若查询失败，输出错误信息
        qDebug() << "分组记录失败: " << query.lastError().text();
    }
    return query;
}

// 获取指定日期的汇总消耗时间记录（业务封装：表不存在时返回空查询）
QSqlQuery sqlDBM::consuTimeOfDay(const QString &selectedDay)
{
    QString day = selectedDay;
    if (!tableExits(day))
    {
        // 如果表不存在，则返回空查询
        return QSqlQuery();
    }
    qDebug() << "assReturnRecord called with selectedDay:" << day;
    // 调用数据库管理类的返回消耗时间记录方法
    return returnConsuTime(day);
}

// 获取指定一周内每天的总计数时间
QSqlQuery sqlDBM::returnSumCounterTime(QStringList selectedWeek)
{
    QSqlQuery query;
    // 构建查询一周内每天总计数时间的 SQL 语句
    QString sql =
        // 使用 SUM(counter) 并添加 WHERE counter > 0 条件
        QString("SELECT '%1' AS table_name, SUM(counter) AS total FROM `%1` WHERE counter > 0 ")
            .arg(selectedWeek.at(0)) +
        QString("UNION ALL SELECT '%1' AS table_name, SUM(counter) FROM `%1` WHERE counter > 0 ")
            .arg(selectedWeek.at(1)) +
        QString("UNION ALL SELECT '%1' AS table_name, SUM(counter) FROM `%1` WHERE counter > 0 ")
            .arg(selectedWeek.at(2)) +
        QString("UNION ALL SELECT '%1' AS table_name, SUM(counter) FROM `%1` WHERE counter > 0 ")
            .arg(selectedWeek.at(3)) +
        QString("UNION ALL SELECT '%1' AS table_name, SUM(counter) FROM `%1` WHERE counter > 0 ")
            .arg(selectedWeek.at(4)) +
        QString("UNION ALL SELECT '%1' AS table_name, SUM(counter) FROM `%1` WHERE counter > 0 ")
            .arg(selectedWeek.at(5)) +
        QString("UNION ALL SELECT '%1' AS table_name, SUM(counter) FROM `%1` WHERE counter > 0;")
            .arg(selectedWeek.at(6));

    // 执行查询操作
    if (!query.exec(sql)) {
        // 若查询失败，输出错误信息
        qDebug() << "查询周分神数据失败: " << query.lastError().text();
    }
    // 尝试移动到第一条记录
    if (!query.next())
        qDebug() << "查询结果为空";

    return query;
}

// 格式化时间字符串
QString sqlDBM::formatTime(const QString &input)
{
    // 尝试按 "h:mm" 格式解析时间
    QTime time = QTime::fromString(input, "h:mm");
    if (time.isValid()) return time.toString("hh:mm:ss");

    // 尝试按 "h:m" 格式解析时间（分钟可能只有一位）
    time = QTime::fromString(input, "h:m");
    if (time.isValid()) return time.toString("hh:mm:ss");

    // 尝试按 "hh:mm" 格式解析时间
    time = QTime::fromString(input, "hh:mm");
    if (time.isValid()) return time.toString("hh:mm:ss");

    // 其他格式，使用手动补全
    return input;
}
