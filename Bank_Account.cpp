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

std::vector<const Transaction*> BankAccount::getSortedTransactions() const {
    std::vector<const Transaction*> sorted;
    sorted.reserve(transactions.size());
    for (const auto& t : transactions) {
        sorted.push_back(t.get());
    }
    std::ranges::sort(sorted, std::ranges::less{}, &Transaction::getData);
    return sorted;
}

BankAccount::Summary BankAccount::computeSummary() const {
    Summary summary{};
    summary.balance = balance();
    for (const auto* t : getSortedTransactions()) {
        const double val = t->getValue();
        if (val >= 0) summary.deposits += val;
        else          summary.withdrawals += -val;
    }
    return summary;
}

static void printTransaction(const Transaction& t) {
    std::cout << std::format(
        "ID: {}\n"
        "Date: {}\n"
        "Amount: {:.2f}\n"
        "Operation: {}\n"
        "Category: {}\n"
        "Description: {}\n"
        "Sender: {}\n"
        "Receiver: {}\n\n",
        t.getId(),
        t.getDataFormatted().substr(0, 19),
        t.getAmount(),
        t.getOperationType(),
        t.getCategory(),
        t.getDescription(),
        t.getSenderAccount(),
        t.getReceiverAccount()
    );
}

template <typename Pred>
void BankAccount::printFiltered(const std::string& pwd, Pred predicate) const {
    requireAuth(pwd);
    std::vector<const Transaction*> filtered;
    for (const auto& t : transactions) {
        if (predicate(*t)) {
            filtered.push_back(t.get());
        }
    }
    if (filtered.empty()) {
        std::cout << "No transactions found\n";
        return;
    }
    std::ranges::sort(filtered, std::ranges::less{}, &Transaction::getData);
    for (const auto* t : filtered) {
        printTransaction(*t);
    }
}

void BankAccount::validateTransfer(const BankAccount* destinationAccount) const {
    if (!destinationAccount) {
        throw std::invalid_argument("Transfer requires a destination account");
    }
    if (this->getOwnerId() == destinationAccount->getOwnerId() &&
        this->getBankId() == destinationAccount->getBankId()) {
        throw std::runtime_error("Transfer rule violated");
    }
}

void BankAccount::addTransaction(std::unique_ptr<Transaction> t,
                                 const BankAccount* destinationAccount) {
    // Regole per i trasferimenti:
    // Non si possono effettuare spese se supera la soglia del saldo presete nel conto
    if (t->getType() == "Expense" && balance()+ t->getValue() <0 ){
        throw std::runtime_error("Insufficient balance");
    }
    // deve esserci un destinatario
    if (t->getCategory() == "Transfer") {
        validateTransfer(destinationAccount);
    }
    // Nota: non viene controllata la duplicazione degli ID, si assume che siano unici
    transactions.push_back(std::move(t));
}

double BankAccount::balance() const {
    double total = 0.0;
    for (const auto& t : transactions) {
        total += t->getValue();
    }
    return total;
}

void BankAccount::requireAuth(const std::string& pwd) const {
    if (!verifyPwd(pwd)) {
        throw std::runtime_error("Access denied: incorrect password");
    }
}

const Transaction* BankAccount::findTransactionById(const std::string& txId) const {
    for (const auto& t : transactions) {
        if (t->getId() == txId) {
            return t.get();
        }
    }
    return nullptr;
}

std::vector<const Transaction*> BankAccount::filterByType(const std::string& opType) const {
    std::vector<const Transaction*> out;
    for (const auto& t : transactions) {
        if (t->getOperationType() == opType) {
            out.push_back(t.get());
        }
    }
    return out;
}

std::vector<const Transaction*> BankAccount::filterByCounterparty(const std::string& accountId) const {
    std::vector<const Transaction*> out;
    for (const auto& t : transactions) {
        if (t->getSenderAccount() == accountId ||
            t->getReceiverAccount() == accountId) {
            out.push_back(t.get());
        }
    }
    return out;
}

void BankAccount::printTransactionById(const std::string& pwd,
                                       const std::string& txId) const {
    printFiltered(pwd, [&](const Transaction& t) {
        return t.getId() == txId;
    });
}

void BankAccount::printTransactionsByType(const std::string& pwd,
                                          const std::string& opType) const {
    printFiltered(pwd, [&](const Transaction& t) {
        return t.getOperationType() == opType;
    });
}

void BankAccount::printTransactionsByAccount(const std::string& pwd,
                                             const std::string& accountId) const {
    printFiltered(pwd, [&](const Transaction& t) {
        return t.getSenderAccount() == accountId || t.getReceiverAccount() == accountId;
    });
}

void BankAccount::printTransactions() const {
    std::cout << "\n--- Transaction List ---\n";

    auto sorted = getSortedTransactions();
    Summary summary = computeSummary();

    for (const auto* t : sorted) {
        printTransaction(*t);
    }

    std::cout << "----------------------------------------------\n";
    std::cout << std::format(
        "Total Deposits: \x1b[38;2;0;100;0m{:.2f}\x1b[0m\n Total Withdrawals: \x1b[38;2;139;0;0m{:.2f}\x1b[0m\nBalance: {:.2f}\n",
        summary.deposits, summary.withdrawals, summary.balance
    );
}

void BankAccount::SaveToFile(const std::string& filename, const std::string& pwd) const {
    requireAuth(pwd);

    std::ofstream file(filename);
    if (!file) throw std::runtime_error("Error opening file");

    file << std::format("Account Owner: {}, Bank: {}\n", ownerId, bankId);
    file << "ID,Date,Amount,Operation,Category,Description,Sender,Receiver\n";

    auto sorted = getSortedTransactions();
    Summary summary = computeSummary();

    for (const auto* t : sorted) {
        file << std::format("{},{},{:.2f},{},{},{},{},{}\n",
                            t->getId(), t->getDataFormatted().substr(0,19), t->getAmount(),
                            t->getOperationType(), t->getCategory(),
                            t->getDescription(), t->getSenderAccount(), t->getReceiverAccount());
    }

    file << std::format("Summary, Total Deposits: {:.2f},Total Withdrawals: {:.2f},Final Balance: {:.2f}\n",
                        summary.deposits, summary.withdrawals, summary.balance);
}

void BankAccount::ReadFromFile(const std::string& filename, const std::string& pwd) {
    requireAuth(pwd);
    transactions.clear();

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

        // Splitta 8 campi: ID,Date,Amount,Operation,Category,Description,Sender,Receiver
        std::array<std::string, 8> cols{};
        {
            std::stringstream ss(line);
            for (int i = 0; i < 8; ++i) {
                if (!std::getline(ss, cols[i], ',')) {
                    throw std::runtime_error("Malformed CSV line: " + line);
                }
            }
        }

        const std::string& id        = cols[0];
        const std::string& dateS     = cols[1];
        const std::string& amtS      = cols[2];
        const std::string& op        = cols[3];
        const std::string& cat       = cols[4];
        const std::string& desc      = cols[5];
        const std::string& senderAcc = cols[6];
        const std::string& recvAcc   = cols[7];

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
        if (op == "Income") {
            auto tx = std::make_unique<Income>(id, tp, amount, desc, cat, op, senderAcc, recvAcc);
            transactions.push_back(std::move(tx));
        } else if (op == "Expense") {
            auto tx = std::make_unique<Expense>(id, tp, amount, desc, cat, op, senderAcc, recvAcc);
            transactions.push_back(std::move(tx));
        } else {
            throw std::runtime_error("Unknown Operation in CSV: " + op);
        }
    }
}