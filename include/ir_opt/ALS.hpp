#pragma once

#include <unordered_map>
#include <functional>

#include "../../include/ir/ir.hpp"
#include "../../include/ir_opt/DomTree.hpp"

struct ALexp
{
    int Num = 0;
    std::unordered_map<Value *, int> exps;

    bool operator==(const ALexp &b) const
    {
        if (Num != b.Num || exps.size() != b.exps.size())
            return false;
        for (auto it : b.exps)
        {
            auto val = exps.find(it.first);
            if (val == exps.end())
                return false;
            if (it.second != val->second)
                return false;
        }
        return true;
    }

    void print(Value *val) const
    {
        val->print_ID();
        printf(" = %d + ", Num);
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

struct ALexpHash
{
    std::size_t operator()(const ALexp &key) const
    {
        size_t re = std::hash<int>()(key.Num);
        for (auto it : key.exps)
            re = re ^ std::hash<Value *>()(it.first) ^ std::hash<int>()(it.second);
        return re;
    }
};

struct Vs
{
    static DomTree *domtree;
    std::vector<Value *> vals;

    bool replaceVal(Instrution *nwVal)
    {
        for (auto val : vals)
        {
            if (is_a<Param>(val))
            {
                auto Users = nwVal->get_user_list();
                for (auto it : *Users)
                    it->set_val(val);
                Users->clear();
                return true;
            }
            else if (is_a<Instrution>(val))
            {
                BasicBlock *curBB = ((Instrution *)val)->get_parent();
                assert(domtree != nullptr);
                if (domtree->is_dom(curBB, nwVal->get_parent()) || curBB == nwVal->get_parent())
                {
                    auto Users = nwVal->get_user_list();
                    for (auto it : *Users)
                        it->set_val(val);
                    Users->clear();
                    return true;
                }
            }
            else
                assert(0);
        }
        return false;
    }
};

class ALS
{
    Module *p_module;
    std::unordered_map<Value *, ALexp *> map;

public:
    void FuncCSS(Function *func);
    void FuncDealer(Function *func);
    void PassRun(Module *_module);
};
