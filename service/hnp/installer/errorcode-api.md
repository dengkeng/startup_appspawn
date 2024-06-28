# Native包BMS接口错误码

> **说明：**
>
> 以下仅介绍Native包BMS安装卸载接口特有错误码。

## 0x2001 参数非法

**错误信息**

params is invalid.

**错误描述**

参数非法

**可能原因**

传入非法的参数就会报这个错误。

**处理步骤**

检查传入的参数是否正确


## 0x2002 fork子进程失败

**错误信息**

fork unsuccess.

**错误描述**

程序fork子进程失败。

**可能原因**

系统性能问题。

**处理步骤**

重试请求接口。

## 0x2003 等待子进程退出失败

**错误信息**

wait pid unsuccess.

**错误描述**

程序等待子进程退出失败。

**可能原因**

系统性能问题。

**处理步骤**

重试请求接口。

## 0x2004 非开发者模式

**错误信息**

native package install/uninstall not in developer mode。

**错误描述**

当前为非开发者模式请求接口。

**可能原因**

当前为非开发者模式

**处理步骤**

打开开发者模式进行接口请求

## 0x2005 创建管道失败

**错误信息**

pipe fd unsuccess.

**错误描述**

创建pipe管道失败

**可能原因**

系统性能问题。

**处理步骤**

重试请求接口。

## 0x2006 读取管道失败

**错误信息**

read stream unsuccess.

**错误描述**

读取管道内容失败。

**可能原因**

系统禁止管道输出

**处理步骤**

查看权限

## 0x2007 获取返回码失败

**错误信息**

get return unsuccess.

**错误描述**

获取返回码失败。

**可能原因**

输出影响

**处理步骤**

检查日志，确定内容是否正确。

## 0x2008 移动内存失败

**错误信息**

mem move unsuccess.

**错误描述**

内存移动失败会返回这个错误码。

**可能原因**

系统内部原因

**处理步骤**

再次请求接口。