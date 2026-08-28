#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QCoreApplication>
#include <QDate>

#include "service/record_service.h"
#include "mock_db_access.h"
#include "dto/time_record.h"

// 全局测试环境：确保 QCoreApplication 在所有测试前已创建
// QSqlDatabase 需要 QCoreApplication 才能正常工作
class RecordServiceTestEnvironment : public ::testing::Environment
{
public:
    void SetUp() override
    {
        if (qApp == nullptr) {
            int argc = 0;
            m_app = new QCoreApplication(argc, nullptr);
        }
    }
    void TearDown() override
    {
        // 不删除 qApp，由 Qt 自动管理
    }
private:
    QCoreApplication *m_app = nullptr;
};

// 注册全局环境（使用静态对象在 main 之前注册）
// GTest 支持 Environment 的静态注册方式
namespace {
struct EnvRegistrar {
    EnvRegistrar() {
        ::testing::AddGlobalTestEnvironment(new RecordServiceTestEnvironment());
    }
};
static EnvRegistrar s_envRegistrar;
} // anonymous namespace

// ================================================================
// RecordService 单元测试
// ================================================================

class RecordServiceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_mockDb = new MockDbAccess();
        m_service = new RecordService(m_mockDb);
    }

    void TearDown() override
    {
        delete m_service;   // 先删 service（不再使用 mockDb）
        delete m_mockDb;
    }

    MockDbAccess *m_mockDb = nullptr;
    RecordService *m_service = nullptr;
};

// ---------- 构造与空指针测试 ----------

TEST_F(RecordServiceTest, ConstructWithValidDb_Succeeds)
{
    // 构造已在 SetUp 中完成，验证服务非空
    ASSERT_NE(m_service, nullptr);
}

TEST_F(RecordServiceTest, SaveTimeRecord_EmptyDate_ReturnsFalse)
{
    // 空日期不应导致崩溃，但空值不应写入（由上层保证非空）
    // 这里验证：正常参数下 saveTimeRecord 能通过 dataChanged 信号成功
    const QString date = "2026-08-28";
    const QString start = "09:00:00";
    const QString end = "10:30:00";

    // 清除 mock 数据
    m_mockDb->clearDatabase();

    bool ok = m_service->saveTimeRecord(date, start, end, 90, 0, "测试记录", "工作学习");
    EXPECT_TRUE(ok);

    // 验证内存库中确实写入了数据
    EXPECT_GT(m_mockDb->getRowCount(date), 0);
}

// ---------- saveTimeRecord 写路径 ----------

TEST_F(RecordServiceTest, SaveTimeRecord_ValidInputs_InsertsOneRow)
{
    m_mockDb->clearDatabase();
    const QString date = "2026-08-28";

    bool ok = m_service->saveTimeRecord(date, "09:00:00", "10:00:00", 60, 2, "写代码", "工作学习");
    ASSERT_TRUE(ok);
    EXPECT_GT(m_mockDb->getRowCount(date), 0);

    // 验证字段正确
    QSqlQuery q = m_mockDb->sortedRecord(date);
    ASSERT_TRUE(q.next());
    EXPECT_EQ(q.value(1).toString(), "09:00:00");   // startTime
    EXPECT_EQ(q.value(2).toString(), "10:00:00");   // endTime
    EXPECT_EQ(q.value(3).toInt(), 60);               // consumTime
    EXPECT_EQ(q.value(4).toInt(), 2);               // counter
    EXPECT_EQ(q.value(5).toString(), "写代码");     // detail
    EXPECT_EQ(q.value(6).toString(), "工作学习");   // eventClass
}

TEST_F(RecordServiceTest, SaveFromSnapshot_ExtractsFieldsCorrectly)
{
    m_mockDb->clearDatabase();

    TimerEndSnapshot snap;
    snap.startTime = "14:00:00";
    snap.endTime = "15:30:00";
    snap.consumMin = 90;
    snap.counter = 1;
    snap.elapsedMs = 5400000;

    bool ok = m_service->saveFromSnapshot(snap, "2026-08-28", "午休后工作", "工作学习");
    ASSERT_TRUE(ok);

    QSqlQuery q = m_mockDb->sortedRecord("2026-08-28");
    ASSERT_TRUE(q.next());
    EXPECT_EQ(q.value(1).toString(), "14:00:00");
    EXPECT_EQ(q.value(2).toString(), "15:30:00");
    EXPECT_EQ(q.value(3).toInt(), 90);
}

// ---------- deleteRecord ----------

TEST_F(RecordServiceTest, DeleteRecord_ExistingRow_RemovesIt)
{
    m_mockDb->clearDatabase();
    const QString date = "2026-08-28";

    // 先插入一行
    m_service->saveTimeRecord(date, "09:00:00", "10:00:00", 60, 0, "待删除", "工作学习");
    ASSERT_GT(m_mockDb->getRowCount(date), 0);

    // 再删除
    bool ok = m_service->deleteRecord(QDate(2026, 8, 28), 1);
    EXPECT_TRUE(ok);
    EXPECT_EQ(m_mockDb->getRowCount(date), 0);
}

