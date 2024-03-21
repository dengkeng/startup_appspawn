/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "hnp_base.h"
#include "hnp_pack.h"
#include "securec.h"

using namespace testing;
using namespace testing::ext;

#ifdef __cplusplus
    extern "C" {
#endif


#ifdef __cplusplus
    }
#endif

namespace OHOS {
class HnpPackTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HnpPackTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "Hnp_Pack_TEST SetUpTestCase";
}

void HnpPackTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "Hnp_Pack_TEST TearDownTestCase";
}

void HnpPackTest::SetUp()
{
    GTEST_LOG_(INFO) << "Hnp_Pack_TEST SetUp";
}

void HnpPackTest::TearDown()
{
    GTEST_LOG_(INFO) << "Hnp_Pack_TEST TearDown";
}


/**
* @tc.name: Hnp_Pack_001
* @tc.desc:  Verify set Arg if HnpCmdPack succeed.
* @tc.type: FUNC
* @tc.require:issueI98PSE
* @tc.author:
*/
HWTEST(HnpPackTest, Hnp_Pack_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Pack_001 start";

    // clear resource before test
    remove("./hnp_out/sample.hnp");
    rmdir("hnp_out");
    rmdir("hnp_sample");
    remove("hnp.cfg");

    EXPECT_EQ(mkdir("hnp_sample", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir("hnp_out", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    
    char arg1[] = "hnp";
    char arg2[] = "pack";

    { // param num not enough
        char* argv[] = {arg1, arg2};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdPack(argc, argv), HNP_ERRNO_PACK_ARGV_NUM_INVALID);
    }
    { // src dir path is invalid
        char arg3[] = "./hnp_sample2";
        char arg4[] = "./hnp_out";
        char* argv[] = {arg1, arg2, arg3, arg4};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdPack(argc, argv), HNP_ERRNO_PACK_GET_REALPATH_FAILED);
    }
    { // no name and version
        char arg3[] = "./hnp_sample";
        char arg4[] = "/hnp_out";
        char* argv[] = {arg1, arg2, arg3, arg4};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdPack(argc, argv), HNP_ERRNO_PACK_MISS_OPERATOR_PARAM);
    }
    { // ok
        char arg3[] = "./hnp_sample";
        char arg4[] = "/hnp_out";
        char arg5[] = "-name";
        char arg6[] = "sample";
        char arg7[] = "-v";
        char arg8[] = "1.1";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdPack(argc, argv), 0);
        EXPECT_EQ(remove("./hnp_out/sample.hnp"), 0);
    }

    EXPECT_EQ(rmdir("hnp_sample"), 0);
    EXPECT_EQ(rmdir("hnp_out"), 0);
    
    GTEST_LOG_(INFO) << "Hnp_Pack_001 end";
}

/**
* @tc.name: Hnp_Pack_002
* @tc.desc:  Verify set Arg with cfg if HnpCmdPack succeed.
* @tc.type: FUNC
* @tc.require:issueI98PSE
* @tc.author:
*/
HWTEST(HnpPackTest, Hnp_Pack_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Pack_002 start";

    EXPECT_EQ(mkdir("hnp_sample", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir("hnp_out", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    FILE *fp = fopen("hnp.cfg", "w");
    EXPECT_NE(fp, nullptr);
    fclose(fp);
    
    char arg1[] = "hnp";
    char arg2[] = "pack";
    char arg3[] = "./hnp_sample";
    char arg4[] = "/hnp_out";
    char arg5[] = "-cfg";

    { // cfg file path is invalid
        char arg6[] = "./hnp2.cfg";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdPack(argc, argv), HNP_ERRNO_PACK_GET_REALPATH_FAILED);
    }
    { // cfg file content is empty
        char arg6[] = "./hnp.cfg";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdPack(argc, argv), HNP_ERRNO_PACK_READ_FILE_STREAM_FAILED);
    }
    { // cfg file not json formaat
        char cfg[] = "this is for test!";
        FILE *fp = fopen("./hnp.cfg", "w");
        EXPECT_EQ(fwrite(cfg, sizeof(char), strlen(cfg) + 1, fp), strlen(cfg) + 1);
        fclose(fp);

        char arg6[] = "./hnp.cfg";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdPack(argc, argv), HNP_ERRNO_PACK_PARSE_JSON_FAILED);
    }

    EXPECT_EQ(rmdir("hnp_sample"), 0);
    EXPECT_EQ(rmdir("hnp_out"), 0);
    EXPECT_EQ(remove("hnp.cfg"), 0);

    GTEST_LOG_(INFO) << "Hnp_Pack_002 end";
}

