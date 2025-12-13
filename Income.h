//
// Created by Andrea Peli on 15/11/25.
//

#ifndef FINANCIAL_TRANSACTIONS_INCOME_H
#define FINANCIAL_TRANSACTIONS_INCOME_H
#include <utility>

#include "Transaction.h"

class Income : public Transaction {
public:
    Income(std::string idGen, TimePoint d, double i, std::string desc, std::string cat,
            std::string OpType, std::string SendAcc, std::string RecAccount)
        : Transaction(std::move(idGen), d, i, std::move(desc), std::move(cat), std::move(OpType), std::move(SendAcc), std::move(RecAccount)) {}

    std::string getType() const override{
        return "Income";
    }
    double getValue() const override{
        return amount;
    }
};



#endif //FINANCIAL_TRANSACTIONS_INCOME_H