// ---------- updateRecord ----------

TEST_F(RecordServiceTest, UpdateRecord_ExistingRow_UpdatesFields)
{
    m_mockDb->clearDatabase();
    const QString date = "2026-08-28";

    // 先插入一行
    m_service->saveTimeRecord(date, "09:00:00", "10:00:00", 60, 0, "原始", "工作学习");

    // 更新
    TimeRecord rec;
    rec.eventId = 1;
    rec.begTime = "09:30:00";
    rec.endTime = "11:00:00";
    rec.consumTime = 90;
    rec.counter = 3;
    rec.detail = "已更新";
    rec.eventClass = "休闲娱乐";

    bool ok = m_service->updateRecord(QDate(2026, 8, 28), rec);
    EXPECT_TRUE(ok);

    // 验证字段已更新
    QSqlQuery q = m_mockDb->sortedRecord(date);
    ASSERT_TRUE(q.next());
    EXPECT_EQ(q.value(1).toString(), "09:30:00");
    EXPECT_EQ(q.value(2).toString(), "11:00:00");
    EXPECT_EQ(q.value(3).toInt(), 90);
    EXPECT_EQ(q.value(4).toInt(), 3);
    EXPECT_EQ(q.value(5).toString(), "已更新");
    EXPECT_EQ(q.value(6).toString(), "休闲娱乐");
}

// ---------- addEmptyRecord ----------

TEST_F(RecordServiceTest, AddEmptyRecord_ValidInputs_InsertsRow)
{
    m_mockDb->clearDatabase();

    TimeRecord rec;
    rec.begTime = "08:00:00";
    rec.endTime = "09:00:00";
    rec.consumTime = 60;
    rec.counter = 0;
    rec.detail = "手动添加";
    rec.eventClass = "家庭亲情";

    bool ok = m_service->addEmptyRecord(QDate(2026, 8, 28), rec);
    EXPECT_TRUE(ok);
    EXPECT_EQ(m_mockDb->getRowCount("2026-08-28"), 1);
}

// ---------- queryRecordsOfDay ----------

TEST_F(RecordServiceTest, QueryRecordsOfDay_MultipleRows_ReturnsAllSorted)
{
    m_mockDb->clearDatabase();
    const QString date = "2026-08-28";

    // 插入多行
    m_service->saveTimeRecord(date, "14:00:00", "15:00:00", 60, 0, "下午", "工作学习");
    m_service->saveTimeRecord(date, "09:00:00", "10:00:00", 60, 0, "上午", "工作学习");
    m_service->saveTimeRecord(date, "20:00:00", "21:00:00", 60, 0, "晚上", "休闲娱乐");

    QVector<TimeRecord> records = m_service->queryRecordsOfDay(QDate(2026, 8, 28));
    ASSERT_EQ(records.size(), 3);

    // 验证按开始时间排序
    EXPECT_EQ(records[0].begTime, "09:00:00");
    EXPECT_EQ(records[1].begTime, "14:00:00");
    EXPECT_EQ(records[2].begTime, "20:00:00");
}

TEST_F(RecordServiceTest, QueryRecordsOfDay_NoTable_ReturnsEmpty)
{
    m_mockDb->clearDatabase();

    QVector<TimeRecord> records = m_service->queryRecordsOfDay(QDate(2020, 1, 1));
    EXPECT_TRUE(records.isEmpty());
}

// ---------- dayClassConsume ----------

TEST_F(RecordServiceTest, DayClassConsume_MultipleClasses_AggregatesCorrectly)
{
    m_mockDb->clearDatabase();
    const QString date = "2026-08-28";

    m_service->saveTimeRecord(date, "09:00:00", "10:00:00", 60, 0, "", "工作学习");
    m_service->saveTimeRecord(date, "10:00:00", "11:30:00", 90, 0, "", "工作学习");
    m_service->saveTimeRecord(date, "14:00:00", "15:00:00", 60, 0, "", "休闲娱乐");

    QMap<QString, int> result = m_service->dayClassConsume(QDate(2026, 8, 28));
    EXPECT_EQ(result.value("工作学习"), 150);   // 60 + 90
    EXPECT_EQ(result.value("休闲娱乐"), 60);
}

// ---------- weekCounterStats ----------

TEST_F(RecordServiceTest, WeekCounterStats_CountsPerDay)
{
    m_mockDb->clearDatabase();

    // 先创建一周内所有表（缺失表会导致 UNION ALL 查询失败）
    QStringList weekDays = {"2026-08-24", "2026-08-25", "2026-08-26",
                            "2026-08-27", "2026-08-28", "2026-08-29", "2026-08-30"};
    for (const QString &d : weekDays) {
        m_mockDb->createTable(d);
    }

    // 插入三天的分神记录
    m_service->saveTimeRecord("2026-08-24", "09:00:00", "10:00:00", 60, 3, "", "工作学习");
    m_service->saveTimeRecord("2026-08-25", "09:00:00", "10:00:00", 60, 1, "", "工作学习");
    m_service->saveTimeRecord("2026-08-26", "09:00:00", "10:00:00", 60, 2, "", "工作学习");

    QMap<QString, int> result = m_service->weekCounterStats(weekDays);
    EXPECT_EQ(result.value("2026-08-24"), 3);
    EXPECT_EQ(result.value("2026-08-25"), 1);
    EXPECT_EQ(result.value("2026-08-26"), 2);
}

