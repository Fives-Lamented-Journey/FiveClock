//
// Created by wufan on 2026/9/20.
//

#include <gtest/gtest.h>
#include "fivecore/util/string_utils.h"
#include "fivecore/util/sha256_utils.h"

using namespace fivecore::util;

TEST(StringUtil, TrimTest)
{
    EXPECT_EQ(trim("   abc   "), "abc");
    EXPECT_EQ(trim("empty"), "empty");
    EXPECT_EQ(trim(""), "");
}

TEST(Sha256Util, StringStub)
{
    EXPECT_EQ(sha256_string("test"), "stub-sha256-placeholder");
}

TEST(Sha256Util, FileStub)
{
    EXPECT_EQ(sha256_file("test.db"), "stub-file-sha256-placeholder");
}