//
// Created by Andrea Peli on 11/12/25.
//

#include <gtest/gtest.h>
#include "Bank_Account.h"
#include "Income.h"
#include "Expense.h"
#include <chrono>
#include <memory>


using namespace std::chrono;

class TestBankAccount : public ::testing::Test {
protected:
    std::unique_ptr<BankAccount> accountA;
    std::unique_ptr<BankAccount> accountB;
    std::unique_ptr<BankAccount> accountA2;
    const std::string pwdA = "passwordA";
    const std::string pwdB = "passwordB";

    system_clock::time_point now = system_clock::now();

    void SetUp() override {
        accountA = std::make_unique<BankAccount>("Alice", "BankA", pwdA);
        accountB = std::make_unique<BankAccount>("Bob", "BankB", pwdB);
        accountA2 = std::make_unique<BankAccount>("Alice", "BankB", pwdA);
    }

    std::unique_ptr<Income> makeIncome(double amount, const std::string& desc = "income", const std::string& id = "INC-001") {
        return std::make_unique<Income>(
            id, now, amount, desc, "salary", "Income", "Alice", "Alice");
    }

    std::unique_ptr<Expense> makeExpense(double amount, const std::string& desc = "expense", const std::string& id = "EXP-001") {
        return std::make_unique<Expense>(
            id, now, amount, desc, "general", "Expense", "Alice", "Alice");
    }
};

TEST_F(TestBankAccount, BasicTransactionsAndBalance) {
    auto income = makeIncome(100.0, "paycheck", "INC-001");
    accountA->addTransaction(std::move(income));
    EXPECT_DOUBLE_EQ(accountA->balance(), 100.0);

    auto expense = makeExpense(40.0, "groceries", "EXP-001");
    accountA->addTransaction(std::move(expense));
    EXPECT_DOUBLE_EQ(accountA->balance(), 60.0);

    accountA->addTransaction(std::move(makeIncome(50.0, "bonus", "INC-002")));
    accountA->addTransaction(std::move(makeExpense(10.0, "snacks", "EXP-002")));
    EXPECT_DOUBLE_EQ(accountA->balance(), 100.0);
}

TEST_F(TestBankAccount, WithdrawMoreThanBalanceThrows) {
    accountA->addTransaction(std::move(makeIncome(50.0, "deposit", "INC-003")));
    auto expense = makeExpense(70.0, "big purchase", "EXP-003");
    EXPECT_THROW(accountA->addTransaction(std::move(expense)), std::runtime_error);
}

TEST_F(TestBankAccount, FilterByTypeAndCounterparty) {
    accountA->addTransaction(std::move(makeIncome(100.0, "salary", "INC-004")));
    accountA->addTransaction(std::move(makeExpense(30.0, "dinner", "EXP-004")));
    accountA->addTransaction(std::move(makeIncome(20.0, "gift", "INC-005")));
    accountA->addTransaction(std::move(makeExpense(10.0, "taxi", "EXP-005")));

    auto incomes = accountA->filterByType("Income");
    EXPECT_GE(incomes.size(), 2);
    for (const auto* t : incomes) {
        EXPECT_EQ(t->getType(), "Income");
    }

    auto expenses = accountA->filterByType("Expense");
    EXPECT_GE(expenses.size(), 2);
    for (const auto* t : expenses) {
        EXPECT_EQ(t->getType(), "Expense");
    }

    // Add a transfer transaction from accountA to accountA2
    auto transferIncome = std::make_unique<Income>(
        "INC-006", now, 50.0, "transfer", "transfer", "Income", "Alice", "Alice");
    accountA->addTransaction(std::move(transferIncome), accountA2.get());

    auto filteredByCounterparty = accountA->filterByCounterparty("Alice");
    EXPECT_GE(filteredByCounterparty.size(), 1);
    for (const auto* t : filteredByCounterparty) {
        EXPECT_EQ(t->getReceiverAccount(), "Alice");
    }
}

TEST_F(TestBankAccount, PrintFunctionsWithPassword) {
    accountA->addTransaction(std::move(makeIncome(100.0, "salary", "INC-007")));
    accountA->addTransaction(std::move(makeExpense(40.0, "groceries", "EXP-006")));

    // Should print without throwing
    EXPECT_NO_THROW(accountA->printTransactions());

    auto incomes = accountA->filterByType("Income");
    if (!incomes.empty()) {
        const std::string txId = incomes[0]->getId();
        EXPECT_NO_THROW(accountA->printTransactionById(pwdA, txId));
    }

    EXPECT_NO_THROW(accountA->printTransactionsByType(pwdA, "Income"));
    EXPECT_NO_THROW(accountA->printTransactionsByAccount(pwdA, "Alice"));

    // Wrong password should throw when printing by id
    if (!incomes.empty()) {
        const std::string txId = incomes[0]->getId();
        EXPECT_THROW(accountA->printTransactionById("wrongpwd", txId), std::runtime_error);
    }
}

TEST_F(TestBankAccount, SaveToFileAndReadFromFileRoundTrip) {
    accountA->addTransaction(std::move(makeIncome(100.0, "salary", "INC-008")));
    accountA->addTransaction(std::move(makeExpense(40.0, "groceries", "EXP-007")));

    auto transferIncome = std::make_unique<Income>(
        "INC-009", now, 50.0, "transfer", "transfer", "Income", "Alice", "Alice");
    accountA->addTransaction(std::move(transferIncome), accountA2.get());

    const std::string filename = "test_account.csv";

    EXPECT_NO_THROW(accountA->SaveToFile(filename, pwdA));

    BankAccount loadedAccount(accountA->getOwnerId(), accountA->getBankId(), pwdA);
    EXPECT_NO_THROW(loadedAccount.ReadFromFile(filename, pwdA));

    EXPECT_EQ(accountA->getOwnerId(), loadedAccount.getOwnerId());
    EXPECT_DOUBLE_EQ(accountA->balance(), loadedAccount.balance());

    // Check that transactions match by id and amount
    auto origIncomes = accountA->filterByType("Income");
    auto loadedIncomes = loadedAccount.filterByType("Income");
    EXPECT_EQ(origIncomes.size(), loadedIncomes.size());
    for (size_t i = 0; i < origIncomes.size(); ++i) {
        EXPECT_EQ(origIncomes[i]->getId(), loadedIncomes[i]->getId());
        EXPECT_DOUBLE_EQ(origIncomes[i]->getAmount(), loadedIncomes[i]->getAmount());
        EXPECT_EQ(origIncomes[i]->getDescription(), loadedIncomes[i]->getDescription());
    }

    auto origExpenses = accountA->filterByType("Expense");
    auto loadedExpenses = loadedAccount.filterByType("Expense");
    EXPECT_EQ(origExpenses.size(), loadedExpenses.size());
    for (size_t i = 0; i < origExpenses.size(); ++i) {
        EXPECT_EQ(origExpenses[i]->getId(), loadedExpenses[i]->getId());
        EXPECT_DOUBLE_EQ(origExpenses[i]->getAmount(), loadedExpenses[i]->getAmount());
        EXPECT_EQ(origExpenses[i]->getDescription(), loadedExpenses[i]->getDescription());
    }

    std::remove(filename.c_str());
}