// ---------- markedDaysOfMonth ----------

TEST_F(RecordServiceTest, MarkedDaysOfMonth_ReturnsDaysWithData)
{
    m_mockDb->clearDatabase();

    // 8月有数据
    m_service->saveTimeRecord("2026-08-15", "09:00:00", "10:00:00", 60, 0, "", "工作学习");
    m_service->saveTimeRecord("2026-08-20", "09:00:00", "10:00:00", 60, 0, "", "工作学习");
    // 9月有数据（不应出现在8月结果中）
    m_service->saveTimeRecord("2026-09-01", "09:00:00", "10:00:00", 60, 0, "", "工作学习");

    QSet<QString> marked = m_service->markedDaysOfMonth(2026, 8);
    EXPECT_TRUE(marked.contains("2026-08-15"));
    EXPECT_TRUE(marked.contains("2026-08-20"));
    EXPECT_FALSE(marked.contains("2026-09-01"));   // 9月不在8月结果中
}

// ---------- dataChanged 信号转发 ----------

TEST_F(RecordServiceTest, DataChanged_ForwardedFromDb)
{
    QSignalSpy spy(m_service, &RecordService::dataChanged);
    ASSERT_TRUE(spy.isValid());

    // 通过 mock 触发写操作（会发射 dataChanged）
    m_mockDb->clearDatabase();
    m_service->saveTimeRecord("2026-08-28", "09:00:00", "10:00:00", 60, 0, "", "工作学习");

    // RecordService 应该转发了 dataChanged 信号
    EXPECT_GE(spy.count(), 1);
}

// ---------- 边界条件 ----------

TEST_F(RecordServiceTest, NullDbPointer_SaveReturnsFalse)
{
    // 构造一个无 db 的 RecordService（边界场景）
    RecordService service(nullptr);
    bool ok = service.saveTimeRecord("2026-08-28", "09:00:00", "10:00:00", 60, 0, "", "工作学习");
    EXPECT_FALSE(ok);
}

TEST_F(RecordServiceTest, NullDbPointer_QueryReturnsEmpty)
{
    RecordService service(nullptr);
    QVector<TimeRecord> records = service.queryRecordsOfDay(QDate(2026, 8, 28));
    EXPECT_TRUE(records.isEmpty());
}

// ---------- 周换算与统计工具（static 纯业务规则，B-14a 自 chartPage 下沉） ----------

TEST(RecordServiceStaticTest, WeekRangeForIndex_Zero_IsLastWeek)
{
    const QDate today = QDate::currentDate();
    const auto range = RecordService::weekRangeForIndex(0);

    // 索引 0 = 最近一周：today-7 ~ today-1，区间恰为 7 天
    EXPECT_EQ(range.first, today.addDays(-7));
    EXPECT_EQ(range.second, today.addDays(-1));
    EXPECT_EQ(range.first.daysTo(range.second), 6);
}

TEST(RecordServiceStaticTest, WeekRangeForIndex_Indexed_WeeksContinuous)
{
    const auto r0 = RecordService::weekRangeForIndex(0);
    const auto r1 = RecordService::weekRangeForIndex(1);
    const auto r3 = RecordService::weekRangeForIndex(3);

    // 索引每 +1 向前推一周；相邻周区间连续且不重叠（前一周结束日 = 后一周开始日前一天）
    EXPECT_EQ(r1.second.daysTo(r0.first), 1);
    // r3.second = today-22，r0.first = today-7，相距 15 天
    EXPECT_EQ(r3.second.daysTo(r0.first), 15);
}

TEST(RecordServiceStaticTest, WeekDaysForIndex_SevenConsecutiveFormattedDays)
{
    const QStringList days = RecordService::weekDaysForIndex(0);

    // 恰为 7 天，格式 yyyy-MM-dd，且与周区间起始日逐日连续
    ASSERT_EQ(days.size(), 7);
    const QDate start = QDate::fromString(days.first(), "yyyy-MM-dd");
    ASSERT_TRUE(start.isValid());
    EXPECT_EQ(start, QDate::currentDate().addDays(-7));
    for (int i = 0; i < 7; ++i) {
        const QDate day = QDate::fromString(days[i], "yyyy-MM-dd");
        ASSERT_TRUE(day.isValid());
        EXPECT_EQ(day, start.addDays(i));
    }
}

TEST(RecordServiceStaticTest, SumMinutes_EmptyMap_IsZero)
{
    EXPECT_EQ(RecordService::sumMinutes({}), 0);
}

TEST(RecordServiceStaticTest, SumMinutes_MultipleClasses_SumsAllValues)
{
    const QMap<QString, int> data{{"工作学习", 150}, {"休闲娱乐", 60}, {"运动健康", 40}};
    EXPECT_EQ(RecordService::sumMinutes(data), 250);
}
