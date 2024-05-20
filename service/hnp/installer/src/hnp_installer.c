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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>

#include "hnp_installer.h"

#ifdef __cplusplus
extern "C" {
#endif

static int HnpInstallerUidGet(const char *uidIn, int *uidOut)
{
    int index;

    for (index = 0; uidIn[index] != '\0'; index++) {
        if (!isdigit(uidIn[index])) {
            return HNP_ERRNO_INSTALLER_ARGV_UID_INVALID;
        }
    }

    *uidOut = atoi(uidIn); // 转化为10进制
    return 0;
}

static int HnpGenerateSoftLinkAllByJson(const char *installPath, const char *dstPath, HnpCfgInfo *hnpCfg)
{
    char srcFile[MAX_FILE_PATH_LEN];
    char dstFile[MAX_FILE_PATH_LEN];
    NativeBinLink *currentLink = hnpCfg->links;
    char *fileNameTmp;

    if (access(dstPath, F_OK) != 0) {
        int ret = mkdir(dstPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if ((ret != 0) && (errno != EEXIST)) {
            HNP_LOGE("mkdir [%s] unsuccess, ret=%d, errno:%d", dstPath, ret, errno);
            return HNP_ERRNO_BASE_MKDIR_PATH_FAILED;
        }
    }

    for (unsigned int i = 0; i < hnpCfg->linkNum; i++) {
        int ret = sprintf_s(srcFile, MAX_FILE_PATH_LEN, "%s/%s", installPath, currentLink->source);
        char *fileName;
        if (ret < 0) {
            HNP_LOGE("sprintf install bin src file unsuccess.");
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }
        /* 如果target为空则使用源二进制名称 */
        if (strcmp(currentLink->target, "") == 0) {
            fileNameTmp = currentLink->source;
        } else {
            fileNameTmp = currentLink->target;
        }
        fileName = strrchr(fileNameTmp, DIR_SPLIT_SYMBOL);
        if (fileName == NULL) {
            fileName = fileNameTmp;
        } else {
            fileName++;
        }
        ret = sprintf_s(dstFile, MAX_FILE_PATH_LEN, "%s/%s", dstPath, fileName);
        if (ret < 0) {
            HNP_LOGE("sprintf install bin dst file unsuccess.");
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }
        /* 生成软链接 */
        ret = HnpSymlink(srcFile, dstFile);
        if (ret != 0) {
            return ret;
        }

        currentLink++;
    }

    return 0;
}

static int HnpGenerateSoftLinkAll(const char *installPath, const char *dstPath)
{
    char srcPath[MAX_FILE_PATH_LEN];
    char srcFile[MAX_FILE_PATH_LEN];
    char dstFile[MAX_FILE_PATH_LEN];
    int ret;
    DIR *dir;
    struct dirent *entry;

    ret = sprintf_s(srcPath, MAX_FILE_PATH_LEN, "%s/bin", installPath);
    if (ret < 0) {
        HNP_LOGE("sprintf install bin path unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    if ((dir = opendir(srcPath)) == NULL) {
        HNP_LOGI("soft link bin file:%s not exist", srcPath);
        return 0;
    }

    if (access(dstPath, F_OK) != 0) {
        ret = mkdir(dstPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if ((ret != 0) && (errno != EEXIST)) {
            closedir(dir);
            HNP_LOGE("mkdir [%s] unsuccess, ret=%d, errno:%d", dstPath, ret, errno);
            return HNP_ERRNO_BASE_MKDIR_PATH_FAILED;
        }
    }

    while (((entry = readdir(dir)) != NULL)) {
        /* 非二进制文件跳过 */
        if (entry->d_type != DT_REG) {
            continue;
        }
        ret = sprintf_s(srcFile, MAX_FILE_PATH_LEN, "%s/%s", srcPath, entry->d_name);
        if (ret < 0) {
            closedir(dir);
            HNP_LOGE("sprintf install bin src file unsuccess.");
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }

        ret = sprintf_s(dstFile, MAX_FILE_PATH_LEN, "%s/%s", dstPath, entry->d_name);
        if (ret < 0) {
            closedir(dir);
            HNP_LOGE("sprintf install bin dst file unsuccess.");
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }
        /* 生成软链接 */
        ret = HnpSymlink(srcFile, dstFile);
        if (ret != 0) {
            closedir(dir);
            return ret;
        }
    }

    closedir(dir);
    return 0;
}

static int HnpGenerateSoftLink(const char *installPath, const char *hnpBasePath, HnpCfgInfo *hnpCfg)
{
    int ret = 0;
    char binPath[MAX_FILE_PATH_LEN];

    ret = sprintf_s(binPath, MAX_FILE_PATH_LEN, "%s/bin", hnpBasePath);
    if (ret < 0) {
        HNP_LOGE("sprintf install bin path unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    if (hnpCfg->linkNum == 0) {
        ret = HnpGenerateSoftLinkAll(installPath, binPath);
    } else {
        ret = HnpGenerateSoftLinkAllByJson(installPath, binPath, hnpCfg);
    }

    return ret;
}

static int HnpInstall(const char *hnpFile, HnpInstallInfo *hnpInfo, HnpCfgInfo *hnpCfg)
{
    int ret;

    /* 解压hnp文件 */
    ret = HnpUnZip(hnpFile, hnpInfo->hnpVersionPath);
    if (ret != 0) {
        return ret; /* 内部已打印日志 */
    }

    /* 生成软链 */
    return HnpGenerateSoftLink(hnpInfo->hnpVersionPath, hnpInfo->hnpBasePath, hnpCfg);
}

static int HnpUnInstallPublicHnp(const char* packageName, const char *name, const char *version, int uid)
{
    int ret;
    char hnpNamePath[MAX_FILE_PATH_LEN];
    char sandboxPath[MAX_FILE_PATH_LEN];

    if (sprintf_s(hnpNamePath, MAX_FILE_PATH_LEN, HNP_DEFAULT_INSTALL_ROOT_PATH"/%d/hnppublic/%s.org", uid, name) < 0) {
        HNP_LOGE("hnp uninstall name path sprintf unsuccess,uid:%d,name:%s", uid, name);
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    if (sprintf_s(sandboxPath, MAX_FILE_PATH_LEN, HNP_SANDBOX_BASE_PATH"/%s.org", name) < 0) {
        HNP_LOGE("sprintf unstall base path unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    ret = HnpProcessRunCheck(sandboxPath);
    if (ret != 0) {
        return ret;
    }

    ret = HnpPackageInfoHnpDelete(packageName, name, version);
    if (ret != 0) {
        return ret;
    }

    return HnpDeleteFolder(hnpNamePath);
}

static int HnpUnInstall(int uid, const char *packageName)
{
    HnpPackageInfo *packageInfo = NULL;
    int count = 0;
    char privatePath[MAX_FILE_PATH_LEN];
    char dstPath[MAX_FILE_PATH_LEN];

    /* 拼接卸载路径 */
    if (sprintf_s(dstPath, MAX_FILE_PATH_LEN, HNP_DEFAULT_INSTALL_ROOT_PATH"/%d", uid) < 0) {
        HNP_LOGE("hnp install sprintf unsuccess, uid:%d", uid);
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    /* 验证卸载路径是否存在 */
    if (access(dstPath, F_OK) != 0) {
        HNP_LOGE("hnp uninstall uid path[%s] is not exist", dstPath);
        return HNP_ERRNO_UNINSTALLER_HNP_PATH_NOT_EXIST;
    }

    int ret = HnpPackageInfoGet(packageName, &packageInfo, &count);
    if (ret != 0) {
        return ret;
    }

    /* 卸载公有native */
    for (int i = 0; i < count; i++) {
        HNP_LOGI("hnp uninstall start now! name=%s,version=%s,uid=%d,package=%s", packageInfo[i].name,
            packageInfo[i].version, uid, packageName);
        ret = HnpUnInstallPublicHnp(packageName, packageInfo[i].name, packageInfo[i].version, uid);
        HNP_LOGI("hnp uninstall end! ret=%d", ret);
        if (ret != 0) {
            free(packageInfo);
            return ret;
        }
    }
    free(packageInfo);

    ret = HnpPackageInfoDelete(packageName);
    if (ret != 0) {
        return ret;
    }

    if (sprintf_s(privatePath, MAX_FILE_PATH_LEN, HNP_DEFAULT_INSTALL_ROOT_PATH"/%d/hnp/%s", uid, packageName) < 0) {
        HNP_LOGE("hnp uninstall private path sprintf unsuccess, uid:%d,package name[%s]", uid, packageName);
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    (void)HnpDeleteFolder(privatePath);

    return 0;
}

static int HnpInstallForceCheck(HnpCfgInfo *hnpCfgInfo, HnpInstallInfo *hnpInfo)
{
    int ret = 0;
    char *version = NULL;

    /* 判断安装目录是否存在，存在判断是否是强制安装，如果是则走卸载流程，否则返回错误 */
    if (access(hnpInfo->hnpSoftwarePath, F_OK) == 0) {
        if (hnpInfo->isForce == false) {
            HNP_LOGE("hnp install path[%s] exist, but force is false", hnpInfo->hnpSoftwarePath);
            return HNP_ERRNO_INSTALLER_PATH_IS_EXIST;
        }
        if (hnpInfo->isPublic) {
            version = HnpPackgeHnpVersionGet(hnpInfo->hapPackageName, hnpCfgInfo->name);
            if (version != NULL) {
                ret = HnpUnInstallPublicHnp(hnpInfo->hapPackageName, hnpCfgInfo->name, version, hnpInfo->uid);
            }
        } else {
            ret = HnpDeleteFolder(hnpInfo->hnpSoftwarePath);
        }
        if (ret != 0) {
            return ret;
        }
    }

    ret = HnpCreateFolder(hnpInfo->hnpVersionPath);
    if (ret != 0) {
        return HnpDeleteFolder(hnpInfo->hnpVersionPath);
    }
    return ret;
}

static int HnpInstallPathGet(HnpCfgInfo *hnpCfgInfo, HnpInstallInfo *hnpInfo)
{
    int ret;

    /* 拼接安装路径 */
    ret = sprintf_s(hnpInfo->hnpSoftwarePath, MAX_FILE_PATH_LEN, "%s/%s.org", hnpInfo->hnpBasePath,
        hnpCfgInfo->name);
    if (ret < 0) {
        HNP_LOGE("hnp install sprintf hnp base path unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    /* 拼接安装路径 */
    ret = sprintf_s(hnpInfo->hnpVersionPath, MAX_FILE_PATH_LEN, "%s/%s_%s", hnpInfo->hnpSoftwarePath,
        hnpCfgInfo->name, hnpCfgInfo->version);
    if (ret < 0) {
        HNP_LOGE("hnp install sprintf install path unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    return 0;
}

static int HnpReadAndInstall(char *srcFile, HnpInstallInfo *hnpInfo)
{
    int ret;
    HnpCfgInfo hnpCfg = {0};

    /* 从hnp zip获取cfg信息 */
    ret = HnpCfgGetFromZip(srcFile, &hnpCfg);
    if (ret != 0) {
        return ret;
    }

    ret = HnpInstallPathGet(&hnpCfg, hnpInfo);
    if (ret != 0) {
        // 释放软链接占用的内存
        if (hnpCfg.links != NULL) {
            free(hnpCfg.links);
        }
        return ret;
    }

    /* 存在对应版本的公有hnp包跳过安装 */
    if (access(hnpInfo->hnpVersionPath, F_OK) == 0 && hnpInfo->isPublic) {
        // 释放软链接占用的内存
        if (hnpCfg.links != NULL) {
            free(hnpCfg.links);
        }
        return HnpInstallInfoJsonWrite(hnpInfo->hapPackageName, &hnpCfg);
    }

    ret = HnpInstallForceCheck(&hnpCfg, hnpInfo);
    if (ret != 0) {
        // 释放软链接占用的内存
        if (hnpCfg.links != NULL) {
            free(hnpCfg.links);
        }
        return ret;
    }

    /* hnp安装 */
    ret = HnpInstall(srcFile, hnpInfo, &hnpCfg);
    // 释放软链接占用的内存
    if (hnpCfg.links != NULL) {
        free(hnpCfg.links);
    }
    if (ret != 0) {
        HnpUnInstallPublicHnp(hnpInfo->hapPackageName, hnpCfg.name, hnpCfg.version, hnpInfo->uid);
        return ret;
    }
    if (hnpInfo->isPublic) {
        ret = HnpInstallInfoJsonWrite(hnpInfo->hapPackageName, &hnpCfg);
        if (ret != 0) {
            HnpUnInstallPublicHnp(hnpInfo->hapPackageName, hnpCfg.name, hnpCfg.version, hnpInfo->uid);
        }
    }

    return ret;
}

static bool HnpFileCheck(const char *file)
{
    const char suffix[] = ".hnp";
    int len = strlen(file);
    int suffixLen = strlen(suffix);
    if ((len >= suffixLen) && (strcmp(file + len - suffixLen, suffix) == 0)) {
        return true;
    }

    return false;
}

static int HnpPackageGetAndInstall(const char *dirPath, HnpInstallInfo *hnpInfo)
{
    DIR *dir;
    struct dirent *entry;
    char path[MAX_FILE_PATH_LEN];
    int ret;

    if ((dir = opendir(dirPath)) == NULL) {
        HNP_LOGE("hnp install opendir:%s unsuccess, errno=%d", dirPath, errno);
        return HNP_ERRNO_BASE_DIR_OPEN_FAILED;
    }

    while ((entry = readdir(dir)) != NULL) {
        if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)) {
            continue;
        }
        if (sprintf_s(path, MAX_FILE_PATH_LEN, "%s/%s", dirPath, entry->d_name) < 0) {
            HNP_LOGE("hnp install sprintf unsuccess, dir[%s], path[%s]", dirPath, entry->d_name);
            closedir(dir);
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }

        if (entry->d_type == DT_DIR) {
            ret = HnpPackageGetAndInstall(path, hnpInfo);
            if (ret != 0) {
                closedir(dir);
                return ret;
            }
        } else {
            if (HnpFileCheck(path) == false) {
                continue;
            }
            HNP_LOGI("hnp install start now! src file=%s, dst path=%s", path, hnpInfo->hnpBasePath);
            ret = HnpReadAndInstall(path, hnpInfo);
            HNP_LOGI("hnp install end, ret=%d", ret);
            if (ret != 0) {
                closedir(dir);
                return ret;
            }
        }
    }
    closedir(dir);
    return 0;
}

static int HnpInsatllPre(HapInstallInfo *installInfo)
{
    char dstPath[MAX_FILE_PATH_LEN];
    HnpInstallInfo hnpInfo = {installInfo->uid, installInfo->hapPackageName, installInfo->hapPath, installInfo->abi,
        installInfo->isForce, false, {0}, {0}, {0}};
    struct dirent *entry;
    char hnpPath[MAX_FILE_PATH_LEN];

    /* 拼接安装路径 */
    if (sprintf_s(dstPath, MAX_FILE_PATH_LEN, HNP_DEFAULT_INSTALL_ROOT_PATH"/%d", installInfo->uid) < 0) {
        HNP_LOGE("hnp install sprintf unsuccess, uid:%d", installInfo->uid);
        return HNP_ERRNO_INSTALLER_GET_HNP_PATH_FAILED;
    }

    /* 验证安装路径是否存在 */
    if (access(dstPath, F_OK) != 0) {
        HNP_LOGE("hnp install uid path[%s] is not exist", dstPath);
        return HNP_ERRNO_INSTALLER_GET_REALPATH_FAILED;
    }

    DIR *dir = opendir(installInfo->hnpRootPath);
    if (dir == NULL) {
        HNP_LOGE("hnp install opendir:%s unsuccess, errno=%d", installInfo->hnpRootPath, errno);
        return HNP_ERRNO_BASE_DIR_OPEN_FAILED;
    }

    /* 遍历src目录 */
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, "public") == 0) {
            hnpInfo.isPublic = true;
            if ((sprintf_s(hnpInfo.hnpBasePath, MAX_FILE_PATH_LEN, "%s/hnppublic", dstPath) < 0) ||
                (sprintf_s(hnpPath, MAX_FILE_PATH_LEN, "%s/public", installInfo->hnpRootPath) < 0)) {
                closedir(dir);
                HNP_LOGE("hnp install public base path sprintf unsuccess.");
                return HNP_ERRNO_BASE_SPRINTF_FAILED;
            }
        } else if (strcmp(entry->d_name, "private") == 0) {
            hnpInfo.isPublic = false;
            if ((sprintf_s(hnpInfo.hnpBasePath, MAX_FILE_PATH_LEN, "%s/hnp/%s", dstPath,
                installInfo->hapPackageName) < 0) || (sprintf_s(hnpPath, MAX_FILE_PATH_LEN, "%s/private",
                installInfo->hnpRootPath) < 0)) {
                closedir(dir);
                HNP_LOGE("hnp install private base path sprintf unsuccess.");
                return HNP_ERRNO_BASE_SPRINTF_FAILED;
            }
        } else {
            continue;
        }

        int ret = HnpPackageGetAndInstall(hnpPath, &hnpInfo);
        if (ret != 0) {
            closedir(dir);
            return ret;
        }
    }
    closedir(dir);
    return 0;
}

static int ParseInstallArgs(HapInstallArgv *installArgv, HapInstallInfo *installInfo)
{
    int ret;

    if (installArgv->uid == NULL) {
        HNP_LOGE("install argv uid is null.");
        return HNP_ERRNO_OPERATOR_ARGV_MISS;
    }
    ret = HnpInstallerUidGet(installArgv->uid, &installInfo->uid);
    if (ret != 0) {
        HNP_LOGE("hnp install argv uid[%s] invalid", installArgv->uid);
        return ret;
    }

    if (installArgv->hnpRootPath == NULL) {
        HNP_LOGE("install argv hnp root path is null.");
        return HNP_ERRNO_OPERATOR_ARGV_MISS;
    }
    if (GetRealPath(installArgv->hnpRootPath, installInfo->hnpRootPath) != 0) {
        HNP_LOGE("hnp install argv hnp root path=%s is invalid.", installArgv->hnpRootPath);
        return HNP_ERRNO_INSTALLER_GET_REALPATH_FAILED;
    }

    if (installArgv->hapPath == NULL) {
        HNP_LOGE("install argv hap path is null.");
        return HNP_ERRNO_OPERATOR_ARGV_MISS;
    }
    if (GetRealPath(installArgv->hapPath, installInfo->hapPath) != 0) {
        HNP_LOGE("hnp install argv hap path=%s is invalid.", installArgv->hapPath);
        return HNP_ERRNO_INSTALLER_GET_REALPATH_FAILED;
    }

    if (installArgv->abi == NULL) {
        HNP_LOGE("install argv abi path is null.");
        return HNP_ERRNO_OPERATOR_ARGV_MISS;
    }
    if (GetRealPath(installArgv->abi, installInfo->abi) != 0) {
        HNP_LOGE("hnp install argv abi path=%s is invalid.", installArgv->abi);
        return HNP_ERRNO_INSTALLER_GET_REALPATH_FAILED;
    }

    if (installArgv->hapPackageName == NULL) {
        HNP_LOGE("install argv hap package name is null.");
    }
    if (strcpy_s(installInfo->hapPackageName, MAX_FILE_PATH_LEN, installArgv->hapPackageName) != EOK) {
        HNP_LOGE("strcpy name argv unsuccess.");
        return HNP_ERRNO_BASE_COPY_FAILED;
    }

    return 0;
}

int HnpCmdInstall(int argc, char *argv[])
{
    HapInstallArgv installArgv = {0};
    HapInstallInfo installInfo = {0};
    int ch;

    optind = 1; // 从头开始遍历参数
    while ((ch = getopt_long(argc, argv, "hu:p:i:s:a:f", NULL, NULL)) != -1) {
        switch (ch) {
            case 'h' :
                return HNP_ERRNO_OPERATOR_ARGV_MISS;
            case 'u': // 用户id
                installArgv.uid = optarg;
                break;
            case 'p': // app名称
                installArgv.hapPackageName = (char *)optarg;
                break;
            case 'i': // hnp安装目录
                installArgv.hnpRootPath = (char *)optarg;
                break;
            case 's': // hap目录
                installArgv.hapPath = (char *)optarg;
                break;
            case 'a': // 系统abi路径
                installArgv.abi = (char *)optarg;
                break;
            case 'f': // is force
                installInfo.isForce = true;
                break;
            default:
                break;
        }
    }

    // 解析参数并生成安装信息
    int ret = ParseInstallArgs(&installArgv, &installInfo);
    if (ret != 0) {
        return ret;
    }

    return HnpInsatllPre(&installInfo);
}

int HnpCmdUnInstall(int argc, char *argv[])
{
    int uid;
    char *uidArg = NULL;
    char *packageName = NULL;
    int ret;
    int ch;

    optind = 1; // 从头开始遍历参数
    while ((ch = getopt_long(argc, argv, "hu:p:", NULL, NULL)) != -1) {
        switch (ch) {
            case 'h' :
                return HNP_ERRNO_OPERATOR_ARGV_MISS;
            case 'u': // uid
                uidArg = optarg;
                ret = HnpInstallerUidGet(uidArg, &uid);
                if (ret != 0) {
                    HNP_LOGE("hnp install arg uid[%s] invalid", uidArg);
                    return ret;
                }
                break;
            case 'p': // hnp package name
                packageName = (char *)optarg;
                break;
            default:
                break;
            }
    }

    if ((uidArg == NULL) || (packageName == NULL)) {
        HNP_LOGE("hnp uninstall params invalid uid[%s], package name[%s]", uidArg, packageName);
        return HNP_ERRNO_OPERATOR_ARGV_MISS;
    }

    return HnpUnInstall(uid, packageName);
}

#ifdef __cplusplus
}
#endif