/**
* @tc.name: Hnp_Pack_003
* @tc.desc:  Verify set cfg type item invalid if HnpCmdPack succeed.
* @tc.type: FUNC
* @tc.require:issueI98PSE
* @tc.author:
*/
HWTEST(HnpPackTest, Hnp_Pack_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Pack_003 start";

    EXPECT_EQ(mkdir("hnp_sample", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir("hnp_out", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    FILE *fp = fopen("hnp.cfg", "w");
    EXPECT_NE(fp, nullptr);
    fclose(fp);
    
    char arg1[] = "hnp";
    char arg2[] = "pack";
    char arg3[] = "./hnp_sample";
    char arg4[] = "/hnp_out";
    char arg5[] = "-cfg";
    char arg6[] = "./hnp.cfg";
    char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
    int argc = sizeof(argv) / sizeof(argv[0]);

    { // cfg file content has not type item
        char cfg[] = "{\"typ\":\"hnp-config\",\"name\":\"sample\",\"version\":\"1.1\",\"install\":"
            "{\"links\":[{\"source\":\"bin/out\",\"target\":\"out\"},{\"source\":\"bin/out2\","
            "\"target\":\"out2\"}]}}";
        FILE *fp = fopen("./hnp.cfg", "w");
        EXPECT_EQ(fwrite(cfg, sizeof(char), strlen(cfg) + 1, fp), strlen(cfg) + 1);
        fclose(fp);

        EXPECT_EQ(HnpCmdPack(argc, argv), HNP_ERRNO_PACK_PARSE_ITEM_NO_FOUND);
    }
    { // type item value is not expected
        char cfg[] = "{\"type\":\"hnpconfig\",\"name\":\"sample\",\"version\":\"1.1\",\"install\":"
            "{\"links\":[{\"source\":\"bin/out\",\"target\":\"out\"},{\"source\":\"bin/out2\","
            "\"target\":\"out2\"}]}}";
        FILE *fp = fopen("./hnp.cfg", "w");
        EXPECT_EQ(fwrite(cfg, sizeof(char), strlen(cfg) + 1, fp), strlen(cfg) + 1);
        fclose(fp);

        EXPECT_EQ(HnpCmdPack(argc, argv), HNP_ERRNO_PACK_PARSE_ITEM_NO_FOUND);
    }

    EXPECT_EQ(rmdir("hnp_sample"), 0);
    EXPECT_EQ(rmdir("hnp_out"), 0);
    EXPECT_EQ(remove("hnp.cfg"), 0);

    GTEST_LOG_(INFO) << "Hnp_Pack_003 end";
}

/**
* @tc.name: Hnp_Pack_004
* @tc.desc:  Verify set cfg invalid if HnpCmdPack succeed.
* @tc.type: FUNC
* @tc.require:issueI98PSE
* @tc.author:
*/
HWTEST(HnpPackTest, Hnp_Pack_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Pack_004 start";

    EXPECT_EQ(mkdir("hnp_sample", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir("hnp_out", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    FILE *fp = fopen("hnp.cfg", "w");
    EXPECT_NE(fp, nullptr);
    fclose(fp);
    
    char arg1[] = "hnp";
    char arg2[] = "pack";
    char arg3[] = "./hnp_sample";
    char arg4[] = "/hnp_out";
    char arg5[] = "-cfg";
    char arg6[] = "./hnp.cfg";
    char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
    int argc = sizeof(argv) / sizeof(argv[0]);
    
    { // cfg file content has not name item
        char cfg[] = "{\"type\":\"hnp-config\",\"version\":\"1.1\",\"install\":"
            "{\"links\":[{\"source\":\"bin/out\",\"target\":\"out\"},{\"source\":\"bin/out2\","
            "\"target\":\"out2\"}]}}";
        FILE *fp = fopen("./hnp.cfg", "w");
        EXPECT_EQ(fwrite(cfg, sizeof(char), strlen(cfg) + 1, fp), strlen(cfg) + 1);
        fclose(fp);

        EXPECT_EQ(HnpCmdPack(argc, argv), HNP_ERRNO_PACK_PARSE_ITEM_NO_FOUND);
    }
    { // cfg file content has not version item
        char cfg[] = "{\"type\":\"hnp-config\",\"name\":\"sample\",\"install\":"
            "{\"links\":[{\"source\":\"bin/out\",\"target\":\"out\"},{\"source\":\"bin/out2\","
            "\"target\":\"out2\"}]}}";
        FILE *fp = fopen("./hnp.cfg", "w");
        EXPECT_EQ(fwrite(cfg, sizeof(char), strlen(cfg) + 1, fp), strlen(cfg) + 1);
        fclose(fp);

        EXPECT_EQ(HnpCmdPack(argc, argv), HNP_ERRNO_PACK_PARSE_ITEM_NO_FOUND);
    }
    { // cfg file content has not install item
        char cfg[] = "{\"type\":\"hnp-config\",\"name\":\"sample\",\"version\":\"1.1\",\"uninstall\":"
            "{\"links\":[{\"source\":\"bin/out\",\"target\":\"out\"},{\"source\":\"bin/out2\","
            "\"target\":\"out2\"}]}}";
        FILE *fp = fopen("./hnp.cfg", "w");
        EXPECT_EQ(fwrite(cfg, sizeof(char), strlen(cfg) + 1, fp), strlen(cfg) + 1);
        fclose(fp);

        EXPECT_EQ(HnpCmdPack(argc, argv), HNP_ERRNO_PACK_PARSE_ITEM_NO_FOUND);
    }

    EXPECT_EQ(rmdir("hnp_sample"), 0);
    EXPECT_EQ(rmdir("hnp_out"), 0);
    EXPECT_EQ(remove("hnp.cfg"), 0);

    GTEST_LOG_(INFO) << "Hnp_Pack_004 end";
}

/**
* @tc.name: Hnp_Pack_005
* @tc.desc:  Verify set cfg links item invalid if HnpCmdPack succeed.
* @tc.type: FUNC
* @tc.require:issueI98PSE
* @tc.author:
*/
HWTEST(HnpPackTest, Hnp_Pack_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Pack_005 start";

    EXPECT_EQ(mkdir("hnp_sample", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir("hnp_out", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    FILE *fp = fopen("hnp.cfg", "w");
    EXPECT_NE(fp, nullptr);
    fclose(fp);
    
    char arg1[] = "hnp";
    char arg2[] = "pack";
    char arg3[] = "./hnp_sample";
    char arg4[] = "/hnp_out";
    char arg5[] = "-cfg";
    char arg6[] = "./hnp.cfg";
    char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
    int argc = sizeof(argv) / sizeof(argv[0]);

    { // link arry item has not source item
        char cfg[] = "{\"type\":\"hnp-config\",\"name\":\"sample\",\"version\":\"1.1\",\"install\":"
            "{\"links\":[{\"src\":\"bin/out\",\"target\":\"out\"},{\"source\":\"bin/out2\","
            "\"target\":\"out2\"}]}}";
        FILE *fp = fopen("./hnp.cfg", "w");
        EXPECT_EQ(fwrite(cfg, sizeof(char), strlen(cfg) + 1, fp), strlen(cfg) + 1);
        fclose(fp);

        EXPECT_EQ(HnpCmdPack(argc, argv), HNP_ERRNO_PACK_PARSE_ITEM_NO_FOUND);
    }
    { // ok, link arry item has not target item
        char cfg[] = "{\"type\":\"hnp-config\",\"name\":\"sample\",\"version\":\"1.1\",\"install\":"
            "{\"links\":[{\"source\":\"bin/out\",\"tar\":\"out\"},{\"source\":\"bin/out2\","
            "\"target\":\"out2\"}]}}";
        FILE *fp = fopen("./hnp.cfg", "w");
        EXPECT_EQ(fwrite(cfg, sizeof(char), strlen(cfg) + 1, fp), strlen(cfg) + 1);
        fclose(fp);

        EXPECT_EQ(HnpCmdPack(argc, argv), 0);
        EXPECT_EQ(remove("./hnp_out/sample.hnp"), 0);
    }

    EXPECT_EQ(rmdir("hnp_sample"), 0);
    EXPECT_EQ(rmdir("hnp_out"), 0);
    EXPECT_EQ(remove("hnp.cfg"), 0);

    GTEST_LOG_(INFO) << "Hnp_Pack_005 end";
}

/**
* @tc.name: Hnp_Pack_006
* @tc.desc:  Verify set cfg valid if HnpCmdPack succeed.
* @tc.type: FUNC
* @tc.require:issueI98PSE
* @tc.author:
*/
HWTEST(HnpPackTest, Hnp_Pack_006, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Pack_006 start";

    EXPECT_EQ(mkdir("hnp_sample", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir("hnp_out", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    FILE *fp = fopen("hnp.cfg", "w");
    EXPECT_NE(fp, nullptr);
    fclose(fp);
    
    char arg1[] = "hnp";
    char arg2[] = "pack";
    char arg3[] = "./hnp_sample";
    char arg4[] = "/hnp_out";
    char arg5[] = "-cfg";
    char arg6[] = "./hnp.cfg";
    char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
    int argc = sizeof(argv) / sizeof(argv[0]);

    { // ok. no links item in install item
        char cfg[] = "{\"type\":\"hnp-config\",\"name\":\"sample\",\"version\":\"1.1\",\"install\":{}}";
        FILE *fp = fopen("./hnp.cfg", "w");
        EXPECT_EQ(fwrite(cfg, sizeof(char), strlen(cfg) + 1, fp), strlen(cfg) + 1);
        fclose(fp);

        EXPECT_EQ(HnpCmdPack(argc, argv), 0);
        EXPECT_EQ(remove("./hnp_out/sample.hnp"), 0);
    }
    { // ok. no array in links item
        char cfg[] = "{\"type\":\"hnp-config\",\"name\":\"sample\",\"version\":\"1.1\",\"install\":"
            "{\"links\":[]}}";
        FILE *fp = fopen("./hnp.cfg", "w");
        EXPECT_EQ(fwrite(cfg, sizeof(char), strlen(cfg) + 1, fp), strlen(cfg) + 1);
        fclose(fp);

        EXPECT_EQ(HnpCmdPack(argc, argv), 0);
        EXPECT_EQ(remove("./hnp_out/sample.hnp"), 0);
    }
    { // ok. 2 links
        char cfg[] = "{\"type\":\"hnp-config\",\"name\":\"sample\",\"version\":\"1.1\",\"install\":"
            "{\"links\":[{\"source\":\"bin/out\",\"target\":\"out\"},{\"source\":\"bin/out2\","
            "\"target\":\"out2\"}]}}";
        FILE *fp = fopen("./hnp.cfg", "w");
        EXPECT_EQ(fwrite(cfg, sizeof(char), strlen(cfg) + 1, fp), strlen(cfg) + 1);
        fclose(fp);

        EXPECT_EQ(HnpCmdPack(argc, argv), 0);
        EXPECT_EQ(remove("./hnp_out/sample.hnp"), 0);
    }

    EXPECT_EQ(rmdir("hnp_sample"), 0);
    EXPECT_EQ(rmdir("hnp_out"), 0);
    EXPECT_EQ(remove("hnp.cfg"), 0);

    GTEST_LOG_(INFO) << "Hnp_Pack_006 end";
}


} // namespace OHOS