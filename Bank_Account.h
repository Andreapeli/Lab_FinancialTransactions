//
// Created by Andrea Peli on 15/11/25.
//

#ifndef FINANCIAL_TRANSACTIONS_BANK_ACCOUNT_H
#define FINANCIAL_TRANSACTIONS_BANK_ACCOUNT_H



#include <vector>
#include <memory>
#include "Transaction.h"

class BankAccount {
private:
    std::string ownerId;
    std::string bankId;
    std::string password;
    std::vector<std::unique_ptr<Transaction>> transactions;

public:
    BankAccount(std::string owner, std::string bank, std::string pwd)
        : ownerId(owner), bankId(bank), password(pwd) {}

    const std::string getOwnerId() const{
        return ownerId;
    }
    const std::string getBankId() const{
        return bankId;
    }

    bool verifyPwd(const std::string& pwd) const{
        return pwd == password;
    }

    void addTransaction(std::unique_ptr<Transaction> t, const BankAccount* destinationAcc = nullptr);

    double balance() const;
    void requireAuth(const std::string& pwd) const;
    const Transaction* findTransactionById(const std::string& txId) const;
    std::vector<const Transaction*> filterByType(const std::string& opType) const;
    std::vector<const Transaction*> filterByCounterparty(const std::string& accountId) const;

    void printTransactionById(const std::string& pwd, const std::string& txId) const;
    void printTransactionsByType(const std::string& pwd, const std::string& opType) const;
    void printTransactionsByAccount(const std::string& pwd, const std::string& accountId) const;
    void printTransactions() const;

    void SaveToFile(const std::string& filename, const std::string& pwd) const;
    void ReadFromFile(const std::string& filename, const std::string& pwd);
};



#endif //FINANCIAL_TRANSACTIONS_BANK_ACCOUNT_H