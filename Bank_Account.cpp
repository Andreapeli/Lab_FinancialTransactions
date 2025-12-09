//
// Created by Andrea Peli on 15/11/25.
#include <algorithm>
#include <ranges>
#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <array>
#include<stdio.h>
#include "Expense.h"
#include "Income.h"
#include "Bank_Account.h"

// --- Supporto data/ora

static TimePoint parseDateTime(const std::string& s) {
    std::tm tm{};
    std::istringstream iss(s);
    iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (iss.fail()) {
        throw std::runtime_error("Invalid datetime format: " + s);
    }
    // Converte ora locale in time_t
    std::time_t tt = std::mktime(&tm);
    if (tt == -1) {
        throw std::runtime_error("mktime failed for: " + s);
    }
    // Converte a time_point
    return Clock::from_time_t(tt);
}


void BankAccount::addTransaction(std::unique_ptr<Transaction> t,
                                 const BankAccount* destinationAccount) {
    // Regole per i trasferimenti:
    //deve esserci un destinatario
    if (t->getOperationType() == "Transfer") {
        if (!destinationAccount) {
            throw std::invalid_argument("Transfer requires a destination account");
        }
        //uno stesso account può effettuare trasferimenti su conti correnti differenti
        if (this->getOwnerId() != destinationAccount->getOwnerId() ||
            this->getBankId() == destinationAccount->getBankId()) {
            std::cerr << "Invalid transfer: accounts must belong to the same owner and have different bankId\n";
            throw std::runtime_error("Transfer rule violated");
        }
        /*if (t->getSenderAccount() == t->getReceiverAccount()) {
            std::cerr << "Invalid transfer: sender and receiver accounts must differ\n";
            throw std::runtime_error("Transfer rule violated (same account)");
        }*/
    }

    // Non si possono effettuare spese nel momento in cui il conto corrente è in negativo
    if (t->getType() == "Expense" && balance() + t->getValue() < 0) {
        std::cerr << "Insufficient funds for withdrawal\n";
        throw std::runtime_error("Insufficient balance");
    }
    transactions.push_back(std::move(t));
}

double BankAccount::balance() const {
    double total = 0.0;
    for (const auto& t : transactions) {
        total += t->getValue();
    }
    return total;
}

void BankAccount::printTransactions() const {
    std::cout << "\n--- Transaction List ---\n";
    std::vector<const Transaction*> trs;
    trs.reserve(transactions.size());
    for (const auto& t : transactions) {
        trs.push_back(t.get());
    }
    std::ranges::sort(trs, std::ranges::less{}, &Transaction::getData);

    double totalDeposits = 0.0, totalWithdrawals = 0.0;
    for (const auto* t : trs) {
        std::cout << std::format(
            "ID: {}\n"
            "|Date: {}\n"
            "|Amount: {:.2f}\n"
            "|Type: {}\n"
            "|Operation: {}\n"
            "|Category: {}\n"
            "|Description: {}\n\n",
            t->getId(), t->getDataFormatted(), t->getAmount(),
            t->getType(), t->getOperationType(), t->getCategory(), t->getDescription()
        );
        const double val = t->getValue();
        if (val >= 0) totalDeposits += val;
        else          totalWithdrawals += -val;
    }

    std::cout << "----------------------------------------------\n";
    std::cout << std::format(
        "Total Deposits: {:.2f}\nTotal Withdrawals: {:.2f}\nBalance: {:.2f}\n",
        totalDeposits, totalWithdrawals, balance()
    );
}

