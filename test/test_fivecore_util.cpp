//
// Created by wufan on 2026/9/20.
//

#include <gtest/gtest.h>
#include "service/backup_service.h"


TEST(test_fivecore_util,integration)
{
    BackupService bs;
    bs.doBackup();
}

