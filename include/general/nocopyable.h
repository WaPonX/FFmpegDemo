/// \file nocopyable.h
/// \brife 提供禁止复制构造的类和宏定义
/// 参考google实现
/// \date 2019/4/16
/// \version 0.1.0
/// \author waponxie
/// \copyright
/// |修改人|修改日期|修改描述|
/// |:-----|:-------|:-------|
/// |waponxie|2019/4/16|创建文件|

#pragma once

#define DISALLOW_COPY(CLASS) \
    CLASS(const CLASS &);\
    void operator=(const CLASS &)

class Nocopyable
{
public:
    Nocopyable() {}

    ~Nocopyable() {}
private:
    DISALLOW_COPY(Nocopyable);
};