void BankAccount::SaveToFile(const std::string& filename, const std::string& pwd) const {
    if (!verifyPwd(pwd)) {
        std::cerr << "Access denied: incorrect password\n";
        throw std::runtime_error("Invalid password");
    }

    std::ofstream file(filename);
    if (!file) throw std::runtime_error("Error opening file");

    file << std::format("Account Owner: {}, Bank: {}\n", ownerId, bankId);
    file << "ID,Date,Amount,Type,Operation,Category,Description,Sender,Receiver\n";

    std::vector<const Transaction*> sorted;
    sorted.reserve(transactions.size());
    for (const auto& t : transactions) {
        sorted.push_back(t.get());
    }
    std::ranges::sort(sorted, std::ranges::less{}, &Transaction::getData);

    double totalDeposits = 0.0, totalWithdrawals = 0.0;
    for (const auto* t : sorted) {
        file << std::format("{},{},{:.2f},{},{},{},{},{},{}\n",
                            t->getId(), t->getDataFormatted(), t->getAmount(),
                            t->getType(), t->getOperationType(), t->getCategory(),
                            t->getDescription(), t->getSenderAccount(), t->getReceiverAccount());
        const double val = t->getValue();
        if (val >= 0) totalDeposits += val;
        else          totalWithdrawals += -val;
    }

    file << std::format("Summary,,Total Deposits: {:.2f},Total Withdrawals: {:.2f},Final Balance: {:.2f}\n",
                        totalDeposits, totalWithdrawals, balance());
}

void BankAccount::ReadFromFile(const std::string& filename, const std::string& pwd) {
    if (!verifyPwd(pwd)) throw std::runtime_error("Invalid password");

    std::ifstream file(filename);
    if (!file) throw std::runtime_error("Error opening file");

    std::string line;

    // 1) Prima riga: "Account Owner: X, Bank: Y" -> validazione coerenza
    if (!std::getline(file, line)) throw std::runtime_error("Empty file");
    {
        // Parsing semplice e robusto
        const std::string ownerKey = "Account Owner: ";
        const std::string bankKey  = ", Bank: ";

        const auto pOwner = line.find(ownerKey);
        const auto pBank  = line.find(bankKey);
        if (pOwner == std::string::npos || pBank == std::string::npos || pBank <= pOwner + ownerKey.size()) {
            throw std::runtime_error("Malformed header line: " + line);
        }
        const std::string fileOwner = line.substr(pOwner + ownerKey.size(), pBank - (pOwner + ownerKey.size()));
        const std::string fileBank  = line.substr(pBank + bankKey.size());

        if (fileOwner != ownerId || fileBank != bankId) {
            throw std::runtime_error("File does not match this account (owner/bank mismatch)");
        }
    }

    // 2) Header CSV (atteso con Sender,Receiver)
    if (!std::getline(file, line)) throw std::runtime_error("Missing CSV header");
    // opzionale: validare che contenga tutte le colonne attese

    // 3) Righe dati
    while (std::getline(file, line)) {
        if (line.rfind("Summary", 0) == 0) {
            break; // ignora il sommario
        }

        // Splitta 9 campi: ID,Date,Amount,Type,Operation,Category,Description,Sender,Receiver
        std::array<std::string, 9> cols{};
        {
            std::stringstream ss(line);
            for (int i = 0; i < 9; ++i) {
                if (!std::getline(ss, cols[i], ',')) {
                    throw std::runtime_error("Malformed CSV line: " + line);
                }
            }
        }

        const std::string& id        = cols[0];
        const std::string& dateS     = cols[1];
        const std::string& amtS      = cols[2];
        const std::string& type      = cols[3];
        const std::string& op        = cols[4];
        const std::string& cat       = cols[5];
        const std::string& desc      = cols[6];
        const std::string& senderAcc = cols[7];
        const std::string& recvAcc   = cols[8];

        double amount = 0.0;
        try {
            amount = std::stod(amtS);
        } catch (...) {
            throw std::runtime_error("Invalid amount: " + amtS);
        }

        TimePoint tp = parseDateTime(dateS);


        /* In fase di import NON applichiamo le regole di validazione dei transfer:
         inseriamo direttamente nella lista, così anche le righe "Transfer" vengono
         ripristinate fedelmente dallo storico.*/
        if (type == "Income") {
            auto tx = std::make_unique<Income>(id, tp, amount, desc, cat, op, senderAcc, recvAcc);
            transactions.push_back(std::move(tx));
        } else if (type == "Expense") {
            auto tx = std::make_unique<Expense>(id, tp, amount, desc, cat, op, senderAcc, recvAcc);
            transactions.push_back(std::move(tx));
        } else {
            throw std::runtime_error("Unknown Type in CSV: " + type);
        }
    }
}