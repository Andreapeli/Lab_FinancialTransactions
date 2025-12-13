//
// Created by Andrea Peli on 09/11/25.
//
#ifndef TRANSAZIONE_H
#define TRANSAZIONE_H

#include <string>
#include <chrono>
#include <format>
#include <utility>
#include<algorithm>

using Clock = std::chrono::system_clock;
using TimePoint = Clock::time_point;

class Transaction
{
protected:
    std::string id;
    TimePoint data;
    double amount;
    std::string description;
    std::string category;
    std::string OperationType;
    std::string SenderAccount;
    std::string ReceiverAccount;


public:
    Transaction(std::string idGen, TimePoint d, double i, std::string desc, std::string cat,
                std::string Optype, std::string sendAcc, std::string recAcc)
        : id(std::move(idGen)), data(d), amount(i), description(std::move(desc)), category(std::move(cat)),
          OperationType(std::move(Optype)), SenderAccount(std::move(sendAcc)), ReceiverAccount(std::move(recAcc)) {}

    virtual ~Transaction() = default;

    const std::string getId() const{
        return id;
    }
    const std::string getSenderAccount() const{
        return SenderAccount;
    }
    const std::string getReceiverAccount() const{
        return ReceiverAccount;
    }
    const std::string getCategory() const{
        return category;
    }
    const std::string getDescription() const{
        return description;
    }
    const std::string getOperationType() const{
        return OperationType;
    }
    double getAmount() const{
        return amount;
    }
    TimePoint getData() const{
        return data;
    }

    std::string getDataFormatted() const {
        return std::format("{:%Y-%m-%d %H:%M:%S}", data);
    }

    virtual std::string getType() const = 0;
    virtual double getValue() const = 0;
    virtual std::string toCSV() const {
        return std::format("{},{},{},{},{},{},{},{},{}", id, getDataFormatted(), amount,
                           description, category, OperationType, getType(), SenderAccount, ReceiverAccount);
    }
};



#endif
