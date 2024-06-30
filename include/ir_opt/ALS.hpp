#pragma once

#include <ir/ir.hpp>
#include <unordered_map>

struct ALexp
{
    int Num = 0;
    std::unordered_map<Value *, int> exps;

    void print(Value *val)
    {
        val->print_ID();
        printf(" = ");
        for (auto it : exps)
        {
            it.first->print_ID();
            printf(" * %d, ", it.second);
        }
        putchar('\n');
    }

    ALexp(const ALexp &b)
        : Num(b.Num), exps(b.exps)
    {
    }

    ALexp(Value *val)
    {
        exps.insert({val, 1});
    }

    ALexp(int val)
        : Num(val)
    {
    }

    ALexp operator+(const ALexp &b) const
    {
        ALexp re(*this);
        re.Num += b.Num;
        for (auto &it : b.exps)
        {
            auto val = re.exps.find(it.first);
            if (val == re.exps.end())
                re.exps.insert({it.first, it.second});
            else
            {
                re.exps[val->first] += it.second;
                if (re.exps[val->first] == 0)
                    re.exps.erase(val);
            }
        }
        return re;
    }

    ALexp operator+(const int &num) const
    {
        ALexp re(*this);
        re.Num += num;
        return re;
    }

    ALexp operator-(const ALexp &b) const
    {
        ALexp re(*this);
        re.Num -= b.Num;
        for (auto &it : b.exps)
        {
            auto val = re.exps.find(it.first);
            if (val == re.exps.end())
                re.exps.insert({it.first, -it.second});
            else
            {
                re.exps[val->first] -= it.second;
                if (re.exps[val->first] == 0)
                    re.exps.erase(val);
            }
        }
        return re;
    }

    ALexp operator-(const int &num) const
    {
        ALexp re(*this);
        re.Num -= num;
        return re;
    }

    ALexp operator*(const int &b) const
    {
        ALexp re(*this);
        re.Num *= b;
        for (auto &it : re.exps)
            it.second *= b;
        return re;
    }

    bool checkMod(const int &b)
    {
        bool flag = true;
        for (auto &it : exps)
        {
            if (it.second % b)
            {
                flag = false;
                break;
            }
        }
        return Num % b ? false : flag;
    }

    ALexp operator/(const int &b) const
    {
        ALexp re(*this);
        re.Num /= b;
        for (auto &it : re.exps)
            it.second /= b;
        return re;
    }

    void operator=(const ALexp &b)
    {
        Num = b.Num;
        exps.clear();
        for (auto &it : b.exps)
            exps.insert({it.first, it.second});
    }
};

class ALS
{
    Module *p_module;
    std::unordered_map<Value *, ALexp *> map;

public:
    void FuncDealer(Function *func);
    void PassRun(Module *_module);
};
