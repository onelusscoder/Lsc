//
// Created by lsc on 20-05-18.
//
//
//#ifndef LSC_APP_H
//#define LSC_APP_H
#pragma once
#include <Lsc/Rem/Rem.h>
#include <Lsc/Rem/Object.h>
#include <Lsc/TerminalAppApi/Kernel/Terminal.h>



namespace Lsc
{

class Terminal
{
    
    TAA::Terminal* _T = nullptr;
public:
    
    Terminal() = default;
    ~Terminal();
    
    Return operator()();
};
}
//#endif //LSC_APP_H