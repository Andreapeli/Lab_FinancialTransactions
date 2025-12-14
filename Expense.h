//
// Created by Andrea Peli on 15/11/25.
//

#ifndef FINANCIAL_TRANSACTIONS_EXPENSE_H
#define FINANCIAL_TRANSACTIONS_EXPENSE_H

#include <algorithm>
#include<utility>

#include "Transaction.h"

class Expense : public Transaction {
public:
    Expense(std::string idGen, TimePoint d, double i, std::string desc, std::string cat,
           std::string tipoOp, std::string SendAcc, std::string RecAcc)
        : Transaction(std::move(idGen), d,i, std::move(desc), std::move(cat), std::move(tipoOp), std::move(SendAcc),std::move( RecAcc)) {}

    std::string getType() const override {
        return "Expense";
    }
    double getValue() const override{
        return -amount;
    }
};


#endif //FINANCIAL_TRANSACTIONS_EXPENSE_H