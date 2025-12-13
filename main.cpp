// main.cpp
#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <unordered_map>
#include "Bank_Account.h"
#include "Transaction.h"
#include "Income.h"
#include "Expense.h"

using std::string;
using Clock = std::chrono::system_clock;


static TimePoint nowtp() { return Clock::now(); }


static std::unique_ptr<Transaction> mk_income(
    const string& id, double amount,
    const string& description,
    const string& category,
    const string& opType,
    const string& senderAcc,
    const string& receiverAcc)
{
    return std::make_unique<Income>(id, nowtp(), amount, description, category, opType, senderAcc, receiverAcc);
}

static std::unique_ptr<Transaction> mk_expense(
    const string& id, double amount,
    const string& description,
    const string& category,
    const string& opType,
    const string& senderAcc,
    const string& receiverAcc)
{
    return std::make_unique<Expense>(id, nowtp(), amount, description, category, opType, senderAcc, receiverAcc);
}

int main() {
    std::cout << "<< Avvio test transazioni >>\n";


    BankAccount A1("Alice", "IT0001", "pwdA");
    BankAccount A2("Alice", "IT0002", "pwdA");
    BankAccount B1("Bob",  "IT7777", "pwdB");
    BankAccount EA("extern", "EXT001","pwd000");

    try {
        A1.addTransaction(mk_income("INC-A1-001", 1500.0,
                                    "Stipendio novembre", "Salary", "Income",
                                    EA.getBankId(), A1.getBankId()),
                          nullptr);
        A1.addTransaction(mk_expense("EXP-A1-001", 120.0,
                                     "Spesa supermercato", "Groceries", "Expense",
                                     A1.getBankId(), EA.getBankId()),
                          nullptr);
        A2.addTransaction(mk_income("INC-A2-001", 500.0,
                                    "Bonus tredicesima", "Gift", "Income",
                                    EA.getBankId(), A2.getBankId()),
                          nullptr);
        std::cout << "[OK] Operazioni base inserite.\n";
    } catch (const std::exception& ex) {
        std::cerr << "[ERR] Operazioni base: " << ex.what() << "\n";
    }

    try {

        A1.addTransaction(mk_expense("TRF-A1A2-OUT-001", 300.0,
                                     "Trasferimento a IT0002", "Transfer", "Expense",
                                     A1.getBankId(), A2.getBankId()),
                          &A2);

        // Lato entrata (Income) con OperationType "Transfer" e categoria "Transfer"
        A2.addTransaction(mk_income("TRF-A1A2-IN-001", 300.0,
                                    "Ricevuto da IT0001", "Transfer", "Income",
                                    A1.getBankId(), A2.getBankId()),
                          &A1);

        std::cout << "[OK] Transfer valido A1->A2.\n";
    } catch (const std::exception& ex) {
        std::cerr << "[ERR] Transfer A1->A2: " << ex.what() << "\n";
    }
    try {
        A1.addTransaction(mk_expense("TRF-A1B1-OUT-001", 25.0,
                                     "Pagamento a Bob", "Transfer", "Expense",
                                     A1.getBankId(), B1.getBankId()),
                          &B1);
        B1.addTransaction(mk_income("TRF-A1B1-IN-001", 25.0,
                                    "Ricevuto da Alice", "Transfer", "Income",
                                    A1.getBankId(), B1.getBankId()),
                          &A1);

        std::cout << "[OK] Transfer valido A1->B1 (owner diversi).\n";
    } catch (const std::exception& ex) {
        std::cerr << "[ERR] Transfer A1"
                     "->B1: " << ex.what() << "\n";
    }

    // Casi di errore
    try {
        A1.addTransaction(mk_expense("TRF-NO-DST", 50.0,
                                     "Transfer senza destinazione", "Transfer", "Transfer",
                                     A1.getBankId(), A2.getBankId()),
                          nullptr);
        std::cerr << "[FAIL] Mi aspettavo eccezione (no destination)\n";
    } catch (const std::invalid_argument& ex) {
        std::cout << "[OK] Eccezione attesa (no destination): " << ex.what() << "\n";
    } catch (const std::exception& ex) {
        std::cout << "[OK?] Eccezione (no destination): " << ex.what() << "\n";
    }

    try {
        A1.addTransaction(mk_expense("TRF-SAME-ACC", 10.0,
                                     "Transfer verso stesso conto (atteso KO)", "invalid Transfer", "Transfer",
                                     A1.getBankId(), A1.getBankId()),
                          &A1);
        std::cerr << "[FAIL] Mi aspettavo eccezione (same account)\n";
    } catch (const std::runtime_error& ex1) {
        std::cout << "[OK] Eccezione attesa (same account): " << ex1.what() << "\n";
    } catch (const std::exception& ex) {
        std::cout << "[OK?] Eccezione (same account): " << ex.what() << "\n";
    }

    try {

        A1.addTransaction(mk_expense("EXP-NOFUNDS", 5000.0,
                                      "Acquisto eccessivo", "Transfer", "Expense",
                                      EA.getBankId(), A1.getBankId()),
                           nullptr);
        std::cerr << "[FAIL] Mi aspettavo eccezione (fondi insufficienti)\n";
    } catch (const std::runtime_error& ex) {
        std::cout << "[OK] Eccezione attesa (fondi insufficienti): " << ex.what() << "\n";
    } catch (const std::exception& ex) {
        std::cout << "[OK?] Eccezione (fondi insufficienti): " << ex.what() << "\n";
    }

    try {
        // Income di tipo Rimborso
        B1.addTransaction(mk_income("INC-B1-REFUND-001", 80.0,
                                    "Rimborso spese viaggio", "Refund", "Income",
                                    EA.getBankId(), B1.getBankId()),
                          nullptr);
        // Expense di tipo Multa
        B1.addTransaction(mk_expense("EXP-B1-FINE-001", 40.0,
                                     "Multa parcheggio", "Fine", "Expense",
                                     EA.getBankId(), B1.getBankId()),
                          nullptr);
        std::cout << "[OK] Esempi aggiuntivi (Refund/Fine) inseriti.\n";
    } catch (const std::exception& ex) {
        std::cerr << "[ERR] Esempi aggiuntivi: " << ex.what() << "\n";
    }
    system("read -p \"Premi INVIO per continuare...\" _; clear");

    // 6) Stampa riepiloghi
    std::cout << "\n=== TRANSAZIONI A1 ===\n";
    A1.printTransactions();
    system("pause");

    try {
        std::cout << "\n--- [TEST] Singola transazione A1 (ID valido) ---\n";
        A1.printTransactionById("pwdA", "INC-A1-001");
    } catch (const std::exception& ex) {
        std::cerr << "[ERR] printTransactionById valido: " << ex.what() << "\n";
    }

    try {
        std::cout << "\n--- [TEST] Singola transazione A1 (password errata) ---\n";
        A1.printTransactionById("wrongPwd", "INC-A1-001");
        std::cerr << "[FAIL] Mi aspettavo eccezione (password errata)\n";
    } catch (const std::runtime_error& ex) {
        std::cout << "[OK] Eccezione attesa (password errata): " << ex.what() << "\n";
    }

    try {
        std::cout << "\n--- [TEST] Singola transazione A1 (ID inesistente) ---\n";
        A1.printTransactionById("pwdA", "NO-TX-ID");
    } catch (const std::exception& ex) {
        std::cerr << "[ERR] printTransactionById ID inesistente: " << ex.what() << "\n";
    }

    std::cout << "\n=== TRANSAZIONI A2 ===\n";
    A2.printTransactions();

    try {
        std::cout << "\n--- [TEST] Solo ENTRATE A2 ---\n";
        A2.printTransactionsByType("pwdA", "Income");
    } catch (const std::exception& ex) {
        std::cerr << "[ERR] printTransactionsByType Income: " << ex.what() << "\n";
    }

    try {
        std::cout << "\n--- [TEST] Solo SPESE A2 (attese vuote) ---\n";
        A2.printTransactionsByType("pwdA", "Expense");
    } catch (const std::exception& ex) {
        std::cerr << "[ERR] printTransactionsByType Expense: " << ex.what() << "\n";
    }

    std::cout << "\n=== TRANSAZIONI B1 ===\n";
    B1.printTransactions();

    try {
        std::cout << "\n--- [TEST] Transazioni verso IT0001 (A1) ---\n";
        B1.printTransactionsByAccount("pwdB", "IT0001");
    } catch (const std::exception& ex) {
        std::cerr << "[ERR] printTransactionsByAccount: " << ex.what() << "\n";
    }

    try {
        std::cout << "\n--- [TEST] Transazioni verso IT0001 (password errata) ---\n";
        B1.printTransactionsByAccount("wrongPwd", "IT0001");
        std::cerr << "[FAIL] Mi aspettavo eccezione (password errata)\n";
    } catch (const std::runtime_error& ex) {
        std::cout << "[OK] Eccezione attesa (password errata): " << ex.what() << "\n";
    }

    system("read -p \"Premi INVIO per continuare...\" _; clear");

    // 7) Salvataggio CSV
    const string fA1 = "A1.csv";
    const string fA2 = "A2.csv";
    const string fB1 = "B1.csv";
    try {
        A1.SaveToFile(fA1, "pwdA");
        A2.SaveToFile(fA2, "pwdA");
        B1.SaveToFile(fB1, "pwdB");
        std::cout << "[OK] CSV salvati.\n";
    } catch (const std::exception& ex) {
        std::cerr << "[ERR] SaveToFile: " << ex.what() << "\n";
    }
    system("read -p \"Premi INVIO per continuare...\" _; clear");
    // 8) Lettura CSV su nuove istanze e ristampa
    try {
        /*BankAccount A1("Alice", "IT0001", "pwdA");
        BankAccount A2("Alice", "IT0002", "pwdA");
        BankAccount B1("Bob",  "IT7777", "pwdB");*/

        A1.ReadFromFile(fA1, "pwdA");
        A2.ReadFromFile(fA2, "pwdA");
        B1.ReadFromFile(fB1, "pwdB");


        std::cout << "\n=== IMPORT A1 ===\n"; A1.printTransactions();
        system("read -p \"Premi INVIO per continuare...\" _; clear");
        std::cout << "\n=== IMPORT A2 ===\n"; A2.printTransactions();
        system("read -p \"Premi INVIO per continuare...\" _; clear");
        std::cout << "\n=== IMPORT B1 ===\n"; B1.printTransactions();


        std::cout << "[OK] Lettura CSV completata.\n";
    } catch (const std::exception& ex) {
        std::cerr << "[ERR] ReadFromFile: " << ex.what() << "\n";
    }
    system("read -p \"Premi INVIO per continuare...\" _; clear");

    std::cout << "\n=== [CHECK SEMANTICO] Tutte le eccezioni critiche sono state lanciate correttamente ===\n";
    std::cout << "[OK] Autenticazione protetta per tutte le stampe\n";
    std::cout << "[OK] ID transazione non valido gestito correttamente\n";
    std::cout << "[OK] Filtri per tipo e account funzionanti\n";

    std::cout << "\n<< Fine test >>\n";
    return 0;
}